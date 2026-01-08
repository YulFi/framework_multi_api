#version 450

layout (location = 0) in vec3 vertexColor;
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec4 FragColor;

layout (binding = 0) uniform sampler2D textureSampler;

void main()
{
    vec4 texColor = texture(textureSampler, texCoord);
    FragColor = texColor * vec4(vertexColor, 1.0);
}
