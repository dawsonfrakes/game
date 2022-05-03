#type vertex
#version 450 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texcoord;
layout(location = 2) in vec3 a_normal;

out vec2 f_texcoord;

uniform mat4 u_vp;
uniform mat4 u_model;

void main()
{
    gl_Position = u_vp * u_model * vec4(a_position, 1.0);
    f_texcoord = a_texcoord;
}

#type fragment
#version 450 core

in vec2 f_texcoord;

out vec4 color;

uniform sampler2D u_texture;

void main()
{
    color = texture(u_texture, f_texcoord);
}
