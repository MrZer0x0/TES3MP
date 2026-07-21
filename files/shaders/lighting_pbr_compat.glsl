// PBR compatibility layer for the TES3MP/OpenMW 0.47 lighting API.
// Keeps the existing shader interfaces while replacing legacy Blinn-Phong
// highlights with a normalized GGX/Smith/Schlick microfacet BRDF.
#ifndef TES3MP_PBR_COMPAT_GLSL
#define TES3MP_PBR_COMPAT_GLSL

const float PBR_PI = 3.14159265358979323846;

float pbrSaturate(float v) { return clamp(v, 0.0, 1.0); }
float pbrDistributionGGX(float NdotH, float roughness)
{
    float a = max(roughness * roughness, 0.0025);
    float a2 = a * a;
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(PBR_PI * d * d, 1e-5);
}
float pbrGeometrySchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) * 0.125;
    return NdotX / max(NdotX * (1.0 - k) + k, 1e-5);
}
float pbrGeometrySmith(float NdotV, float NdotL, float roughness)
{
    return pbrGeometrySchlickGGX(NdotV, roughness) * pbrGeometrySchlickGGX(NdotL, roughness);
}
vec3 pbrFresnelSchlick(float HdotV, vec3 f0)
{
    float f = pow(1.0 - pbrSaturate(HdotV), 5.0);
    return f0 + (1.0 - f0) * f;
}
float pbrRoughnessFromShininess(float shininess)
{
    return clamp(sqrt(2.0 / max(shininess + 2.0, 2.0)), 0.045, 1.0);
}
vec3 pbrSunSpecular(vec3 N, vec3 V, vec3 L, float shininess, vec3 materialSpecular)
{
    vec3 H = normalize(V + L);
    float NdotL = pbrSaturate(dot(N, L));
    float NdotV = pbrSaturate(dot(N, V));
    float NdotH = pbrSaturate(dot(N, H));
    float HdotV = pbrSaturate(dot(H, V));
    if (NdotL <= 0.0 || NdotV <= 0.0) return vec3(0.0);
    float roughness = pbrRoughnessFromShininess(shininess);
    vec3 f0 = clamp(materialSpecular, vec3(0.02), vec3(0.98));
    float D = pbrDistributionGGX(NdotH, roughness);
    float G = pbrGeometrySmith(NdotV, NdotL, roughness);
    vec3 F = pbrFresnelSchlick(HdotV, f0);
    return (D * G * F / max(4.0 * NdotV * NdotL, 1e-4)) * NdotL;
}
#endif
