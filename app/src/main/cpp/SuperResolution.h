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
#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

namespace tflite {
namespace superresolution {

const int kInputImageWidth = 360;
const int kInputImageHeight = 640;
const int kImageChannels = 1;
const int kNumberOfInputPixels = kInputImageHeight * kInputImageWidth;
const int kUpscaleFactor = 3;
const int kOutputImageHeight = kInputImageHeight * kUpscaleFactor;
const int kOutputImageWidth = kInputImageWidth * kUpscaleFactor;
const int kNumberOfOutputPixels = kOutputImageHeight * kOutputImageWidth;

class SuperResolution {
  public:
    SuperResolution(const void* model_data, size_t model_size, int use_accelerator);
    ~SuperResolution();
    bool IsInterpreterCreated();

    int* DoSuperResolution(uint8_t* lr_img_rgb);
    std::unique_ptr<int[]> DoSuperResolution(AHardwareBuffer** hardwareBuffer);
    bool isResponding();

  private:
    TfLiteInterpreter* interpreter_;
    TfLiteModel* model_ = nullptr;
    TfLiteInterpreterOptions* options_ = nullptr;
    TfLiteDelegate* delegate_ = nullptr;


};

}  // namespace superresolution
}  // namespace tflite
#endif
