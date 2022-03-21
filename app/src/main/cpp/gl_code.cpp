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


#include "SuperResolution.h"
//#include <glm/hlm.hpp>


#define  LOG_TAG    "libgl2jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

long super_resolution_native_handle;
std::unique_ptr<int[]> srImg;
uint8_t width;
uint8_t height;

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
    }
}

auto gVertexShader = R"(#version 320 es
    in vec4 vPosition;
    void main() {
      gl_Position = vec4(vPosition.x, 0.0f - vPosition.y, vPosition.z, vPosition.w);
    }
    )";

auto gFragmentShader = R"(#version 320 es
    precision mediump float;
    out vec4 fragColor;
    //out vec4 fragColor[2];

    vec3 YUVfromRGB(vec3 color){
        color = color * 255.0f;
        float y =  0.257f * color.r + 0.504f * color.g + 0.098f * color.b + 160.0f;
        float u = -0.148f * color.r - 0.291f * color.g + 0.439f * color.b + 1280.0f;
        float v = 0.439f * color.r - 0.368f * color.g - 0.071f * color.b + 128.0f;
        return vec3(y, u, v) * (1.0f/255.0f);
    }

    void main() {
       vec3 color = vec3(1.0f, 1.0f, 0.0f);

        vec3 yuv = YUVfromRGB(color);
        fragColor = vec4(yuv, 1.0f);
        //fragColor = vec4(color, 1.0f);
    }
    )";

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    LOGE("Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
                glDeleteShader(shader);
                shader = 0;
            }
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    LOGE("Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint gProgram;
GLuint gvPositionHandle;

GLuint fbo;
GLuint texture;
AHardwareBuffer* hardware_buffer = nullptr;
EGLImageKHR imageKHR;
EGLClientBuffer clientBuffer;

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
    gProgram = createProgram(gVertexShader, gFragmentShader);
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

const GLfloat gTriangleVertices[] = { 0.0f, 0.5f, -0.5f, -0.5f,
        0.5f, -0.5f };

int64_t startTime;
int64_t endTime;

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

std::unique_ptr<int[]> doSuperResolution(){
    if (!reinterpret_cast<tflite::examples::superresolution::SuperResolution *>(super_resolution_native_handle)
            ->IsInterpreterCreated()) {
        return nullptr;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    uint8_t pixels[width * height * 4];

    glReadPixels(0, 0, width, height, GL_RGBA,  GL_UNSIGNED_BYTE, pixels);

    endTime = getTimeNsec() - startTime;
    LOGI("Render + Readback Time: %ld", endTime);

    GLenum readPixelsError = glGetError();
    FILE* pFile;
    pFile = fopen("/data/data/com.android.gl2jni/buffer_file_2.data", "wb");
    fwrite(pixels, 1, width*height*4, pFile);
    fclose(pFile);

    LOGI("SUPERRESOLUTION ABOUT TO START");
    LOGI("SuperRes native handle: %dl", super_resolution_native_handle);
    tflite::examples::superresolution::SuperResolution* superRes = reinterpret_cast<tflite::examples::superresolution::SuperResolution*> (super_resolution_native_handle);
    if(superRes == nullptr){
        LOGE("super res handle is nullptr");
    }
    std::unique_ptr<int[]> srImgInt = superRes->DoSuperResolution((int*)pixels);
    //std::unique_ptr<int[]> srImgInt = reinterpret_cast<tflite::examples::superresolution::SuperResolution*> (super_resolution_native_handle)
    //       ->DoSuperResolution((int*)pixels);
   // std::unique_ptr<int[]> srImgInt = reinterpret_cast<tflite::examples::superresolution::SuperResolution*> (super_resolution_native_handle)
   //       ->DoSuperResolutionCNN(pixels);

    FILE* pFile2;
    pFile2 = fopen("/data/data/com.android.gl2jni/buffer_file_3.data", "wb");
    fwrite(&srImgInt[0], 1, 200*200*4, pFile2);
    fclose(pFile2);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return srImgInt;
}

std::unique_ptr<int[]> doSuperResolutionHardwareBuffer(){
    if (!reinterpret_cast<tflite::examples::superresolution::SuperResolution *>(super_resolution_native_handle)
            ->IsInterpreterCreated()) {
        return nullptr;
    }

    // Generate super resolution image
    auto srImgyo =  reinterpret_cast<tflite::examples::superresolution::SuperResolution *>(super_resolution_native_handle)
            ->DoSuperResolution(&hardware_buffer);

    return srImgyo;
}



void renderFrame() {

    startTime = getTimeNsec();

    static float grey;
    grey = 0.01f;
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

    //GLenum buffers[]{ GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT};
    //glDrawBuffers(2, buffers);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    checkGlError("glDrawArrays");

}

extern "C" {
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv * env, jclass obj,  jint width, jint height);
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_step(JNIEnv * env, jclass obj);
    JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_setNativeHandle(JNIEnv * env,jclass obj, jlong native_handle);
    JNIEXPORT jintArray JNICALL Java_com_android_gl2jni_GL2JNILib_getSuperResImg(JNIEnv* env, jclass obj);
};

extern "C" JNIEXPORT void JNICALL Java_com_android_gl2jni_GL2JNILib_init(JNIEnv * env, jclass obj,  jint width, jint height)
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
}

extern "C" JNIEXPORT jintArray JNICALL Java_com_android_gl2jni_GL2JNILib_getSuperResImg(JNIEnv* env, jclass obj){
    auto img = doSuperResolution();
    jintArray sr_img_rgb = env->NewIntArray(107*3*160 * 3);
    env->SetIntArrayRegion(sr_img_rgb, 0, 107*3*160 * 3,
                           img.get());

    return sr_img_rgb;
}



