#version 450 core

layout(location = 0) in vec3 f_color;

layout(location = 0) out vec4 color;

void main()
{
    color = vec4(f_color, 1.0);
}
