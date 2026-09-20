#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec4 cameraPos;
    vec4 renderParams;//这个里面是自定义数值.x暂时表示bias yzw表示阳光方向
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPosition;
layout(location = 4) out vec4 fragCameraPos; 

layout(location = 3) in float inFlow;
layout(location = 3) out float fragFlow;

layout(location = 5) out vec4 fragLightSpacePos;  
layout(location = 6) out vec4 fragRenderParams;
void main() {
    vec4 pos = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0); 
    fragCameraPos = ubo.cameraPos;
    fragFlow = inFlow;
    fragRenderParams = ubo.renderParams;
    gl_Position = pos;
    fragColor = inColor;

    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    fragLightSpacePos = ubo.lightViewProj * worldPos; 

    fragNormal = normalize(inNormal);
    fragWorldPosition =   (ubo.model * vec4(inPosition, 1.0) ).xyz;
}