#version 450

layout (location = 0) out vec2 outUv;

vec2 positions[4] = 
vec2[4](
 vec2(-1, -1),
 vec2( 1, -1),
 vec2( 1,  1),
 vec2(-1,  1)
);

vec2 uvs[4] = 
vec2[4](
 vec2(0, 0),
 vec2(1, 0),
 vec2(1, 1),
 vec2(0, 1)
 );

void main() 
{
    outUv = uvs[gl_VertexID];
    gl_Position = vec4(positions[gl_VertexID], 1.0, 1.0);
}
