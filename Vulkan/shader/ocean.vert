#version 450
#extension GL_KHR_vulkan_glsl : enable
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in float inFlow;


// 给 ocean.frag 的输出
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPosition;
layout(location = 3) out vec2 fragOceanUV;
layout(location = 4) out float fragOceanHeight;


// set = 0：沿用你原有的全局 UBO
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec4 cameraPos;
    vec4 renderParams;
} ubo;


// set = 1：Ocean 类自己创建的 IFFT 高度图
layout(set = 1, binding = 0)
uniform sampler2D oceanHeightMaps[4];//高度图,单纯垂直移动顶点

layout(set = 1, binding = 1)
uniform sampler2D oceanDisplacementXMaps[4];//和下面都是专门用来水平移动顶点的

layout(set = 1, binding = 2)
uniform sampler2D oceanDisplacementYMaps[4];

// 必须与 OceanPushConstant 完全一致：16 字节
layout(push_constant) uniform OceanPushConstant {
    float oceanRange;
    float heightScale;
    float horizontalScale;
    float normalSampleStep;

    vec4 cascadeRanges;
    vec4 cascadeContributions;
}pc;


vec2 getCascadeUV(vec2 worldXY, int cascadeIndex) {
    return worldXY / pc.cascadeRanges[cascadeIndex] + vec2(0.5);
}

vec3 sampleOceanSurface(vec2 worldXY) {
    vec3 result = vec3(worldXY, 0.0);

    for (int i = 0; i < 4; ++i) {
        vec2 uv = getCascadeUV(worldXY, i);

        float height =
            texture(oceanHeightMaps[i], uv).r *
            pc.heightScale *
            pc.cascadeContributions[i];

        vec2 displacement = vec2(
            texture(oceanDisplacementXMaps[i], uv).r,
            texture(oceanDisplacementYMaps[i], uv).r
        ) *
            pc.horizontalScale *
            pc.cascadeContributions[i];

        result.xy += displacement;
        result.z += height;
    }

    return result;
}

vec3 calculateOceanNormal(vec2 worldXY) {
    float d = pc.normalSampleStep;

    vec3 left  = sampleOceanSurface(worldXY - vec2(d, 0.0));
    vec3 right = sampleOceanSurface(worldXY + vec2(d, 0.0));
    vec3 down  = sampleOceanSurface(worldXY - vec2(0.0, d));
    vec3 up    = sampleOceanSurface(worldXY + vec2(0.0, d));

    return normalize(cross(right - left, up - down));
}
float sampleOceanHeight(vec2 uv) {// 采样 oceanHeightMap，返回高度值
    return texture(oceanHeightMap, uv).r *
        pc.heightScale;
}




void main() {
   vec2 baseWorldXY = inPosition.xy;

    vec3 oceanSurface =
        sampleOceanSurface(baseWorldXY);

    vec3 worldPosition = oceanSurface;

    float oceanHeight = worldPosition.z;

    fragNormal =
        calculateOceanNormal(baseWorldXY);




    fragColor = inColor;


    fragWorldPosition = worldPosition;

    fragOceanHeight = oceanHeight;

    gl_Position =
        ubo.proj *
        ubo.view *
        ubo.model *
        vec4(worldPosition, 1.0);
}
