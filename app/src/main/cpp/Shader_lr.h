
#ifndef HELLO_GL2_SHADER_LR_H
#define HELLO_GL2_SHADER_LR_H

//SHADER FOR LOW RESOLUTION
//vertex shader
auto vertexShaderLR = R"(#version 320 es
layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec2 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 fTexCoord;

void main() {
    gl_Position = projection * view * model * vec4(vPosition.x, vPosition.y, vPosition.z, 1.0);
    fTexCoord = vec2(texCoord.x, texCoord.y);
}
)";

//fragment shader. RGB to YUV same formula as in gl_context_lr
auto fragmentShaderLR = R"(#version 320 es
precision mediump float;
out vec4 fragColor;

in vec2 fTexCoord;

layout (location = 0) uniform sampler2D noiseTex;

vec3 RGBtoYUV(vec3 color){
    color = color * 255.0;
    float y =  0.257 * color.r + 0.504 * color.g + 0.098 * color.b + 16.0;
    float u = -0.148 * color.r - 0.291 * color.g + 0.439 * color.b + 128.0;
    float v = 0.439 * color.r - 0.368 * color.g - 0.071 * color.b + 128.0;
    return vec3(y, u, v) * (1.0/255.0);
}

void main() {
    vec3 color = vec3(1.0, 1.0, 0.0);
    vec4 coloredTexture = texture(noiseTex, fTexCoord * 10.0);
    vec3 color_yuv = RGBtoYUV(coloredTexture.rgb * 1.0);
    fragColor = vec4(color_yuv * 1.0, 1.0);
    //fragColor = vec4(color_yuv.r, 0.0, 0.0, 1.0);
    //fragColor = vec4(coloredTexture.rgb, 1.0);
    //fragColor = vec4(color, 1.0);
}
)";
#endif //HELLO_GL2_SHADER_LR_H