
#ifndef HELLO_GL2_GL_TOOLS_HR_H
#define HELLO_GL2_GL_TOOLS_HR_H

#include <ctime>

#define  LOG_TAG    "libgles32_escpn"
#define  LOGINFO(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGERROR(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

namespace GLTools_HR{

void dumpBuffer(AHardwareBuffer &hardware_buffer){
    uint8_t* virtualAdress = nullptr;
    AHardwareBuffer_lock(&hardware_buffer, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, nullptr, (void**) &virtualAdress);

    FILE* pFile;
    pFile = fopen("/data/data/com.android.gl2jni/buffer_file.data", "wb");
    fwrite(virtualAdress, 1, 50*50*4, pFile);
    fclose(pFile);

    AHardwareBuffer_unlock(&hardware_buffer, nullptr);
}

GLuint loadShader(GLenum shaderType, const char* pSource) {

    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &pSource, NULL);
    glCompileShader(shader);
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        LOGERROR("Could not compile shader %d:\n", shaderType);
        glDeleteShader(shader);
    }

    return shader;
}

static void checkGlError(const char* op) {
    GLint error = glGetError();
    if(error != 0){
        LOGINFO("after %s() glError (0x%x)\n", op, error);
    }
}

GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fragmentShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    checkGlError("glAttachShader");
    glAttachShader(program, fragmentShader);
    checkGlError("glAttachShader");
    glLinkProgram(program);
    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        LOGERROR("Could not link program:\n");
        glDeleteProgram(program);
    }
    return program;
}

int64_t getTimeNsec() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t) now.tv_sec*1000000000LL + now.tv_nsec;
}



} //namespace
#endif //HELLO_GL2_GL_TOOLS_HR_H
