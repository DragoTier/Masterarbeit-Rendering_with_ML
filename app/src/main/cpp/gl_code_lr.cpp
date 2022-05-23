 #include <jni.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android/hardware_buffer.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <android/asset_manager.h>
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "gl_tools_lr.h"
#include "gl_texture_model.h"
#include "SuperResolution.h"
#include "Shader_lr.h"
#include "Assimp_Model.h"

#define  LOG_TAG    "libgles32_escpn"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

namespace GLCode{

long super_resolution_native_handle;
tflite::superresolution::SuperResolution* superRes = nullptr;

//clear color
float clear_color[] = {0.0f, 0.1f, 0.1f};
std::vector<GLfloat> clear_color_yuv;

//transform
float rotation = 0.0f;

GLuint shaderProgramLR;
GLuint vPositionHandleLR;
GLuint fbo;
GLuint texture;
GLuint depth_texture;

//global variables
uint8_t* gl_texture_lowRes = nullptr;
uint8_t* gl_texture_highRes = nullptr;
int width_lr = 0;
int height_lr = 0;

//Assimp model
AssimpModelLoader::Assimp_Model* model = nullptr;
GLuint noise_texture;


AHardwareBuffer* hardware_buffer = nullptr;
EGLImageKHR imageKHR;
EGLClientBuffer clientBuffer;
const GLfloat gTriangleVertices[] = { 0.0f, 0.5f, -0.5f, -0.5f,0.5f, -0.5f };

int64_t startTime;
int64_t endTime;

void RGBtoYUV(float r, float g, float b, std::vector<GLfloat> &yuv);

bool setupGraphics(int w, int h, const void* model_data, size_t model_size, const int* noise_texture_ptr) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    glEnable(GL_DEPTH_TEST);

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

    glGenTextures(1, &depth_texture);
    glBindTexture(GL_TEXTURE_2D, depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width_lr, height_lr, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);

    GLenum framebufferstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(framebufferstatus != GL_FRAMEBUFFER_COMPLETE){
        LOGE("FRAMEBUFFER NOT OK");
    }

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


    //prepare clear color
    RGBtoYUV(clear_color[0], clear_color[1], clear_color[2], clear_color_yuv);

    //assimp
    model = new AssimpModelLoader::Assimp_Model(model_data, model_size);

    glGenTextures(1, &noise_texture);
    glBindTexture(GL_TEXTURE_2D, noise_texture);
    checkGlError("glBindTexture");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Get Y (greyscale) from clear_color_yuv
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,350, 350, 0, GL_RGBA, GL_UNSIGNED_BYTE, noise_texture_ptr);
    checkGlError("glTexImage2D");

    glBindTexture(GL_TEXTURE_2D, 0);

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

    uint8_t* srImgInt = superRes->DoSuperResolution(pixels);
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
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(clear_color_yuv[0], clear_color_yuv[1], clear_color_yuv[2], 1.0f);
    checkGlError("glClearColor");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(shaderProgramLR);
    checkGlError("glUseProgram");

   /* glVertexAttribPointer(vPositionHandleLR, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(vPositionHandleLR);
    checkGlError("glEnableVertexAttribArray"); */
    rotation += 2.0f;
    glm ::mat4 mat_model = glm::mat4(1.0f);
    mat_model = glm::rotate(mat_model, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    mat_model = glm::translate(mat_model, glm::vec3(0.0f, -.7f, 0.0f));
    mat_model = glm::scale(mat_model, glm::vec3(4.0f));
    glm::mat4 mat_view = glm::mat4(1.0f);
    mat_view = glm::translate(mat_view, glm::vec3(0.0f, 0.0f, -3.0f));
    glm::mat4 mat_proj = glm::perspective(glm::radians(45.0f), (float)width_lr/(float)height_lr, 0.1f, 1000.0f);

    unsigned int transformLoc = glGetUniformLocation(shaderProgramLR, "model");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(mat_model));
    transformLoc = glGetUniformLocation(shaderProgramLR, "view");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(mat_view));
    transformLoc = glGetUniformLocation(shaderProgramLR, "projection");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(mat_proj));

    GLint loc = glGetUniformLocation(shaderProgramLR, "noiseTex");
    checkGlError("glGetUniformLocation");
    glUniform1i(loc, 0);
    checkGlError("glUniform1i");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, noise_texture);


    //GLenum buffers[]{ GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT};
    //glDrawBuffers(2, buffers);
    //glDrawArrays(GL_TRIANGLES, 0, 3);

    model->Draw(shaderProgramLR);
    checkGlError("glDrawArrays");
    glBindTexture(GL_TEXTURE_2D, 0);
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
    JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_init(JNIEnv * env, jclass obj, jint width, jint height, jobject model_buffer, jintArray noise_texture);
    JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_step(JNIEnv * env, jclass obj);
    JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_setNativeHandle(JNIEnv * env, jclass obj, jlong native_handle);
    JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_upscaleImg(JNIEnv* env, jclass obj);
};

extern "C" JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_init(JNIEnv * env, jclass obj, jint width, jint height, jobject model_buffer, jintArray noise_texture)
{
    const void *model_data = static_cast<void *>(env->GetDirectBufferAddress(model_buffer));
    jlong model_size_bytes = env->GetDirectBufferCapacity(model_buffer);
    jint* noise_texture_pointer = env->GetIntArrayElements(noise_texture, nullptr);
    setupGraphics(width, height, model_data, static_cast<size_t>(model_size_bytes), static_cast<int *>(noise_texture_pointer));
    env->ReleaseIntArrayElements(noise_texture, noise_texture_pointer, 0);
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



