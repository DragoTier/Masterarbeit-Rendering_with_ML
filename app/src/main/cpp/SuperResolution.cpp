#include "SuperResolution.h"

//#include <android/NeuralNetworks.h>
#include <tensorflow/lite/nnapi/nnapi_implementation.h>
#include <android/log.h>
#include <math.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/c/c_api_internal.h>


namespace tflite {
namespace superresolution {

constexpr int kThreadNum = 12;
const NnApi* nnapi_object;
StatefulNnApiDelegate* delegate;

SuperResolution::SuperResolution(const void* model_data, size_t model_size,
                                 int use_accelerator) {
    // Load the model
    model_ = TfLiteModelCreate(model_data, model_size);
    if (!model_) {
        LOGE("Failed to create TFLite model");
    return;
    }

    // Create the interpreter options
    TfLiteInterpreterOptions* options = TfLiteInterpreterOptionsCreate();
    //options->use_nnapi = true;
    options_ = options;

    // 0 => CPU, 1 => NNAPI, 2 => GPU
    switch (use_accelerator) {
        case 0:
            TfLiteInterpreterOptionsSetNumThreads(options_, kThreadNum);
            break;
        case 1:
            {
                nnapi_object = NnApiImplementation();
                StatefulNnApiDelegate::Options nnapi_options = StatefulNnApiDelegate::Options();
                nnapi_options.execution_preference = StatefulNnApiDelegate::Options::kSustainedSpeed;
                nnapi_options.max_number_delegated_partitions = 0;
                //nnapi_options.allow_fp16 = true;
                //nnapi_options.use_burst_computation = true;
               // nnapi_options.accelerator_name = "qti-dsp";
                //nnapi_options.accelerator_name = "google-armnn";
                nnapi_options.accelerator_name = "google-edgetpu";
                //nnapi_options.disallow_nnapi_cpu = true;
                delegate_ = new StatefulNnApiDelegate(nnapi_object, nnapi_options);
                TfLiteInterpreterOptionsAddDelegate(options_, delegate_);
                break;
            }
        case 2:
        {
            TfLiteGpuDelegateOptionsV2 default_options = TfLiteGpuDelegateOptionsV2Default();
            default_options.max_delegated_partitions = 3;
            //default_options.inference_preference = TFLITE_GPU_INFERENCE_PREFERENCE_SUSTAINED_SPEED;
            default_options.inference_priority1 = TFLITE_GPU_INFERENCE_PRIORITY_MIN_LATENCY;
            delegate_ = TfLiteGpuDelegateV2Create(&default_options);
            TfLiteInterpreterOptionsAddDelegate(options_, delegate_);
            break;
        }
        default:
            TfLiteInterpreterOptionsSetNumThreads(options_, kThreadNum);
            break;
    }

    interpreter_ = TfLiteInterpreterCreate(model_, options_);
    if (!interpreter_) {
        LOGE("Failed to create TFLite interpreter");
        return;
    } else {
        LOGI("Interpreter Created");
    }

    int inputCount = TfLiteInterpreterGetInputTensorCount(interpreter_);
    LOGI("Input count %i", inputCount);
    int inputDims[4] = {1, kInputImageHeight, kInputImageWidth, kImageChannels};
    const int * CONST_DIMS = inputDims;
    LOGI("fk size %d", sizeof(CONST_DIMS));
    TfLiteStatus status = TfLiteInterpreterAllocateTensors(interpreter_);
    if (status != kTfLiteOk) {
        LOGE("Something went wrong when allocating tensors");
        return;
    } else {
        LOGI("tensors allocated for default size");
    }
    status = TfLiteInterpreterResizeInputTensor(interpreter_, 0, CONST_DIMS, 4);
    if (status != kTfLiteOk) {
        LOGE("Something went wrong when resizing tensors");
        return;
    } else {
        LOGI("tensors reszied correctly");
    }
    status = TfLiteInterpreterAllocateTensors(interpreter_);
    if (status != kTfLiteOk) {
        LOGE("Something went wrong when allocating tensors");
        return;
    } else {
        LOGI("tensors allocated for pref size");
    }
}

SuperResolution::~SuperResolution() {
    // Dispose of the model and interpreter objects
    if (interpreter_) {
    TfLiteInterpreterDelete(interpreter_);
    }
    if (delegate_) {
    //TfLiteGpuDelegateV2Delete(delegate_);
    }
    if (options_) {
    TfLiteInterpreterOptionsDelete(options_);
    }
    if (model_) {
    TfLiteModelDelete(model_);
    }
}

bool SuperResolution::IsInterpreterCreated() {
    if (!interpreter_) {
        return false;
    } else {
        return true;
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

int* SuperResolution::DoSuperResolution(uint8_t* lr_img_rgb) {

    TfLiteStatus status = TfLiteInterpreterAllocateTensors(interpreter_);
    if (status != kTfLiteOk) {
        LOGE("Something went wrong when allocating tensors");
        return nullptr;
    }

    int input = interpreter_->impl->inputs()[0];
    auto batch_size = interpreter_->impl->tensor(input)->dims->data[0];
    auto height = interpreter_->impl->tensor(input)->dims->data[1];
    auto width = interpreter_->impl->tensor(input)->dims->data[2];
    auto channels = interpreter_->impl->tensor(input)->dims->data[3];

    TfLiteTensor* input_tensor = TfLiteInterpreterGetInputTensor(interpreter_, input);

    int* lr_img_rgb_int = reinterpret_cast<int*>(lr_img_rgb);
    // Get Y (greyscale) from clear_color_yuv
    uint8_t * input_buffer = new uint8_t[kNumberOfInputPixels * kImageChannels];
    for (int i = 0, j = 0; i < kNumberOfInputPixels; i++) {
        input_buffer[j++] = static_cast<uint8_t>(( lr_img_rgb_int[i] >> 8) & 0xff);
    }

    // Feed input into model
    int64_t startTime = getTimeNsec();
    status = TfLiteTensorCopyFromBuffer(input_tensor, input_buffer,kInputImageHeight * kInputImageWidth * sizeof(uint8_t));
    if (status != kTfLiteOk) {
        LOGE("Something went wrong when copying input buffer to input tensor");
        return nullptr;
    }

    // Run the interpreter
    status = TfLiteInterpreterInvoke(interpreter_);
    if (status != kTfLiteOk) {
        LOGE("Something went wrong when running the TFLite model");
        return nullptr;
    }

    // Extract the output tensor data
    const TfLiteTensor* output_tensor =
      TfLiteInterpreterGetOutputTensor(interpreter_, 0);
    uint8_t* output_buffer = new uint8_t[kNumberOfOutputPixels * kImageChannels];
    status = TfLiteTensorCopyToBuffer(
      output_tensor, output_buffer,
      kOutputImageHeight * kOutputImageWidth * sizeof(uint8_t));
    if (status != kTfLiteOk) {
        LOGE("Something went wrong when copying output tensor to output buffer");
        return nullptr;
    }

    int64_t endTime = getTimeNsec() - startTime;
    LOGI("UPSCALE TIME: %f", static_cast<float>(endTime)/1000000);

    //delete input_buffer
    delete[] input_buffer;


    // Postprocess the output from TFLite
    int clipped_output[kImageChannels];
    int* rgb_colors = new int[kNumberOfOutputPixels];

    for (int i = 0; i < kNumberOfOutputPixels; i++) {
        for (int j = 0; j < kImageChannels; j++) {
          clipped_output[j] = std::max<float>(0, std::min<float>(255, output_buffer[i * kImageChannels + j]));
        }
        // When we have RGB values, we pack them into a single pixel.
        // Alpha is set to 255.
        rgb_colors[i] = (clipped_output[0] & 0xff) << 24 | (clipped_output[0] & 0xff) << 8 |
                        (clipped_output[0] & 0xff) |
                        (clipped_output[0] & 0xff) << 16;
        //LOGI("color value: %d", rgb_colors[i]);,
    }

    //delete output_buffer
    delete[] output_buffer;

    //LOGI("SuperResolution Time: %f ms", static_cast<float>(endTime)/1000000);

    return rgb_colors;
}

std::unique_ptr<int[]> SuperResolution::DoSuperResolution(AHardwareBuffer** hardwareBuffer) {


    ANeuralNetworksMemory* mem = nullptr;
    nnapi_object->ANeuralNetworksMemory_createFromAHardwareBuffer(*hardwareBuffer, &mem);

    struct MemoryContext {
        ANeuralNetworksMemory* memory_handle;
        void* memory_data;
        size_t byte_size;
    };

    MemoryContext memory_context = {mem, hardwareBuffer,0};

    StatefulNnApiDelegate::CopyToHostTensorFnPtr callback_CopyToHostTensorFnPtr =
        [](TfLiteTensor* tensor, ANeuralNetworksMemory* memory, size_t memory_offset, size_t byte_size, void* callback_context) -> TfLiteStatus {

        auto memory_context = reinterpret_cast<MemoryContext*>(callback_context);
        if (memory != memory_context->memory_handle || memory_offset + byte_size > memory_context->byte_size) {
        //    return kTfLiteError;
        }
        tensor->bytes = 0;
        //memory_context->memory_data


       // memcpy(tensor->data.raw, reinterpret_cast<uint8_t*>(memory_context->memory_data) + memory_offset,byte_size);
        return kTfLiteOk;
    };

    TfLiteBufferHandle bufferHandle = delegate->RegisterNnapiMemory(mem, callback_CopyToHostTensorFnPtr, &memory_context);

    // Create the interpreter
    interpreter_ = TfLiteInterpreterCreate(model_, options_);

    if (!interpreter_) {
        LOGE("Failed to create TFLite interpreter");
        return nullptr;
    }

    interpreter_->impl->SetBufferHandle(0, bufferHandle, delegate_); //index 0 = Input Tensor
    interpreter_->impl->tensor(0)->data_is_stale = true;
    TfLiteTensor* tensor = TfLiteInterpreterGetInputTensor(interpreter_, 0);

    TfLiteStatus status = TfLiteInterpreterInvoke(interpreter_);

    if (status != kTfLiteOk) {
        LOGE("Something went wrong when running the TFLite model");
        return nullptr;
    }

    // Extract the output tensor data
    const TfLiteTensor* output_tensor =
            TfLiteInterpreterGetOutputTensor(interpreter_, 0);
    float output_buffer[kNumberOfOutputPixels * kImageChannels];

    status = TfLiteTensorCopyToBuffer(
            output_tensor, output_buffer,
            kNumberOfOutputPixels * kImageChannels * sizeof(float));
    if (status != kTfLiteOk) {
        LOGE("Something went wrong when copying output tensor to output buffer");
        return nullptr;
    }
    //LOGI("Time: %ld", endTime);
    // Postprocess the output from TFLite
    int clipped_output;
    auto rgb_colors = std::make_unique<int[]>(kNumberOfOutputPixels);
    for (int i = 0; i < kNumberOfOutputPixels; i++) {
        clipped_output = std::max<float>(0, std::min<float>(255, output_buffer[i]));
        // When we have RGB values, we pack them into a single pixel.
        // Alpha is set to 255.
        rgb_colors[i] = (255u & 0xff) << 24 | (clipped_output & 0xff) << 16 |
                        (clipped_output & 0xff) << 8 |
                        (clipped_output & 0xff);
    }


    return rgb_colors;
}

}  // namespace superresolution
}  // namespace tflite
