#version 450
#extension GL_KHR_vulkan_glsl : enable
layout(location = 0) out vec4 outColor;


// 来自 ocean.vert
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPosition;
layout(location = 3) in vec2 fragOceanUV;
layout(location = 4) in float fragOceanHeight;


// set = 0：和地形共用全局 UBO
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec4 cameraPos;
    vec4 renderParams;
} ubo;

void main() {
    vec3 normal =
        normalize(fragNormal);

    vec3 viewDirection =
        normalize(
            ubo.cameraPos.xyz -
            fragWorldPosition
        );

    // 和地形使用同一份太阳方向：
    // renderParams = vec4(shadowBias, lightDir.x, lightDir.y, lightDir.z)
    vec3 lightDirection =
        normalize(ubo.renderParams.gba);

    float diffuse =
        max(
            dot(normal, lightDirection),
            0.0
        );

    vec3 halfDirection =
        normalize(
            lightDirection +
            viewDirection
        );

    // 越大则高光越细；如果浪面细节不足，先不要设得太大。
    float specular =
        pow(
            max(
                dot(normal, halfDirection),
                0.0
            ),
            128.0
        );

    float NoV =
        max(
            dot(normal, viewDirection),
            0.0
        );

    // 水的 Schlick Fresnel。
    float fresnel =
        0.02 +
        0.98 *
        pow(
            1.0 - NoV,
            5.0
        );

    vec3 deepWaterColor =
        vec3(0.005, 0.025, 0.065);

    vec3 shallowWaterColor =
        vec3(0.01, 0.12, 0.22);

    float waveColorFactor =
        clamp(
            fragOceanHeight * 0.2 + 0.5,
            0.0,
            1.0
        );

    vec3 waterColor =
        mix(
            deepWaterColor,
            shallowWaterColor,
            waveColorFactor
        );

    // 根据反射方向生成一个简单天空渐变。
    // 以后有天空盒时，用真正的 cubemap 反射替换这里。
    vec3 reflectionDirection =
        reflect(
            -viewDirection,
            normal
        );

    float skyAmount =
        clamp(
            reflectionDirection.z * 0.5 + 0.5,
            0.0,
            1.0
        );

    vec3 horizonColor =
        vec3(0.55, 0.68, 0.75);

    vec3 zenithColor =
        vec3(0.08, 0.22, 0.42);

    vec3 reflectedSky =
        mix(
            horizonColor,
            zenithColor,
            skyAmount
        );

    // 正视角保留水本体；掠射角交给反射。
    vec3 waterBase =
        waterColor *
        (0.08 + diffuse * 0.10) *
        (1.0 - fresnel);

    // 暖色太阳高光，强度由菲涅耳和法线共同决定。
    vec3 sunSpecular =
        vec3(1.0, 0.92, 0.72) *
        specular *
        mix(0.04, 1.0, fresnel) *
        1.5;

    vec3 finalColor =
        waterBase +
        reflectedSky * fresnel +
        sunSpecular;

    outColor =
        vec4(finalColor, 1.0);

}