#version 450
layout (location=0) in vec3 position;
layout (location=1) in vec4 color;
layout (location=2) in vec2 uv;
layout (location=3) in float texIndex;

layout (location=0) out float outIndex;
layout (location=1) out vec2 outUv;
layout (location=2) out vec4 outColor;

uniform vec3 camera;

void main() 
{
    outUv = uv;
    outIndex = texIndex;
	outColor = color;
	vec2 pos = position.xy;
    pos -= camera.xy;
    gl_Position = vec4(pos * camera.z, 1.0, 1.0);
}
