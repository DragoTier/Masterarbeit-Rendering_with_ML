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

#ASSIMP
set(ASSIMP_SOURCE_DIR "" CACHE PATH
        "Directory containing assimp")
if(NOT ASSIMP_SOURCE_DIR)
        get_filename_component(ASSIMP_SOURCE_DIR
                "${CMAKE_CURRENT_LIST_DIR}/../../../../libraries/assimp/" ABSOLUTE)
endif()

#set(ASSIMP_INJECT_DEBUG_POSTFIX OFF)

add_subdirectory(
        "${ASSIMP_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/assimp" EXCLUDE_FROM_ALL)

#GL
add_library(gl3aiupscale SHARED
        gl_context_lr.cpp
        gl_context_hr.cpp
        Shader_lr.h
        Shader_hr.h
        gl_tools_lr.h
        gl_tools_hr.h
        gl_texture_model.h
        Assimp_Mesh.cpp
        Assimp_Mesh.h
        Assimp_Model.cpp
        Assimp_Model.h
        espcn_jni.cpp
        SuperResolution.cpp)

target_include_directories(gl3aiupscale PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../../../../libraries/tf_test2/tensorflow_src")
target_include_directories(gl3aiupscale PUBLIC "${GLM_SOURCE_DIR}")
target_include_directories(gl3aiupscale PUBLIC "${ASSIMP_SOURCE_DIR}")

# add lib dependencies
target_link_libraries(  gl3aiupscale
                        tensorflow-lite
                        glm
                        assimp)


target_link_libraries(gl3aiupscale
                      android
                      log
                      EGL
                      GLESv3
                      )

target_link_libraries(
        gl3aiupscale
        neuralnetworks
        android
        ${log-lib}
        )

