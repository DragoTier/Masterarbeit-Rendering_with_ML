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

namespace tflite {
namespace superresolution {

extern "C" JNIEXPORT jlong
Java_de_esolutions_masterthesis_aiupscale_MainActivity_initWithByteBufferFromJNI(JNIEnv *env, jobject thiz, jobject model_buffer, jint useAccelerator) {
  const void *model_data = static_cast<void *>(env->GetDirectBufferAddress(model_buffer));
  jlong model_size_bytes = env->GetDirectBufferCapacity(model_buffer);
  SuperResolution *super_resolution = new SuperResolution( model_data, static_cast<size_t>(model_size_bytes), useAccelerator);
  if (super_resolution->IsInterpreterCreated()) {
    LOGI("Interpreter is created successfully");

    return reinterpret_cast<jlong>(super_resolution);
  } else {
    delete super_resolution;
    return 0;
  }
}

extern "C" JNIEXPORT void JNICALL
Java_de_esolutions_masterthesis_aiupscale_MainActivity_deinitFromJNI(JNIEnv *env, jobject thiz, jlong native_handle) {
  delete reinterpret_cast<SuperResolution*>(native_handle);
}

}  // namespace superresolution
}  // namespace tflite
