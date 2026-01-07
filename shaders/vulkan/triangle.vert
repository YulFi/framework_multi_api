#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

// Push constants for transformation matrices
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 projection;
} pushConstants;

void main() {
    gl_Position = pushConstants.projection * pushConstants.view * pushConstants.model * vec4(inPos, 1.0);
    fragColor = inColor;
}
