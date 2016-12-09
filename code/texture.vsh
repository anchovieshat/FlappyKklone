#version 330 core
#extension GL_ARB_explicit_uniform_location : enable

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) uniform float red;

out vec2 TexCoord;
out float Red;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    TexCoord = texCoord;
    Red = red;
}
