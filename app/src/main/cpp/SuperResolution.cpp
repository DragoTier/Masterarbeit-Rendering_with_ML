/*
 * Copyright 2020 The TensorFlow Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
namespace examples {
namespace superresolution {

constexpr int kThreadNum = 4;
const NnApi* nnapi_object;
StatefulNnApiDelegate* delegate;

SuperResolution::SuperResolution(const void* model_data, size_t model_size,
                                 bool use_nnapi) {
    // Load the model
    //model_ = null;
    model_ = TfLiteModelCreate(model_data, model_size);
    if (!model_) {
        LOGE("Failed to create TFLite model");
    return;
    }

    // Create the interpreter options
    TfLiteInterpreterOptions* options = TfLiteInterpreterOptionsCreate();
    //options->use_nnapi = true;
    options_ = options;

    // add nnapi delegate //Add GPU Delegate
    if (true) {
        nnapi_object = NnApiImplementation();

        StatefulNnApiDelegate::Options nnapi_options = StatefulNnApiDelegate::Options();
        //nnapi_options.accelerator_name = "google-armnn";
        nnapi_options.disallow_nnapi_cpu = true;
        delegate_ = new StatefulNnApiDelegate(nnapi_object, nnapi_options);
        //TfLiteGpuDelegateOptionsV2 default_options = TfLiteGpuDelegateOptionsV2Default();
        //delegate_ = TfLiteGpuDelegateV2Create(&default_options);

        TfLiteInterpreterOptionsAddDelegate(options_, delegate_);
    } else {
       TfLiteInterpreterOptionsSetNumThreads(options_, kThreadNum);
    }

    interpreter_ = TfLiteInterpreterCreate(model_, options_);
    if (!interpreter_) {
        LOGE("Failed to create TFLite interpreter");
        return;
    }

    int inputCount = TfLiteInterpreterGetInputTensorCount(interpreter_);
    LOGI("Input count %i", inputCount);
    int inputDims[4] = {1, kInputImageHeight, kInputImageWidth, kImageChannels};
    //memset(inputDims, 0, sizeof(float)* kNumberOfInputPixels);
    const int * CONST_DIMS = inputDims;
    LOGI("fk size %d", sizeof(CONST_DIMS));
    TfLiteStatus status = TfLiteInterpreterAllocateTensors(interpreter_);
    if (status != kTfLiteOk) {
        LOGE("Something went wrong when allocating tensors");
        return;
    }
    TfLiteInterpreterResizeInputTensor(interpreter_, 0, CONST_DIMS, 4);
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
int counter = 0;

std::unique_ptr<int[]> SuperResolution::DoSuperResolution(int* lr_img_rgb) {

    LOGI("SUPERRESOLUTION METHOD STARTED");
    counter++;

    // Allocate tensors and populate the input tensor data
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



    TfLiteTensor* input_tensor =
      TfLiteInterpreterGetInputTensor(interpreter_, input);


    // Extract RGB values from each pixel
    float input_buffer[kNumberOfInputPixels * kImageChannels];
    for (int i = 0, j = 0; i < kNumberOfInputPixels; i++) {
    // Alpha is ignored
        input_buffer[j++] = static_cast<float>((lr_img_rgb[i] >> 16) & 0xff);
        //input_buffer[j++] = static_cast<float>((lr_img_rgb[i] >> 16) & 0xff);
        //input_buffer[j++] = static_cast<float>((lr_img_rgb[i] >> 8) & 0xff);
       // LOGI("Input value: %f", input_buffer[j]);
    }
    LOGI("Length of input buffer: %d", (sizeof(input_buffer)/sizeof(*input_buffer)));


    // Feed input into model
    //status = TfLiteTensorCopyFromBuffer(input_tensor, input_buffer, kNumberOfInputPixels * kImageChannels * sizeof(float));

    status = TfLiteTensorCopyFromBuffer(
            input_tensor, input_buffer,kInputImageHeight * kInputImageWidth * sizeof(float));
    if (status != kTfLiteOk) {
        LOGE("Something went wrong when copying input buffer to input tensor");
        return nullptr;
    }
    int64_t startTime = getTimeNsec();
    // Run the interpreter
    status = TfLiteInterpreterInvoke(interpreter_);
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
      kOutputImageHeight * kOutputImageWidth * sizeof(float));
    if (status != kTfLiteOk) {
    LOGE("Something went wrong when copying output tensor to output buffer");
    return nullptr;
    }

    int64_t endTime = getTimeNsec() - startTime;

    // Postprocess the output from TFLite
    int clipped_output[kImageChannels];
    auto rgb_colors = std::make_unique<int[]>(kNumberOfOutputPixels);

    for (int i = 0; i < kNumberOfOutputPixels; i++) {
        for (int j = 0; j < kImageChannels; j++) {
          clipped_output[j] = std::max<float>(0, std::min<float>(255, output_buffer[i * kImageChannels + j]));
        }
        // When we have RGB values, we pack them into a single pixel.
        // Alpha is set to 255.
        rgb_colors[i] = (255u & 0xff) << 24 | (clipped_output[0] & 0xff) << 8 |
                        (clipped_output[0] & 0xff) |
                        (clipped_output[0] & 0xff) << 16;
        //LOGI("color value: %d", rgb_colors[i]);
    }

    LOGI("Invoked count: %i", counter);
    LOGI("SuperResolution Time: %f ms", static_cast<float>(endTime)/1000000);

    return rgb_colors;
}

    std::unique_ptr<int[]> SuperResolution::DoSuperResolutionCNN(float* lr_img_r) {


        counter++;
        // Allocate tensors and populate the input tensor data
        TfLiteStatus status = TfLiteInterpreterAllocateTensors(interpreter_);
        if (status != kTfLiteOk) {
            LOGE("Something went wrong when allocating tensors");
            return nullptr;
        }

        TfLiteTensor* input_tensor =
                TfLiteInterpreterGetInputTensor(interpreter_, 0);


        // Feed input into model
        status = TfLiteTensorCopyFromBuffer(
                input_tensor, lr_img_r,
                224*244 * sizeof(float));
        if (status != kTfLiteOk) {
            LOGE("Something went wrong when copying input buffer to input tensor");
            return nullptr;
        }
        int64_t startTime = getTimeNsec();
        // Run the interpreter
        status = TfLiteInterpreterInvoke(interpreter_);
        if (status != kTfLiteOk) {
            LOGE("Something went wrong when running the TFLite model");
            return nullptr;
        }

        // Extract the output tensor data
        const TfLiteTensor* output_tensor =
                TfLiteInterpreterGetOutputTensor(interpreter_, 0);
        float output_buffer[672*672];
        status = TfLiteTensorCopyToBuffer(
                output_tensor, output_buffer,
                672*672 * sizeof(float));
        if (status != kTfLiteOk) {
            LOGE("Something went wrong when copying output tensor to output buffer");
            return nullptr;
        }
        int64_t endTime = getTimeNsec() - startTime;

        // Postprocess the output from TFLite
      /*  int clipped_output[kImageChannels];
        auto rgb_colors = std::make_unique<int[]>(kNumberOfOutputPixels);
        for (int i = 0; i < kNumberOfOutputPixels; i++) {
            for (int j = 0; j < kImageChannels; j++) {
                clipped_output[j] = std::max<float>(
                        0, std::min<float>(255, output_buffer[i * kImageChannels + j]));
            }
            // When we have RGB values, we pack them into a single pixel.
            // Alpha is set to 255.
            rgb_colors[i] = (255u & 0xff) << 24 | (clipped_output[0] & 0xff) << 8 |
                            (clipped_output[1] & 0xff) |
                            (clipped_output[2] & 0xff) << 16;
        } */

        LOGI("Invoked count: %i", counter);
        LOGI("SuperResolution Time: %ld", endTime);

        return nullptr;
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
    int clipped_output[kImageChannels];
    auto rgb_colors = std::make_unique<int[]>(kNumberOfOutputPixels);
    for (int i = 0; i < kNumberOfOutputPixels; i++) {
        for (int j = 0; j < kImageChannels; j++) {
            clipped_output[j] = std::max<float>(
                    0, std::min<float>(255, output_buffer[i * kImageChannels + j]));
        }
        // When we have RGB values, we pack them into a single pixel.
        // Alpha is set to 255.
        rgb_colors[i] = (255u & 0xff) << 24 | (clipped_output[0] & 0xff) << 16 |
                        (clipped_output[1] & 0xff) << 8 |
                        (clipped_output[2] & 0xff);
    }


    return rgb_colors;
}

}  // namespace superresolution
}  // namespace examples
}  // namespace tflite
