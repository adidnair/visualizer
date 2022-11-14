#version 330 core
layout(location = 0) in vec2 vx_position;

void main()
{
    gl_Position = vec4(vx_position, 0.0, 1.0);
}
