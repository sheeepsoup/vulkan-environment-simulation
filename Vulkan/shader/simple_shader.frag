#version 450
layout(location = 0) out vec4 outColor;


layout(location = 0) in vec3 fragColor;//[x]->当前点的高度
layout(location = 1) in vec3 fragNormal;//法线
layout(location = 2) in vec3 fragWorldPosition;
layout(location = 3) in float fragFlow;
layout(location = 4) in vec4 fragCameraPos;
layout(location = 5) in vec4 fragLightSpacePos; 
layout(location = 6) in vec4 fragRenderParams;//gba表示阳光方向


layout(binding = 1) uniform sampler2DShadow shadowMap;

#define HASHSCALE1 .1031
#define HASHSCALE3 vec3(.1031, .1030, .0973)
#define HASHSCALE4 vec4(1031, .1030, .0973, .1099)
const vec3 dirFix = vec3(1,-1,0);//方向修饰,模拟现实特定方向风的风化
const vec3 worldUp = vec3(0,0,1);
const vec3 grassColor={ 0.18f, 0.38f, 0.14f};
const vec3 dirtColor={0.34f, 0.24f, 0.14f};
const vec3 rockColor={0.38f, 0.39f, 0.37f};
const vec3 snowColor={0.88f, 0.92f, 0.95f};
const vec3 fogColor = vec3(0.38, 0.62, 0.82);
const float TERRAIN_SCALE = 2.0f;
const vec4 roughnessTable = vec4(
    0.92, // 1 草
    0.84, // 2 泥土
    0.68, // 3 岩石
    0.48  // 4 雪
);
const float PI = 3.14159265;
struct PBR{
    vec3 albedo;//基础颜色[存储纹理颜色]
    float metallic;//金属度
    float roughness;//粗糙度
    float ao;//环境光遮蔽
};



//绘画阴影 原理一会看
float computeShadow() {
    vec3 projCoords = fragLightSpacePos.xyz / fragLightSpacePos.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

   if (projCoords.z < 0.0 || projCoords.z > 1.0) return 1.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) return 1.0;

    float bias = fragRenderParams.x;   // 防 acne，具体值要调
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 offset = vec2(x, y) * texelSize;
            shadow += texture(shadowMap, vec3(projCoords.xy + offset, projCoords.z - bias));
        }
    }
    shadow /= 9.0;
    return shadow;
}

float Hash12(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}
vec2 Hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * HASHSCALE3);
    p3 += dot(p3, p3.yzx+19.19);
    return fract((p3.xx+p3.yz)*p3.zy);

}
vec2 add = vec2(1.0, 0.0);
float Noise(vec2 x)
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    f = f*f*(3.0-2.0*f);
    
    float res = mix(mix( Hash12(p),          Hash12(p + add.xy),f.x),
                    mix( Hash12(p + add.yx), Hash12(p + add.xx),f.x),f.y);
    return res;
}

