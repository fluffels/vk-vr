#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec4 inPosition;

void main() {
    gl_Position = inPosition;
}
