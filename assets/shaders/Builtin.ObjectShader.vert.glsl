#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(set = 0, binding = 0) uniform global_uniform_object {
    mat4 projection;
	mat4 view;
} global_ubo;

layout(push_constant) uniform push_constant {
    // only guaranteed to be 128 bytes
    // using 64 bytes
    mat4 model;
	
} u_push_constants;

void main() {
    // Projection * view * model * position THIS ORDER MATTERS
    gl_Position = global_ubo.projection * global_ubo.view * u_push_constants.model * vec4(in_position, 1.0);
} 
