#include <jni.h>

#include <jni.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android/hardware_buffer.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <android/asset_manager.h>
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "gl_tools_hr.h"
#include "SuperResolution.h"
#include "Shader_hr.h"
#include "gl_texture_model.h"


namespace GLCodeHR{
jint width;
jint height;
GLuint shaderProgramHR;
GLuint fbo;
GLuint texture;
GLuint lowResTex;
GLuint highResTex;

                                    //pos               //tex coords
const float hr_screen_vertices[] = { 1.0f, 1.0f, 0.0f,   1.0f, 1.0f,
                                     1.0f,-1.0f, 0.0f,   1.0f, 0.0f,
                                    -1.0f,-1.0f, 0.0f,   0.0f, 0.0f,
                                    -1.0f, 1.0f, 0.0f,   0.0f, 1.0f};

GLuint indices[] = {
        0, 1, 3,
        1, 2, 3
};
GLuint VBO, VAO, EBO;

uint8_t* gl_texture_lowRes_test;
int* gl_texture_highRes_test;

bool createTextures();

bool setupGraphicsHR(jint w, jint h) {
    width = w;
    height = h;

    //highres setup
    shaderProgramHR = GLTools_HR::createProgram(vertexShaderHR, fragmentShaderHR);
    if (!shaderProgramHR) {
        LOGE("Could not create program.");
        return false;
    }
    glGenTextures(1, &lowResTex);
    glGenTextures(1, &highResTex);
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(hr_screen_vertices), hr_screen_vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    GLTools_HR::checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(0);
    GLTools_HR::checkGlError("glVertexAttribArray");

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) (3 * sizeof(float)));
    GLTools_HR::checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(1);
    GLTools_HR::checkGlError("glVertexAttribArray");

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glViewport(0, 0, w, h);
    GLTools_HR::checkGlError("glViewport");
    return true;
}

bool createTextures() {
    if(GLCode::gl_texture_lowRes == nullptr || GLCode::gl_texture_highRes == nullptr)
        return false;

    glBindTexture(GL_TEXTURE_2D, lowResTex);
    GLTools_HR::checkGlError("glBindTexture");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, GLCode::width_lr, GLCode::height_lr, 0, GL_RGBA, GL_UNSIGNED_BYTE, GLCode::gl_texture_lowRes);
    GLTools_HR::checkGlError("glTexImage2D");

    glBindTexture(GL_TEXTURE_2D, highResTex);
    GLTools_HR::checkGlError("glBindTexture");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,tflite::superresolution::kOutputImageWidth, tflite::superresolution::kOutputImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, GLCode::gl_texture_highRes);
    GLTools_HR::checkGlError("glTexImage2D");


    if(GLCode::gl_texture_lowRes != nullptr) {
        delete[] GLCode::gl_texture_lowRes;
        GLCode::gl_texture_lowRes = nullptr;
    }
    if(GLCode::gl_texture_highRes != nullptr){
        delete[] GLCode::gl_texture_highRes;
        GLCode::gl_texture_highRes = nullptr;
    }

    return true;
}

void renderFrameHR() {
    static float grey;
    grey = 0.3f;
    glClearColor(grey, grey, grey, 1.0f);
    GLTools_HR::checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    GLTools_HR::checkGlError("glClear");

    if(!createTextures())
        return;

    glUseProgram(shaderProgramHR);
    GLTools_HR::checkGlError("glUseProgram");

    glBindVertexArray(VAO);
    GLTools_HR::checkGlError("glBindVertexArray");

    GLint loc = glGetUniformLocation(shaderProgramHR, "lowResTex");
    glUniform1i(loc, 0);
    GLTools_HR::checkGlError("glUniform1i");

    glUniform1i(glGetUniformLocation(shaderProgramHR, "highResTex"), 1);
    GLTools_HR::checkGlError("glUniform1i");

    glActiveTexture(GL_TEXTURE0);
    GLTools_HR::checkGlError("glActiveTexture");
    glBindTexture(GL_TEXTURE_2D, lowResTex);
    GLTools_HR::checkGlError("glBindTexture");
    glActiveTexture(GL_TEXTURE1);
    GLTools_HR::checkGlError("glActiveTexture");
    glBindTexture(GL_TEXTURE_2D, highResTex);
    GLTools_HR::checkGlError("glBindTexture");

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    GLTools_HR::checkGlError("glDrawElements");
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);

}



extern "C" {
JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_initHR(JNIEnv * env, jclass obj, jint width, jint height);
JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_stepHR(JNIEnv * env, jclass obj);
};

extern "C" JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_initHR(JNIEnv * env, jclass obj, jint width, jint height) {
setupGraphicsHR(width, height);
}

extern "C" JNIEXPORT void JNICALL Java_de_esolutions_masterthesis_aiupscale_GLES32JNILib_stepHR(JNIEnv * env, jclass obj) {
renderFrameHR();
}


} //Namespace


