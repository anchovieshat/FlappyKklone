#version 330 core
#extension GL_ARB_separate_shader_objects : enable

in vec2 TexCoord;
in float Red;

uniform sampler2D textureSampler;

void main()
{
    if(Red == 0.0)
        gl_FragColor = texture2D(textureSampler, TexCoord);
    else if(Red == 1.0)
        gl_FragColor = texture2D(textureSampler, TexCoord) * vec4(1, 0, 0, 1);
}
