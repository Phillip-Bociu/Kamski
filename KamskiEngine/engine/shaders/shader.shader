#shader vertex
#version 450
layout (location=0) in vec4 position;
layout (location=1) in vec3 color;
out vec3 vColor;
void main() {
    vColor = color;
    gl_Position = position;
};

#shader fragment
#version 450
in vec3 vColor;
out vec4 fragColor;
void main() {
    fragColor = vec4(vColor, 1.0);
};