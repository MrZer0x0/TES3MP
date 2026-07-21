#include "lighting_util.glsl"
#include "lighting_pbr_compat.glsl"

const float POINT_BLOOM_MULTIPLIER = 0.30;
const float POINT_GLOW_MULTIPLIER = 0.50;

vec3 bloomAccumulator = vec3(0.0);

vec3 calculateBloom(vec3 lightColor, float lightIntensity, float distance, float radius, float multiplier)
{
    float bloomFactor = lightIntensity * smoothstep(radius * 2.0, 0.0, distance);
    bloomFactor = pow(max(bloomFactor, 0.0), 2.5);
    return lightColor * bloomFactor * max(hdrBloomIntensity, 0.0) * multiplier;
}

vec3 calculateGlow(vec3 lightColor, float distance, float radius, float multiplier)
{
    float effectiveRadius = radius * max(lightGlowRadius, 0.1);
    float glowFactor = smoothstep(effectiveRadius, 0.0, distance);
    glowFactor = pow(max(glowFactor, 0.0), 2.0);
    return lightColor * glowFactor * max(lightGlowIntensity, 0.0) * 0.20 * multiplier;
}

void perLightSun(out vec3 diffuseOut, vec3 viewPos, vec3 viewNormal)
{
    vec3 lightDir = normalize(lcalcPosition(0));
    float lambert = dot(viewNormal.xyz, lightDir);

#ifndef GROUNDCOVER
    lambert = max(lambert, 0.0);
#else
    float eyeCosine = dot(normalize(viewPos), viewNormal.xyz);
    if (lambert < 0.0)
    {
        lambert = -lambert;
        eyeCosine = -eyeCosine;
    }
    lambert *= clamp(-8.0 * (1.0 - 0.3) * eyeCosine + 1.0, 0.3, 1.0);
#endif

    vec3 sunDiffuse = lcalcDiffuse(0).xyz * lambert * 0.7 * max(lightDirectIntensity, 0.0);
    diffuseOut = sunDiffuse;

    float sunBloom = max(lambert - 0.8, 0.0) * 5.0;
    bloomAccumulator += sunDiffuse * sunBloom * max(hdrBloomIntensity, 0.0) * 0.5;
}

void perLightPoint(out vec3 ambientOut, out vec3 diffuseOut, int lightIndex, vec3 viewPos, vec3 viewNormal)
{
    vec3 lightPos = lcalcPosition(lightIndex) - viewPos;
    float lightDistance = length(lightPos);

#if !@lightingMethodFFP
    float radius = lcalcRadius(lightIndex);
    if (lightDistance > radius * max(lightGlowRadius, 0.1) * 2.0)
    {
        ambientOut = vec3(0.0);
        diffuseOut = vec3(0.0);
        return;
    }
#endif

    lightPos = normalize(lightPos);
    float illumination = lcalcIllumination(lightIndex, lightDistance);
    vec3 lightDiffuse = lcalcDiffuse(lightIndex);

    ambientOut = lcalcAmbient(lightIndex) * illumination * max(lightAmbientIntensity, 0.0);
    float lambert = dot(viewNormal.xyz, lightPos) * illumination;

#ifndef GROUNDCOVER
    lambert = max(lambert, 0.0);
#else
    float eyeCosine = dot(normalize(viewPos), viewNormal.xyz);
    if (lambert < 0.0)
    {
        lambert = -lambert;
        eyeCosine = -eyeCosine;
    }
    lambert *= clamp(-8.0 * (1.0 - 0.3) * eyeCosine + 1.0, 0.3, 1.0);
#endif

    diffuseOut = lightDiffuse * lambert * 0.6 * max(lightDirectIntensity, 0.0);

#if !@lightingMethodFFP
    diffuseOut += calculateGlow(lightDiffuse, lightDistance, radius, POINT_GLOW_MULTIPLIER);
    bloomAccumulator += calculateBloom(lightDiffuse, illumination * lambert, lightDistance, radius, POINT_BLOOM_MULTIPLIER);
#endif
}

#if PER_PIXEL_LIGHTING
void doLighting(vec3 viewPos, vec3 viewNormal, float shadowing, out vec3 diffuseLight, out vec3 ambientLight)
#else
void doLighting(vec3 viewPos, vec3 viewNormal, out vec3 diffuseLight, out vec3 ambientLight, out vec3 shadowDiffuse)
#endif
{
    vec3 ambientOut, diffuseOut;
    bloomAccumulator = vec3(0.0);

    perLightSun(diffuseOut, viewPos, viewNormal);
    ambientLight = gl_LightModel.ambient.xyz * max(lightAmbientIntensity, 0.0);
#if PER_PIXEL_LIGHTING
    float ambientLift = 0.15;
    diffuseLight = diffuseOut * max(shadowing, ambientLift);
#else
    shadowDiffuse = diffuseOut;
    diffuseLight = vec3(0.0);
#endif

    for (int i = @startLight; i < @endLight; ++i)
    {
#if @lightingMethodUBO
        perLightPoint(ambientOut, diffuseOut, PointLightIndex[i], viewPos, viewNormal);
#else
        perLightPoint(ambientOut, diffuseOut, i, viewPos, viewNormal);
#endif
        ambientLight += ambientOut;
        diffuseLight += diffuseOut;
    }

    diffuseLight += bloomAccumulator;
}

vec3 getSpecular(vec3 viewNormal, vec3 viewDirection, float shininess, vec3 matSpec)
{
    vec3 lightDir = normalize(lcalcPosition(0));
    vec3 viewDir = normalize(-viewDirection);
    return pbrSunSpecular(normalize(viewNormal), viewDir, lightDir, shininess,
        lcalcSpecular(0).xyz * matSpec) * max(lightSpecularIntensity, 0.0);
}
