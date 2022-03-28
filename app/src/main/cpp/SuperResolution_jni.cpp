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

#include <android/log.h>
#include <jni.h>

#include <cinttypes>
#include <cstring>
#include <string>


#include "SuperResolution.h"


extern "C" JNIEXPORT jlong
Java_com_android_gl2jni_GL2JNIActivity_initWithByteBufferFromJNI(
        JNIEnv *env, jobject thiz, jobject model_buffer, jboolean use_gpu) {
  const void *model_data = static_cast<void *>(env->GetDirectBufferAddress(model_buffer));
  jlong model_size_bytes = env->GetDirectBufferCapacity(model_buffer);
  tflite::superresolution::SuperResolution *super_resolution = new tflite::superresolution::SuperResolution(
          model_data, static_cast<size_t>(model_size_bytes), use_gpu);
  if (super_resolution->IsInterpreterCreated()) {
    LOGI("Interpreter is created successfully");

    return reinterpret_cast<jlong>(super_resolution);
  } else {
    delete super_resolution;
    return 0;
  }
}

extern "C" JNIEXPORT jintArray
Java_com_android_gl2jni_GL2JNIActivity_superResolutionFromJNI(JNIEnv *env, jobject thiz, jobject model_buffer, jintArray low_res_rgb) {

    const void *model_data = static_cast<void *>(env->GetDirectBufferAddress(model_buffer));
    jlong model_size_bytes = env->GetDirectBufferCapacity(model_buffer);

    jboolean jbool = JNI_FALSE;
    jint *lr_img_rgb = env->GetIntArrayElements(low_res_rgb, &jbool);

    tflite::superresolution::SuperResolution *super_resolution = new tflite::superresolution::SuperResolution(model_data, static_cast<size_t>(model_size_bytes), true);
    //TfLiteTensor* tensor = TfLiteInterpreterGetInputTensor(super_resolution->interpreter_,0);

    if (super_resolution->IsInterpreterCreated()) {
        LOGI("Interpreter is created successfully");
    }

    // Generate super resolution image
    int* lr_test = new int[360*640];
    int* sr_rgb_colors = super_resolution->DoSuperResolution2(lr_img_rgb);

    if(sr_rgb_colors == nullptr){
        return nullptr;
    }
    jintArray sr_img_rgb = env->NewIntArray(tflite::superresolution::kNumberOfOutputPixels);
    env->SetIntArrayRegion(sr_img_rgb, 0, tflite::superresolution::kNumberOfOutputPixels,
                        sr_rgb_colors);
    // Clean up before we return
    env->ReleaseIntArrayElements(low_res_rgb, lr_img_rgb, JNI_COMMIT);
    delete[] sr_rgb_colors;
    return sr_img_rgb;
}



extern "C" JNIEXPORT void JNICALL
Java_com_android_gl2jni_GL2JNIActivity_deinitFromJNI(JNIEnv *env, jobject thiz, jlong native_handle) {
  delete reinterpret_cast<tflite::superresolution::SuperResolution*>(native_handle);
}
