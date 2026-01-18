#version 330 core

layout(location=0) in vec4 in_Position;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(in_Position.xyz, 1.0);
}
