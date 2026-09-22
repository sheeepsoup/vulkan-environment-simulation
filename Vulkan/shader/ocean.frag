#version 450

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
    vec3 normal = normalize(fragNormal);

    vec3 viewDirection =
        normalize(
            ubo.cameraPos.xyz -
            fragWorldPosition
        );

    // 暂时固定光线方向。
    // 后面可以和地形统一使用 renderParams.gba。
    vec3 lightDirection =
        normalize(vec3(-1.0, -1.0, 1.0));

    float diffuse =
        max(dot(normal, lightDirection), 0.0);
        vec3 halfDirection =
    normalize(
        lightDirection +
        viewDirection
    );

    // 数值越大，高光越小、越集中
    float specular =
        pow(
            max(dot(normal, halfDirection), 0.0),
            128.0
        );
    // 菲涅耳：
    // 正对水面时反射弱；
    // 斜着看远处水面时反射强。
   float fresnel =
    pow(
        1.0 -
        max(dot(normal, viewDirection), 0.0),
        5.0
    );
    float waterLighting =
    0.12 + diffuse * 0.12;
    vec3 deepWaterColor =
        vec3(0.005, 0.035, 0.09);

    vec3 shallowWaterColor =
        vec3(0.015, 0.20, 0.34);

    // 先用当前波高做一点颜色变化。
    // 真实的深浅水效果以后要根据海底高度算。
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

    vec3 skyReflectionColor =
        vec3(0.40, 0.66, 0.88);

    vec3 finalColor =
        waterColor * (0.18 + diffuse * 0.45) +
        skyReflectionColor * fresnel * 0.70;

    outColor = vec4(finalColor, 1.0);
}