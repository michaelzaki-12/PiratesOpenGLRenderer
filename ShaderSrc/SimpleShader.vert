#version 460 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 shadowMatrices;
out vec4 FragPos; // FragPos from GS (output per emitvertex)

void main()
{
        gl_Position =  shadowMatrices * model * vec4(aPos, 1.0);
        FragPos = gl_Position;
}

