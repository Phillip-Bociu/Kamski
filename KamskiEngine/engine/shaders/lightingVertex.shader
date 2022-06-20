#version 450
layout (location = 0) in vec2 position;
layout (location = 1) in vec4 color;
layout (location = 2) in float range;
layout (location = 3) in float distance;

layout (location = 0) out vec4 outColor;
layout (location = 1) out float outRange;
layout (location = 2) out float outDistance;

uniform vec3 camera;

void main()
{
    vec2 pos = position;
    pos -= camera.xy;
    outColor = color;
    outRange = range;
    outDistance = distance;
    gl_Position = vec4(pos * camera.z, 0.0, 1.0);
}