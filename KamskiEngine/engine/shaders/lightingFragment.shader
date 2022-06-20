#version 450

layout (location = 0) in vec4 color;
layout (location = 1) in float range;
layout (location = 2) in float distance;

layout (location = 0) out vec4 fragColor;

void main() 
{
    fragColor = vec4(color.xyz, color.a);
}