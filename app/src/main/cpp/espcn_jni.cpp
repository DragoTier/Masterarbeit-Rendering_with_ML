#include <android/log.h>
#include <jni.h>
#include <cinttypes>
#include <cstring>
#include <string>
#include "SuperResolution.h"

namespace espcn {

extern "C" JNIEXPORT jlong
Java_de_esolutions_masterthesis_aiupscale_MainActivity_createSuperResolutionInstance(JNIEnv *env, jobject thiz, jobject model_buffer, jint accelerator) {
	const void *model_data = static_cast<void *>(env->GetDirectBufferAddress(model_buffer));
	jlong model_size = env->GetDirectBufferCapacity(model_buffer);

	SuperResolution *super_resolution = new SuperResolution(model_data,static_cast<size_t>(model_size),accelerator);
	if (super_resolution->IsInterpreterInitialized()) {
		return reinterpret_cast<jlong>(super_resolution);
	} else {
		delete super_resolution;
		return 0;
	}
}

extern "C" JNIEXPORT void JNICALL
Java_de_esolutions_masterthesis_aiupscale_MainActivity_deleteSuperResolutionInstance(JNIEnv *env, jobject thiz, jlong native_handle) {

	delete reinterpret_cast<SuperResolution *>(native_handle);
}

}  // namespace espcn

