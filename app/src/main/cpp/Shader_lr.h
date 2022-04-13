//
// Created by ivmu6294 on 05.04.2022.
//

#ifndef HELLO_GL2_SHADER_LR_H
#define HELLO_GL2_SHADER_LR_H

//SHADER FOR LOW RESOLUTION
auto vertexShaderLR = R"(#version 320 es
in vec4 vPosition;
uniform mat4 transform;
void main() {
  gl_Position = transform * vec4(vPosition.x, 0.0f - vPosition.y, vPosition.z, 1.0f);
}
)";

auto fragmentShaderLR = R"(#version 320 es
precision mediump float;
out vec4 fragColor;

vec3 RGBtoYUV(vec3 color){
    color = color * 255.0f;
    float y =  0.257f * color.r + 0.504f * color.g + 0.098f * color.b + 16.0f;
    float u = -0.148f * color.r - 0.291f * color.g + 0.439f * color.b + 128.0f;
    float v = 0.439f * color.r - 0.368f * color.g - 0.071f * color.b + 128.0f;
    return vec3(y, u, v) * (1.0f/255.0f);
}

void main() {
   vec3 color = vec3(1.0f, 1.0f, 0.0f);

    vec3 color_yuv = RGBtoYUV(color);
    fragColor = vec4(color_yuv, 1.0f);
    //fragColor = vec4(color, 1.0f);
}
)";
#endif //HELLO_GL2_SHADER_LR_H