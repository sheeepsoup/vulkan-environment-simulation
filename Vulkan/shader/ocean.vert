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
uniform sampler2D oceanHeightMap;//高度图,单纯垂直移动顶点

layout(set = 1, binding = 1)
uniform sampler2D oceanDisplacementXMap;//和下面都是专门用来水平移动顶点的

layout(set = 1, binding = 2)
uniform sampler2D oceanDisplacementYMap;

// 必须与 OceanPushConstant 完全一致：16 字节
layout(push_constant) uniform OceanPushConstant {
    float oceanRange;
    float heightScale;
    float horizontalScale;
    float padding;
} pc;
float sampleOceanHeight(vec2 uv) {// 采样 oceanHeightMap，返回高度值
    return texture(oceanHeightMap, uv).r *
        pc.heightScale;
}

vec3 sampleOceanSurface(vec2 oceanUV) {
    float height = sampleOceanHeight(oceanUV);
    vec2 horizontalDisplacement = vec2(
        texture(oceanDisplacementXMap, oceanUV).r,
        texture(oceanDisplacementYMap, oceanUV).r
    ) * pc.horizontalScale;

    vec2 basePosition =
        (oceanUV - vec2(0.5)) * pc.oceanRange;

    return vec3(basePosition + horizontalDisplacement, height);
}

vec3 calculateOceanNormal(vec2 oceanUV) {//计算包含水平位移的真实法线
    vec2 texelSize =
        1.0 /
        vec2(textureSize(oceanHeightMap, 0));

    vec3 left = sampleOceanSurface(oceanUV - vec2(texelSize.x, 0.0));
    vec3 right = sampleOceanSurface(oceanUV + vec2(texelSize.x, 0.0));
    vec3 down = sampleOceanSurface(oceanUV - vec2(0.0, texelSize.y));
    vec3 up = sampleOceanSurface(oceanUV + vec2(0.0, texelSize.y));

    return normalize(cross(right - left, up - down));
}
void main() {
    // Ocean::createMesh() 创建的初始平面顶点
    vec3 worldPosition = inPosition;

    // 顶点 x/y 范围：
    // [-oceanRange / 2, +oceanRange / 2]
    // 转成纹理 UV：[0, 1]
    vec2 oceanUV =
        worldPosition.xy / pc.oceanRange +
        vec2(0.5);

    // IFFT 高度图：R 通道就是最终海浪高度
    float oceanHeight =
    sampleOceanHeight(oceanUV);

    float displacementX =
    texture(
        oceanDisplacementXMap,
        oceanUV
    ).r * pc.horizontalScale;

    float displacementY =
        texture(
            oceanDisplacementYMap,
            oceanUV
        ).r * pc.horizontalScale;

    worldPosition.xy += vec2(
        displacementX,
        displacementY
    );

    worldPosition.z += oceanHeight;

    fragNormal = calculateOceanNormal(oceanUV);


    fragColor = inColor;


    fragWorldPosition = worldPosition;
    fragOceanUV = oceanUV;
    fragOceanHeight = oceanHeight;

    gl_Position =
        ubo.proj *
        ubo.view *
        ubo.model *
        vec4(worldPosition, 1.0);
}
