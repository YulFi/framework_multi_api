#version 330 core
in vec3 vertexColor;
in vec2 texCoord;
out vec4 FragColor;

uniform sampler2D textureSampler;

void main()
{
    vec4 texColor = texture(textureSampler, texCoord);
    FragColor = texColor * vec4(vertexColor, 1.0);
}
