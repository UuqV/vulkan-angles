#version 450

layout(location = 0) in dvec2 inPosition;
layout(loaction = 1) in vec3 inColor;
layout(location = 0) in vec3 fragColor;

void main()
{
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}