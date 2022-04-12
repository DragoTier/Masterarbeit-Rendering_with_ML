//
// Created by ivmu6294 on 08.04.2022.
//

#ifndef HELLO_GL2_SHADER_HR_H
#define HELLO_GL2_SHADER_HR_H

//SHADER FOR HIGH RESOLUTION
auto vertexShaderHR = R"(#version 320 es
    precision mediump float;
    layout (location = 0) in vec3 pos;
    layout (location = 1) in vec2 texcoord;

    out vec2 fTexCoord;

    void main() {
        gl_Position = vec4(pos.x, pos.y, pos.z, 1.0f);
        fTexCoord = vec2(texcoord.x, texcoord.y);
    }
    )";

auto fragmentShaderHR = R"(#version 320 es
    precision mediump float;
    out vec4 fragColor;

    in vec2 fTexCoord;

    uniform sampler2D lowResTex;
    uniform sampler2D highResTex;

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
        vec4 colorLR = texture(lowResTex, fTexCoord);
        vec4 colorHR = texture(highResTex, fTexCoord);

        vec3 rgb = YUVtoRGB(vec3(colorHR.x, colorLR.y, colorLR.z));
        fragColor = vec4(rgb, 1.0f);
        //fragColor = colorLR;
        //fragColor = colorHR;
        //fragColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
        //fragColor = vec4(fTexCoord, 0.0f, 1.0f);
    }
    )";

#endif //HELLO_GL2_SHADER_HR_H
