/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// OpenGL ES 2.0 code

#include <jni.h>
#include <android/log.h>

//#include <GLES2/gl2.h>
//#include <GLES2/gl2ext.h>
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

#include "gl_tools.h"
#include "SuperResolution.h"
#include "Shader.h"

#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

namespace GLCode {

long super_resolution_native_handle;
tflite::superresolution::SuperResolution* superRes = nullptr;
std::unique_ptr<int[]> srImg = nullptr;
int width;
int height;
float rotation = 0.0f;

GLuint gProgram;
GLuint gvPositionHandle;

GLuint fbo;
GLuint texture;
GLuint highResTex;
AHardwareBuffer* hardware_buffer = nullptr;
EGLImageKHR imageKHR;
EGLClientBuffer clientBuffer;
const GLfloat gTriangleVertices[] = { 0.0f, 0.5f, -0.5f, -0.5f,0.5f, -0.5f };


                                    //pos               //tex coords
const float hr_screen_vertices[] = {1.0f, 1.0f, 0.0f,   1.0f, 1.0f,
                                    1.0f,-1.0f, 0.0f,   1.0f, 0.0f,
                                   -1.0f,-1.0f, 0.0f,   0.0f, 0.0f,
                                   -1.0f, 1.0f, 0.0f,   0.0f, 1.0f};

GLuint indices[] = {
        0, 1, 3,
        1, 2, 3
};
GLuint VBO, VAO, EBO;


int64_t startTime;
int64_t endTime;





bool setupGraphics(int w, int h) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    width = w;
    height = h;

    AHardwareBuffer_Desc buffer{};
    buffer.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
    buffer.height = h;
    buffer.width = w;
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
    gProgram = createProgram(vertexShaderLR, fragmentShaderLR);
    if (!gProgram) {
        LOGE("Could not create program.");
        return false;
    }
    gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
    checkGlError("glGetAttribLocation");
    LOGI("glGetAttribLocation(\"vPosition\") = %d\n",
            gvPositionHandle);

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    return true;
}


int64_t getTimeNsec() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t) now.tv_sec*1000000000LL + now.tv_nsec;
}

void dumpBuffer(){
    uint8_t* virtualAdress = nullptr;
    AHardwareBuffer_lock(hardware_buffer, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, nullptr, (void**) &virtualAdress);

    FILE* pFile;
    pFile = fopen("/data/data/com.android.gl2jni/buffer_file.data", "wb");
    fwrite(virtualAdress, 1, 50*50*4, pFile);
    fclose(pFile);

    AHardwareBuffer_unlock(hardware_buffer, nullptr);
}

void renderHighResFrame() {
    static float grey;
    grey = 1.0f;
    if (grey > 1.0f) {
        grey = 0.0f;
    }
    glClearColor(grey, grey, grey, 1.0f);
    checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(gProgram);
    checkGlError("glUseProgram");

    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");

    //glVertexAttribPointer(1,2, GL_FLOAT, GL_FALSE, 0, )
    //GLenum buffers[]{ GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT};
    //glDrawBuffers(2, buffers);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    checkGlError("glDrawArrays");
}

int* doSuperResolution(){
    if (!superRes->IsInterpreterCreated()) {
        return nullptr;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    uint8_t* pixels = new uint8_t[width * height * 4];
    glReadPixels(0, 0, width, height, GL_RGBA,  GL_UNSIGNED_BYTE, pixels);

    GLenum readPixelsError = glGetError();

    int* srImgInt = superRes->DoSuperResolution(pixels);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, width*3, height*3);

    glGenTextures(1,&highResTex);
    glBindTexture(GL_TEXTURE_2D, highResTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,tflite::superresolution::kOutputImageWidth, tflite::superresolution::kOutputImageHeight, 0, GL_RGB, GL_INT, srImgInt);
    renderHighResFrame();
    glViewport(0, 0, width, height);

    endTime = getTimeNsec() - startTime;
    LOGI("Render + Readback + Upscale Time: %f ms", static_cast<float>(endTime)/1000000);
    return srImgInt;
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

    static float grey;
    grey = 1.0f;
    if (grey > 1.0f) {
        grey = 0.0f;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor(grey, grey, grey, 1.0f);
    checkGlError("glClearColor");
    glClear(  GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");


    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(gProgram);
    checkGlError("glUseProgram");

    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");
    rotation += 10.0f;
    glm ::mat4 trans = glm::mat4(1.0f);
    trans = glm::rotate(trans, glm::radians(rotation), glm::vec3(0.0, 0.0, 1.0));

    unsigned int transformLoc = glGetUniformLocation(gProgram, "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));
    //GLenum buffers[]{ GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT};
    //glDrawBuffers(2, buffers);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    checkGlError("glDrawArrays");

}

extern "C" {
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv * env, jclass obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv * env, jclass obj);
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_setNativeHandle(JNIEnv * env, jclass obj, jlong native_handle);
    JNIEXPORT jintArray JNICALL Java_com_android_gl2jni_GL2JNILib_getSuperResImg(JNIEnv* env, jclass obj);
};

extern "C" JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv * env, jclass obj, jint width, jint height)
{
    setupGraphics(width, height);
    renderFrame();
}

extern "C" JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv * env, jclass obj)
{
    renderFrame();
}

extern "C" JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_setNativeHandle(JNIEnv * env, jclass obj, jlong native_handle){

    super_resolution_native_handle = native_handle;
    superRes = reinterpret_cast<tflite::superresolution::SuperResolution*> (super_resolution_native_handle);

    if(superRes == nullptr){
        LOGE("super res handle is nullptr");
    }
    if(superRes->isResponding()){
        LOGI("IS RESPONDING");
    }
}

extern "C" JNIEXPORT jintArray JNICALL Java_com_android_gl2jni_GL2JNILib_getSuperResImg(JNIEnv* env, jclass obj){
    int* img = doSuperResolution();
    jintArray sr_img_rgb = env->NewIntArray(tflite::superresolution::kNumberOfOutputPixels);
    env->SetIntArrayRegion(sr_img_rgb, 0, tflite::superresolution::kNumberOfOutputPixels, img);
    delete[] img;
    return sr_img_rgb;
}


} //namespace

/* FILE* pFile2;
 pFile2 = fopen("/data/data/com.android.gl2jni/buffer_file_3.data", "wb");
 fwrite(&srImgInt[0], 1, 200*200*4, pFile2);
 fclose(pFile2); */

/*  FILE* pFile;
  pFile = fopen("/data/data/com.android.gl2jni/buffer_file_2.data", "wb");
  fwrite(pixels, 1, width*height*4, pFile);
  fclose(pFile); */

