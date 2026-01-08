#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

layout (location = 0) out vec3 vertexColor;
layout (location = 1) out vec2 texCoord;

layout (push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 projection;
} pushConstants;

void main()
{
    gl_Position = pushConstants.projection * pushConstants.view * pushConstants.model * vec4(aPos, 1.0);
    vertexColor = aColor;
    texCoord = aTexCoord;
}
