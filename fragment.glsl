#version 330 core
in vec2 vTexCoord;
uniform sampler2D uTexture;
layout (location = 0) out vec4 oColor;
void main()
{
   oColor = texture(uTexture, vTexCoord);
}

