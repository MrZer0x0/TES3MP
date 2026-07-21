#define SHADOWS @shadows_enabled

uniform bool runtimeShadowsEnabled;
uniform float runtimeShadowStrength;
uniform float runtimeShadowSoftness;
uniform float runtimeShadowMaxDistance;
uniform float runtimeShadowFadeStart;
uniform float runtimeShadowTexelSize;

#if SHADOWS
    uniform float maximumShadowMapDistance;
    uniform float shadowFadeStart;
    @foreach shadow_texture_unit_index @shadow_texture_unit_list
        uniform sampler2DShadow shadowTexture@shadow_texture_unit_index;
        varying vec4 shadowSpaceCoords@shadow_texture_unit_index;
#if @perspectiveShadowMaps
        varying vec4 shadowRegionCoords@shadow_texture_unit_index;
#endif
    @endforeach
#endif

float sampleShadowPCF(sampler2DShadow shadowMap, vec4 shadowCoords, float texelSize)
{
    vec3 coords = shadowCoords.xyz / shadowCoords.w;
    float stepSize = max(texelSize, 0.00001) * max(runtimeShadowSoftness, 0.25);

    // Four bilinearly filtered taps give a soft 2x2/4x4-like kernel at low cost.
    float shadow = 0.0;
    shadow += shadow2D(shadowMap, vec3(coords.xy + vec2(-0.75, -0.25) * stepSize, coords.z)).r;
    shadow += shadow2D(shadowMap, vec3(coords.xy + vec2( 0.25, -0.75) * stepSize, coords.z)).r;
    shadow += shadow2D(shadowMap, vec3(coords.xy + vec2(-0.25,  0.75) * stepSize, coords.z)).r;
    shadow += shadow2D(shadowMap, vec3(coords.xy + vec2( 0.75,  0.25) * stepSize, coords.z)).r;
    return shadow * 0.25;
}

float unshadowedLightRatio(float distance)
{
    if (!runtimeShadowsEnabled)
        return 1.0;

    float shadowing = 1.0;
#if SHADOWS
    float maxDistance = runtimeShadowMaxDistance > 0.0 ? runtimeShadowMaxDistance : maximumShadowMapDistance;
    float fadeStart = clamp(runtimeShadowFadeStart, 0.0, 1.0) * maxDistance;
    float fade = maxDistance > 0.0
        ? clamp((distance - fadeStart) / max(maxDistance - fadeStart, 0.001), 0.0, 1.0)
        : 0.0;
    if (fade >= 1.0)
        return 1.0;

#if @shadowMapsOverlap
    bool doneShadows = false;
    @foreach shadow_texture_unit_index @shadow_texture_unit_list
        if (!doneShadows)
        {
            vec3 shadowXYZ = shadowSpaceCoords@shadow_texture_unit_index.xyz / shadowSpaceCoords@shadow_texture_unit_index.w;
#if @perspectiveShadowMaps
            vec3 shadowRegionXYZ = shadowRegionCoords@shadow_texture_unit_index.xyz / shadowRegionCoords@shadow_texture_unit_index.w;
#endif
            if (all(lessThan(shadowXYZ.xy, vec2(1.0))) && all(greaterThan(shadowXYZ.xy, vec2(0.0))))
            {
                shadowing = min(sampleShadowPCF(shadowTexture@shadow_texture_unit_index,
                    shadowSpaceCoords@shadow_texture_unit_index, runtimeShadowTexelSize), shadowing);
                doneShadows = all(lessThan(shadowXYZ, vec3(0.95, 0.95, 1.0)))
                    && all(greaterThan(shadowXYZ, vec3(0.05, 0.05, 0.0)));
#if @perspectiveShadowMaps
                doneShadows = doneShadows && all(lessThan(shadowRegionXYZ, vec3(1.0)))
                    && all(greaterThan(shadowRegionXYZ.xy, vec2(-1.0)));
#endif
            }
        }
    @endforeach
#else
    @foreach shadow_texture_unit_index @shadow_texture_unit_list
        shadowing = min(sampleShadowPCF(shadowTexture@shadow_texture_unit_index,
            shadowSpaceCoords@shadow_texture_unit_index, runtimeShadowTexelSize), shadowing);
    @endforeach
#endif

    shadowing = mix(shadowing, 1.0, fade);
#endif

    return mix(1.0, shadowing, clamp(runtimeShadowStrength, 0.0, 1.0));
}

void applyShadowDebugOverlay()
{
#if SHADOWS && @useShadowDebugOverlay
    bool doneOverlay = false;
    float colourIndex = 0.0;
    @foreach shadow_texture_unit_index @shadow_texture_unit_list
        if (!doneOverlay)
        {
            vec3 shadowXYZ = shadowSpaceCoords@shadow_texture_unit_index.xyz / shadowSpaceCoords@shadow_texture_unit_index.w;
#if @perspectiveShadowMaps
            vec3 shadowRegionXYZ = shadowRegionCoords@shadow_texture_unit_index.xyz / shadowRegionCoords@shadow_texture_unit_index.w;
#endif
            if (all(lessThan(shadowXYZ.xy, vec2(1.0))) && all(greaterThan(shadowXYZ.xy, vec2(0.0))))
            {
                colourIndex = mod(@shadow_texture_unit_index.0, 3.0);
                if (colourIndex < 1.0) gl_FragData[0].x += 0.1;
                else if (colourIndex < 2.0) gl_FragData[0].y += 0.1;
                else gl_FragData[0].z += 0.1;
                doneOverlay = all(lessThan(shadowXYZ, vec3(0.95, 0.95, 1.0)))
                    && all(greaterThan(shadowXYZ, vec3(0.05, 0.05, 0.0)));
#if @perspectiveShadowMaps
                doneOverlay = doneOverlay && all(lessThan(shadowRegionXYZ, vec3(1.0)))
                    && all(greaterThan(shadowRegionXYZ.xy, vec2(-1.0)));
#endif
            }
        }
    @endforeach
#endif
}
