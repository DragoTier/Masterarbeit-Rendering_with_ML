#include <tensorflow/lite/nnapi/nnapi_implementation.h>
#include <android/log.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/c/c_api_internal.h>
#include "SuperResolution.h"

namespace espcn {

constexpr int cpu_nr_of_threads = 8;
const NnApi* nnapi_object;

/**
 * Initializer for SuperResolution Class and TFLite Interpreter with chosen delegate option
 *
 * https://github.com/tensorflow/examples/tree/master/lite/examples/super_resolution/android
 * https://www.tensorflow.org/lite/api_docs/cc
 * example used for reference on basic structure as there is only little to no documentation on how to use the TFLite API with C++...
 * Most of the documentation is obsolete as there are already new methods implemented in the API, which implement the code snippets provided in the docs and more.
 *
 * Also: the CmakeLists.txt for building the library that comes with the current tflite-lib has errors that need to be fixed manually.
 */
SuperResolution::SuperResolution(const void* model_data, size_t model_size, int accelerator) {

    options_ = TfLiteInterpreterOptionsCreate();
    model_ = TfLiteModelCreate(model_data, model_size);
    if (!model_) {
        LOGERROR("model failed");
        return;
    }

    // 0 => CPU, 1 => NNAPI, 2 => GPU
    switch (accelerator) {
        case 0:
            TfLiteInterpreterOptionsSetNumThreads(options_, cpu_nr_of_threads);
            break;
        case 1:
            {
                nnapi_object = NnApiImplementation();
                tflite::StatefulNnApiDelegate::Options nnapi_options = tflite::StatefulNnApiDelegate::Options();
                nnapi_options.execution_preference = tflite::StatefulNnApiDelegate::Options::kSustainedSpeed;
                nnapi_options.max_number_delegated_partitions = 0;
                //nnapi_options.accelerator_name = "qti-hta";
                //nnapi_options.accelerator_name = "google-armnn";
                //nnapi_options.accelerator_name = "google-edgetpu"; //<- Nur für Verwendung mit Google Pixel 6
                delegate_ = new tflite::StatefulNnApiDelegate(nnapi_object, nnapi_options);
                TfLiteInterpreterOptionsAddDelegate(options_, delegate_);
                break;
            }
        case 2:
        {
            TfLiteGpuDelegateOptionsV2 default_options = TfLiteGpuDelegateOptionsV2Default();
            default_options.max_delegated_partitions = 3;
            default_options.inference_priority1 = TFLITE_GPU_INFERENCE_PRIORITY_MIN_LATENCY;
            delegate_ = TfLiteGpuDelegateV2Create(&default_options);
            TfLiteInterpreterOptionsAddDelegate(options_, delegate_);
            break;
        }
        default:
            TfLiteInterpreterOptionsSetNumThreads(options_, cpu_nr_of_threads);
            break;
    }

    //new instance with interpreter
    interpreter_ = TfLiteInterpreterCreate(model_, options_);
    if (interpreter_) {
        LOGINFO("interpreter initialized");
    } else {
        LOGERROR("interpreter initialization failed");
        return;
    }

    int inputCount = TfLiteInterpreterGetInputTensorCount(interpreter_);
    LOGINFO("Input count %i", inputCount);
    int inputDims[4] = {1, inputImageHeight, inputImageWidth, imageChannels};
    const int * CONST_DIMS = inputDims;
    LOGINFO("dims size %d", sizeof(CONST_DIMS));
    TfLiteStatus status = TfLiteInterpreterAllocateTensors(interpreter_);
    if (status != kTfLiteOk) {
        LOGERROR("allocating tensors failed");
        return;
    } else {
        LOGINFO("tensors allocated for default size");
    }
    //resize input tensor once for all upcoming iterations
    status = TfLiteInterpreterResizeInputTensor(interpreter_, 0, CONST_DIMS, 4);
    if (status != kTfLiteOk) {
        LOGERROR("tensor resizing failed");
        return;
    } else {
        LOGINFO("tensors reszied correctly");
    }
    status = TfLiteInterpreterAllocateTensors(interpreter_);
    if (status != kTfLiteOk) {
        LOGERROR("allocating tensors failed");
        return;
    } else {
        LOGINFO("tensors allocated for pref size");
    }
}

SuperResolution::~SuperResolution() {
    delete interpreter_;
    delete options_;
    delete model_;
}

bool SuperResolution::IsInterpreterInitialized() {
    if (interpreter_) {
        return true;
    } else {
        return false;
    }
}

int64_t getTimeNsec() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t) now.tv_sec*1000000000LL + now.tv_nsec;
}

bool SuperResolution::isResponding(){
    return true;
}

uint8_t* SuperResolution::DoSuperResolution(uint8_t* yuv_img) {

    int* yuv_img_int = reinterpret_cast<int*>(yuv_img);
    // Get Y (greyscale) from YUV
    uint8_t * input_buffer = new uint8_t[numberOfInputPixels * imageChannels];
    for (int i = 0; i < numberOfInputPixels; i++) {
        input_buffer[i] = static_cast<uint8_t>(( yuv_img_int[i]) & 0xff);
    }

    //need to be allocated for each iteration
    TfLiteInterpreterAllocateTensors(interpreter_);

    int input = interpreter_->impl->inputs()[0];
    auto batch_size = interpreter_->impl->tensor(input)->dims->data[0];
    auto height = interpreter_->impl->tensor(input)->dims->data[1];
    auto width = interpreter_->impl->tensor(input)->dims->data[2];
    auto channels = interpreter_->impl->tensor(input)->dims->data[3];

    //get input tensor and copy input buffer with greyscale image to the tensor
    TfLiteTensor* input_tensor = TfLiteInterpreterGetInputTensor(interpreter_, input);
    TfLiteTensorCopyFromBuffer(input_tensor, input_buffer, inputImageHeight * inputImageWidth * sizeof(uint8_t));

    //run the inference
    TfLiteInterpreterInvoke(interpreter_);

    //copy the upscaled image from the output tensor to the output buffer
    const TfLiteTensor* output_tensor = TfLiteInterpreterGetOutputTensor(interpreter_, 0);
    uint8_t * output_buffer = new uint8_t[numberOfOutputPixels * imageChannels];
    TfLiteTensorCopyToBuffer(output_tensor, output_buffer,outputImageHeight * outputImageWidth * sizeof(uint8_t));

    //delete the input_buffer because image is already processed by now
    delete[] input_buffer;

    return output_buffer;
}


}  // namespace espcn

