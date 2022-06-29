#ifndef NATIVE_LIBS_SUPERRESOLUTION_H
#define NATIVE_LIBS_SUPERRESOLUTION_H

#include <string>

#include <tensorflow/lite/c/c_api.h>
#include <tensorflow/lite/delegates/nnapi/nnapi_delegate.h>
#include "tensorflow/lite/delegates/gpu/delegate.h"


#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define LOG_TAG "super_resolution::"
#define LOGINFO(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGERROR(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))


namespace espcn {

const int inputImageWidth = 360;
const int inputImageHeight = 640;
const int numberOfInputPixels = inputImageHeight * inputImageWidth;
const int imageChannels = 1;
const int upscaleFactor = 3;
const int outputImageHeight = inputImageHeight * upscaleFactor;
const int outputImageWidth = inputImageWidth * upscaleFactor;
const int numberOfOutputPixels = outputImageHeight * outputImageWidth;

class SuperResolution {
public:
	SuperResolution(const void *model_data, size_t model_size, int accelerator);

	~SuperResolution();

	bool IsInterpreterInitialized();

	uint8_t *DoSuperResolution(uint8_t *yuv_img);

	bool isResponding();

private:
	TfLiteInterpreter *interpreter_;
	TfLiteModel *model_ = nullptr;
	TfLiteInterpreterOptions *options_ = nullptr;
	TfLiteDelegate *delegate_ = nullptr;
};

}  // namespace espcn

#endif
