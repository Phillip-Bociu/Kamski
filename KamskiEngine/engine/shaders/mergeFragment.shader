#version 450
layout (location = 0) in vec2 uv;
layout (location = 0) out vec4 fragColor;

uniform sampler2D framebuffers[2];
float kernel[9] = float[](
    1.0 / 16, 2.0 / 16, 1.0 / 16,
    2.0 / 16, 4.0 / 16, 2.0 / 16,
    1.0 / 16, 2.0 / 16, 1.0 / 16  
);

const float offset = 1.0 / 300.0;

vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), // top-left
        vec2( 0.0f,    offset), // top-center
        vec2( offset,  offset), // top-right
        vec2(-offset,  0.0f),   // center-left
        vec2( 0.0f,    0.0f),   // center-center
        vec2( offset,  0.0f),   // center-right
        vec2(-offset, -offset), // bottom-left
        vec2( 0.0f,   -offset), // bottom-center
        vec2( offset, -offset)  // bottom-right    
);

uniform float blurWholeScreen;

void main() 
{
    vec3 col1 = vec3(0.0);
    vec3 col2 = vec3(0.0);
    
    vec3 sampleTex[9];
    
    for(int i = 0; i < 9; i++)
    {
        sampleTex[i] = vec3(texture(framebuffers[1], uv + offsets[i]));
    }
    
    for(int i = 0; i < 9; i++)
    {
        col2 += sampleTex[i] * kernel[i];
    }
     
    if(blurWholeScreen == 1.0f)
    {
        for(int i = 0; i < 9; i++)
        {
            sampleTex[i] = vec3(texture(framebuffers[0], uv + offsets[i]));
        }
        
        for(int i = 0; i < 9; i++)
        {
            col1 += sampleTex[i] * kernel[i];
        }
    } else
    {
        col1 = vec3(texture(framebuffers[0], uv));
    }
    
    fragColor = vec4(col1, 1.0);
}