vec2 Noise2(vec2 x)
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    f = f*f*(3.0-2.0*f);
    float n = p.x + p.y * 57.0;
   vec2 res = mix(mix( Hash22(p),          Hash22(p + add.xy),f.x),
                  mix( Hash22(p + add.yx), Hash22(p + add.xx),f.x),f.y);
    return res;
}
//--------------------------------------------------------------------------------------------------
//检测数字是否在界限内
bool numInEpsilon(float num,float leftEpsilon,float rightEpsilon){
    if(num <= rightEpsilon && num >= leftEpsilon)return true;
    else{return false;};
}
//获取纹理属性权重[r草][g泥土][b岩石][a雪地]
vec4 getTexureIndex(vec3 normal,vec3 pos){
    //获取坡度
    float pot = dot(normal,dirFix);
    float upness = clamp(dot(normal, worldUp), 0.0, 1.0);
    float slope = 1.0 - upness;
 
    float directionNoise = Noise(pos.xy * 0.015) * 2.0 - 1.0;//噪声扰乱
    pot = clamp(pot + directionNoise * 0.20,0.0,1.0);

    //噪声干扰边界
    float largeNoise = Noise(pos.xy * 0.025);
    float smallNoise = Noise(pos.xy * 0.12);
    float boundaryNoise = ((largeNoise * 0.75 + smallNoise * 0.25) * 2.0 - 1.0);
    float materialHeight = pos.z + boundaryNoise * 0.5;

	//草地权重,跟高度关系最大
	float grassNormalWeight = 1 - smoothstep(0.0f,0.35f, pot);//草原法线权重
	float grassHeightWeight = 1 - smoothstep(0.0f, 5.0f, materialHeight);//草原高度权重
	float grassWeight = mix(grassHeightWeight, grassNormalWeight, 0.5f);//草原权重
    float grassAltitudeFade =
    1.0 - smoothstep(6.0, 9.0, materialHeight);
    grassWeight *= grassAltitudeFade;

	//泥土权重
	float dirtNormalWeight = smoothstep(0.0f, 0.35f, pot);//泥土法线权重
	float dirtHeightWeight = smoothstep(3.0f, 6.0f, materialHeight);//泥土高度权重
	float dirtWeight =mix(dirtHeightWeight, dirtNormalWeight, 0.5f);//泥土权重
	float highAltitudeDirtFade = 1.0f - smoothstep(6.5f, 9.0f, materialHeight);
	dirtWeight *= highAltitudeDirtFade;//去掉泥土环

	//岩石权重
	float rockNormalWeight = smoothstep(0.0f, 0.85f, pot);//岩石法线权重
	float rockHeightWeight = smoothstep(6.0f, 9.0f, materialHeight);//岩石高度权重
    float mountainMask = smoothstep(5.0, 8.0, materialHeight);
	float rockWeight =
    mix(rockHeightWeight, rockNormalWeight, 0.4);//岩石权重
	
	//雪地权重
	float snowNormalWeight = smoothstep(0.55f, 0.90f, upness);//雪地法线权重
	float snowHeightWeight = smoothstep(12.0f, 15.0f, materialHeight);//雪地高度权重
	float baseSnow = snowHeightWeight;//雪地权重
   

    float snowChannel = smoothstep(0.05, 0.40, fragFlow);//流量影响的沟壑
    float moderateSlope = smoothstep(0.08, 0.22, slope);//去除平坦区域流量影响
    float channelHeightMask = smoothstep(7.5, 10.0, materialHeight);//沟壑高度限制区间
    float lowerChannelSnow = snowChannel * moderateSlope * channelHeightMask;//低处的雪沟壑
    float upperLargeNoise = Noise(pos.xy * 0.05 + vec2(31.7, 17.3));//俩噪声扰动
    float upperSmallNoise = Noise(pos.xy * 0.25 + vec2(11.9, 47.1));
    float upperNoise = (upperLargeNoise * 0.75 + upperSmallNoise * 0.25) * 2.0 - 1.0;//调整2合并-1~1
    float upperSnowHeight = materialHeight + upperNoise * 0.75 + lowerChannelSnow * 1.2;//噪声调整一波高度[影响效果]
    float upperBaseSnow = smoothstep(12.0, 15.0, upperSnowHeight);//高处的基础雪生成

    //合并低处雪沟和上方雪圈
    float snowCoverage = 1.0 - (1.0 - lowerChannelSnow) * (1.0 - upperBaseSnow);
    snowCoverage = clamp(snowCoverage, 0.0, 1.0);

    return vec4(grassWeight,dirtWeight,rockWeight,snowCoverage);
}
//获取纹理颜色
vec3 getTextureColor(vec4 weight){
    weight = max(weight, vec4(0.0));
    //根据侵蚀模拟给泥土颜色
    // 干燥土壤：偏浅、偏黄
    const vec3 dryDirtColor = vec3(0.42, 0.31, 0.18);

// 潮湿土壤：更深、更冷，避免直接变成纯黑
    const vec3 wetDirtColor = vec3(0.16, 0.12, 0.085);
    float wetness = smoothstep(0.08, 0.55, fragFlow);

    vec3 finalDirtColor = mix(
        dryDirtColor,
        wetDirtColor,
        wetness
    );
    vec3 terrainColor =
    grassColor * weight.x +
    finalDirtColor  * weight.y +
    rockColor  * weight.z +
    snowColor  * weight.w;

    return terrainColor;
}
//获取纹理权重[1草][2泥土][3岩石][4雪地]
int getMaxTextureIndex(vec4 weight) {
    float maxXY = max(weight.x, weight.y);
    float indexXY = mix(1.0, 2.0, step(weight.x, weight.y));

    float maxXYZ = max(maxXY, weight.z);
    float indexXYZ = mix(indexXY, 3.0, step(maxXY, weight.z));

    float finalIndex = mix(indexXYZ, 4.0, step(maxXYZ, weight.w));

    return int(finalIndex);
}
//--------------------------------------------------------------------------------------------------
//获取pbr的粗糙度[传入纹理的权重]
float getPBRRoughness(int TextureIndex){
     return roughnessTable[TextureIndex - 1];
}
//获取pbr属性[直接返回最终颜色]
vec3 getPBR(PBR obj,vec3 normal,float shadow){
    vec3 N = normalize(normal);//法线
    vec3 V = normalize(fragCameraPos.xyz - fragWorldPosition);//视线方向
    vec3 L = normalize(fragRenderParams.gba);//阳光方向
    vec3 H = normalize(L + V);//视线与光线的中间方向

    float diffuse = max(dot(N, L), 0.0);//漫反射
    float ambient = 0.20;
    float shininess = mix(128.0, 4.0, clamp(obj.roughness, 0.0, 1.0));//粗糙度影响光照强度,这自己细调
    float specular = pow(max(dot(N, H), 0.0), shininess);//镜面反射
    specular *= (1.0 - obj.roughness);//粗糙表面的反射更弱

    vec3 ambientColor = obj.albedo * ambient;//漫反射最终颜色
    vec3 directColor = obj.albedo * diffuse * 0.80 + vec3(specular) * 0.35;//镜面反射最终颜色

    return ambientColor + directColor * shadow;
}

