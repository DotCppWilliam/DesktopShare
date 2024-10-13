#version 330 core
out vec2 vTexCoord;
void main()
{
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    vTexCoord = vec2(x + 1.0, 1.0 - y) * 0.5;
    gl_Position = vec4(x, y, 0, 1);
}

