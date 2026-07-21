// Runtime-configurable HDR and colour grading for ArenaMP.
// The project uses per-material tone mapping rather than a separate full-screen pass,
// so every supported material shares the same uniforms and applies changes immediately.

uniform bool hdrEnabled;
uniform int hdrTonemapper; // 0 ACES, 1 Reinhard, 2 Filmic, 3 Neutral
uniform float hdrExposure;
uniform float hdrGamma;
uniform float hdrBrightness;
uniform float hdrContrast;
uniform float hdrSaturation;
uniform float hdrBloomIntensity;
uniform float hdrBloomThreshold;

// Lighting controls are declared here because HDR.glsl is included before lighting.glsl.
uniform float lightDirectIntensity;
uniform float lightAmbientIntensity;
uniform float lightSpecularIntensity;
uniform float lightGlowIntensity;
uniform float lightGlowRadius;

vec3 arenaAces(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 arenaReinhard(vec3 x)
{
    return x / (vec3(1.0) + x);
}

vec3 arenaFilmic(vec3 x)
{
    // Compact Uncharted-style filmic curve, normalized for a white point of 11.2.
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    vec3 mapped = ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    float white = ((11.2 * (A * 11.2 + C * B) + D * E) / (11.2 * (A * 11.2 + B) + D * F)) - E / F;
    return clamp(mapped / max(white, 0.0001), 0.0, 1.0);
}

vec3 preLight(vec3 x)
{
    return pow(max(x, vec3(0.0)), vec3(2.2));
}

vec3 extractBrightness(vec3 color, float threshold)
{
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float knee = max(threshold * 0.5, 0.0001);
    float soft = clamp((luminance - threshold + knee) / (2.0 * knee), 0.0, 1.0);
    soft = soft * soft * (3.0 - 2.0 * soft);
    float contribution = max(luminance - threshold, 0.0) + soft * knee;
    return color * (contribution / max(luminance, 0.0001));
}

vec3 toneMap(vec3 x)
{
    vec3 color = max(x, vec3(0.0)) * max(hdrExposure, 0.0);
    color += extractBrightness(color, max(hdrBloomThreshold, 0.01)) * max(hdrBloomIntensity, 0.0);

    vec3 mapped;
    if (!hdrEnabled || hdrTonemapper == 3)
        mapped = clamp(color, 0.0, 1.0);
    else if (hdrTonemapper == 1)
        mapped = arenaReinhard(color);
    else if (hdrTonemapper == 2)
        mapped = arenaFilmic(color);
    else
        mapped = arenaAces(color);

    mapped *= max(hdrBrightness, 0.0);
    mapped = (mapped - vec3(0.5)) * max(hdrContrast, 0.0) + vec3(0.5);
    float luminance = dot(mapped, vec3(0.2126, 0.7152, 0.0722));
    mapped = mix(vec3(luminance), mapped, max(hdrSaturation, 0.0));

    return pow(clamp(mapped, 0.0, 1.0), vec3(1.0 / max(hdrGamma, 0.1)));
}