void main(){
    //判断绘制海洋
//    if(fragWorldPosition.z == 0){
//        outColor = vec4(0.1, 0.6, 0.6,1.0);
//        float dist = length(fragCameraPos.xyz - fragWorldPosition);
//        float fogValue = 1 - smoothstep(80.0f,450.0f,dist);
//        outColor =vec4( mix(fogColor,outColor.xyz,fogValue),1.0f);
//        return;
//    }

    vec3 normal = normalize(fragNormal);
    vec4 TexutreAttribute = getTexureIndex(normal,fragWorldPosition / TERRAIN_SCALE);//计算纹理
    vec4 colora = vec4(getTextureColor(TexutreAttribute), 1.0);
 //   colora = vec4(0.5f,0.5f,0.5f,1.0f);
//    //光照计算
//    vec3 lightDirection = normalize(fragRenderParams.gba);
//    float diffuse = max(dot(normal, lightDirection),0.0 );
//    float ambient = 0.20;
    //计算阴影
    float shadow = computeShadow();//阴影的颜色


//    vec3 finalColor = colora.xyz *(ambient + diffuse * 0.80 * shadow) ;
//   //vec3 finalColor = colora.xyz *(ambient + 0.80 * shadow) ;


   //------------------------------------------------------------------------PBR计算
   PBR pbr = {
   colora.rgb,
   0.0f,
   dot(TexutreAttribute, roughnessTable) / max(dot(TexutreAttribute, vec4(1.0)), 0.001),//粗糙度
   0.0f
   };
   vec3 pbrColor = getPBR(pbr,normal,shadow);

   vec3 finalColor = pbrColor;

    //-----------------------------------------------------------------------计算雾
    float dist = length(fragCameraPos.xyz - fragWorldPosition);
    float fogValue = 1 - smoothstep(80.0f,450.0f,dist);
    finalColor = mix(fogColor,finalColor,fogValue);

   outColor = vec4(finalColor, 1.0);
   //outColor = vec4(vec3(smoothstep(0.05,0.65,fragFlow)), 1.0);
   //outColor = vec4(TexutreAttribute.a);
}

