//
// Created by ivmu6294 on 05.04.2022.
//

#ifndef HELLO_GL2_SHADER_H
#define HELLO_GL2_SHADER_H

#endif //HELLO_GL2_SHADER_H

namespace GLCode {

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
//out vec4 fragColor[2];

vec3 RGBtoVUY(vec3 color){
    color = color * 255.0f;
    float y =  0.257f * color.r + 0.504f * color.g + 0.098f * color.b + 16.0f;
    float u = -0.148f * color.r - 0.291f * color.g + 0.439f * color.b + 128.0f;
    float v = 0.439f * color.r - 0.368f * color.g - 0.071f * color.b + 128.0f;
    return vec3(y, u, v) * (1.0f/255.0f);
}

void main() {
   vec3 color = vec3(1.0f, 1.0f, 0.0f);

    vec3 yuv = RGBtoVUY(color);
    fragColor = vec4(yuv, 1.0f);
    //fragColor = vec4(color, 1.0f);
}
)";


//SHADER FOR HIGH RESOLUTION
auto vertexShaderHR = R"(#version 320 es
    in vec4 vPosition;
    void main() {
      gl_Position = vec4(vPosition.x, vPosition.y, vPosition.z, vPosition.w);
    }
    )";

auto fragmentShaderHR = R"(#version 320 es
    precision mediump float;
    out vec4 fragColor;
    //out vec4 fragColor[2];

    vec3 YUVtoRGB(vec3 color){
        color = color * 255.0f;
        float y = color.r - 16.0f;
        float u = color.g - 128.0f;
        float v = color.b - 128.0f;
        float r = 1.164f * y             + 1.596f * v;
        float g = 1.164f * y - 0.392f * u - 0.813f * v;
        float b = 1.164f * y + 2.017f * u;
        return vec3(r, g, b) * (1.0f/255.0f);
    }

    void main() {
       vec3 color = vec3(1.0f, 1.0f, 0.0f);

        vec3 yuv = YUVfromRGB(color);
        fragColor = vec4(yuv, 1.0f);
        //fragColor = vec4(color, 1.0f);
    }
    )";
}