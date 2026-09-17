#version 330 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

uniform mat4 uViewProj;
uniform mat4 uModel;
uniform vec2 uUvOffset;
uniform vec2 uUvScale;

out vec2 vUV;

void main()
{
    vUV = uUvOffset + aUV * uUvScale;
    gl_Position = uViewProj * uModel * vec4(aPos, 0.0, 1.0);
}
