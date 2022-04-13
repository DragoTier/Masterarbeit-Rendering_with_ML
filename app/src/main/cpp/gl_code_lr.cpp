#include <jni.h>
#include <android/log.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

//#include <android/hardware_buffer_jni.h>
#include <android/hardware_buffer.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <android/asset_manager.h>
#include <GLES3/gl31.h>
#include <GLES3/gl3ext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "gl_tools_lr.h"
#include "gl_texture_model.h"
#include "SuperResolution.h"
#include "Shader_lr.h"

#define  LOG_TAG    "libgles32_escpn"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

namespace GLCode{

long super_resolution_native_handle;
tflite::superresolution::SuperResolution* superRes = nullptr;

//clear color
float clear_color[] = {0.1f, 0.1f, 0.1f};
std::vector<GLfloat> clear_color_yuv;

//transform
float rotation = 0.0f;

GLuint shaderProgramLR;
GLuint vPositionHandleLR;
GLuint fbo;
GLuint texture;

//global variables
uint8_t* gl_texture_lowRes = nullptr;
int* gl_texture_highRes = nullptr;
int width_lr = 0;
int height_lr = 0;


AHardwareBuffer* hardware_buffer = nullptr;
EGLImageKHR imageKHR;
EGLClientBuffer clientBuffer;
const GLfloat gTriangleVertices[] = { 0.0f, 0.5f, -0.5f, -0.5f,0.5f, -0.5f };

int64_t startTime;
int64_t endTime;

void RGBtoYUV(float r, float g, float b, std::vector<GLfloat> &yuv);

bool setupGraphics(int w, int h) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    width_lr = w;
    height_lr = h;

    AHardwareBuffer_Desc buffer{};
    buffer.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
    buffer.height = height_lr;
    buffer.width = width_lr;
    buffer.layers = 1;
    //buffer.usage = 0UL; //read never
    buffer.stride = 10;
    buffer.usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_NEVER
                   | AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT;

    int supported = AHardwareBuffer_isSupported(&buffer);

    AHardwareBuffer_allocate(&buffer, &hardware_buffer);

    PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC eglGetNativeClientBufferAndroid = (PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC) eglGetProcAddress("eglGetNativeClientBufferANDROID");
    clientBuffer = eglGetNativeClientBufferAndroid(hardware_buffer);

    EGLDisplay disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint eglImageAttributes[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};

    PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");
    imageKHR = eglCreateImageKHR(disp, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, eglImageAttributes);


    glGenTextures(1, &texture);
    GLenum errorreturn = glGetError();
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)  eglGetProcAddress("glEGLImageTargetTexture2DOES");
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, imageKHR);


    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    GLenum framebufferstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D,0);


    LOGI("setupGraphics(%d, %d)", w, h);
    shaderProgramLR = createProgram(vertexShaderLR, fragmentShaderLR);
    if (!shaderProgramLR) {
        LOGE("Could not create program.");
        return false;
    }
    vPositionHandleLR = glGetAttribLocation(shaderProgramLR, "vPosition");
    checkGlError("glGetAttribLocation");
    LOGI("glGetAttribLocation(\"vPosition\") = %d\n",
         vPositionHandleLR);

    glViewport(0, 0, width_lr, height_lr);
    checkGlError("glViewport");

    RGBtoYUV(clear_color[0], clear_color[1], clear_color[2], clear_color_yuv);

    return true;
}

void doSuperResolution(){
    if (!superRes->IsInterpreterCreated()) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    uint8_t* pixels = new uint8_t[width_lr * height_lr * 4];
    glReadPixels(0, 0, width_lr, height_lr, GL_RGBA,  GL_UNSIGNED_BYTE, pixels);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int* srImgInt = superRes->DoSuperResolution(pixels);
    LOGI("did superresolution");
    gl_texture_lowRes = pixels;
    gl_texture_highRes = srImgInt;


    endTime = getTimeNsec() - startTime;
    LOGI("Render + Readback + Upscale Time: %f ms", static_cast<float>(endTime)/1000000);
    //srImgInt;
}

std::unique_ptr<int[]> doSuperResolutionHardwareBuffer(){
    if (!reinterpret_cast<tflite::superresolution::SuperResolution *>(super_resolution_native_handle)
            ->IsInterpreterCreated()) {
        return nullptr;
    }

    // Generate super resolution image
    auto srImgyo =  reinterpret_cast<tflite::superresolution::SuperResolution *>(super_resolution_native_handle)
            ->DoSuperResolution(&hardware_buffer);

    return srImgyo;
}



void renderFrame() {

    startTime = getTimeNsec();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glClearColor(clear_color_yuv[0], clear_color_yuv[1], clear_color_yuv[2], 1.0f);
    checkGlError("glClearColor");
    glClear(  GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(shaderProgramLR);
    checkGlError("glUseProgram");

    glVertexAttribPointer(vPositionHandleLR, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(vPositionHandleLR);
    checkGlError("glEnableVertexAttribArray");
    rotation += 10.0f;
    glm ::mat4 trans = glm::mat4(1.0f);
    trans = glm::rotate(trans, glm::radians(rotation), glm::vec3(0.0, 0.0, 1.0));

    unsigned int transformLoc = glGetUniformLocation(shaderProgramLR, "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));
    //GLenum buffers[]{ GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT};
    //glDrawBuffers(2, buffers);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    checkGlError("glDrawArrays");

}

void RGBtoYUV(float r, float g, float b, std::vector<GLfloat> &yuv){
    r *= 255.0f;
    g *= 255.0f;
    b *= 255.0f;
    float y =   (0.257f * r + 0.504f * g + 0.098f * b + 16.0f)  * (1.0f/255.0f);
    float u =  (-0.148f * r - 0.291f * g + 0.439f * b + 128.0f) * (1.0f/255.0f);
    float v =   (0.439f * r - 0.368f * g - 0.071f * b + 128.0f) * (1.0f/255.0f);
    yuv.push_back(y);
    yuv.push_back(u);
    yuv.push_back(v);
}

extern "C" {
    JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_init(JNIEnv * env, jclass obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_step(JNIEnv * env, jclass obj);
    JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_setNativeHandle(JNIEnv * env, jclass obj, jlong native_handle);
    JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_upscaleImg(JNIEnv* env, jclass obj);
};

extern "C" JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_init(JNIEnv * env, jclass obj, jint width, jint height)
{
    setupGraphics(width, height);
}

extern "C" JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_step(JNIEnv * env, jclass obj)
{
    renderFrame();
}

extern "C" JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_setNativeHandle(JNIEnv * env, jclass obj, jlong native_handle){

    super_resolution_native_handle = native_handle;
    superRes = reinterpret_cast<tflite::superresolution::SuperResolution*> (super_resolution_native_handle);

    if(superRes == nullptr){
        LOGE("super res handle is nullptr");
    }
    if(superRes->isResponding()){
        LOGI("IS RESPONDING");
    }
}

extern "C" JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_upscaleImg(JNIEnv* env, jclass obj){
    doSuperResolution();
}


} //namespace



