#version 450

layout (location=0) in float outIndex;
layout (location=1) in vec2 outUv;
layout (location=2) in vec4 outColor;

out vec4 fragColor;
uniform sampler2D textures[32];

void main() 
{
	int index = int(outIndex);
	if(outIndex != -1.0f)
	{
    	fragColor = texture(textures[index], outUv) * outColor;
	}
	else
	{
		fragColor = outColor;
	}
}