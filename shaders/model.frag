#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject
{
    mat4    model;
    mat4    view;
    mat4    proj;
    vec4    color;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outColor;


void main()
{
    vec3 ambientLightColor = vec3(0.5,0.5,0.5);
    vec3 globalLightSource = vec3(0,0,100);
    vec3 lightDir = globalLightSource - inPosition;
    normalize(lightDir);
    float brightness = dot(lightDir, inNormal) * .5;
    vec4 texColor = texture(texSampler, inUV);
    //DO SOME MATH FOR NORMALS
    outColor = vec4(texColor.xyz * ambientLightColor, texColor.w);
    outColor.xyz *= brightness * texColor.xyz;
    outColor *= ubo.color;
}
