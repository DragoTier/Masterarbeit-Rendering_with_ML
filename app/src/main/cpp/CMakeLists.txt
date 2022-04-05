cmake_minimum_required(VERSION 3.18)

project("masterarbeit_tf")
# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14 -Wall")
set(CMAKE_CXX_STANDARD 14)

find_library(log-lib log)

#tensorflow
set(TENSORFLOW_SOURCE_DIR "" CACHE PATH
        "Directory that contains the TensorFlow project" )
if(NOT TENSORFLOW_SOURCE_DIR)
        get_filename_component(TENSORFLOW_SOURCE_DIR
                "${CMAKE_CURRENT_LIST_DIR}/../../../../libraries/tf_test2/tensorflow_src/" ABSOLUTE)
endif()

#set(TFLITE_ENABLE_GPU "Enable GPU" ON)
add_subdirectory(
       "${TENSORFLOW_SOURCE_DIR}/tensorflow/lite"
        "${CMAKE_CURRENT_BINARY_DIR}/tensorflow-lite" EXCLUDE_FROM_ALL)

#GLM
set(GLM_SOURCE_DIR "" CACHE PATH
        "Directory containing glm")
if(NOT GLM_SOURCE_DIR)
        get_filename_component(GLM_SOURCE_DIR
                "${CMAKE_CURRENT_LIST_DIR}/../../../../libraries/glm/" ABSOLUTE)
endif()

add_subdirectory(
        "${GLM_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/glm" EXCLUDE_FROM_ALL)


#GL
add_library(gl2jni SHARED
        gl_code.cpp
        Shader.h
        gl_tools.h
        SuperResolution_jni.cpp
        SuperResolution.cpp)

target_include_directories(gl2jni PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../../../../libraries/tf_test2/tensorflow_src")
target_include_directories(gl2jni PUBLIC "${GLM_SOURCE_DIR}")

# add lib dependencies
target_link_libraries(  gl2jni
                        tensorflow-lite
                        glm)


target_link_libraries(gl2jni
                      android
                      log
                      EGL
                      GLESv2
                      )

target_link_libraries(
        gl2jni
        neuralnetworks
        android
        ${log-lib}
        )

