#ifndef LIB_MATERIAL_PARALLAX
#define LIB_MATERIAL_PARALLAX

// ============================================================================
//  AO METHOD SELECTOR  --  set PARALLAX_AO_METHOD to a number:
// ============================================================================
//   0 = LOD-style cavity AO, GLonD3D12-safe (ring taps, no texture2DLod)  <-- recommended
//   1 = original LOD cavity AO using texture2DLod (BREAKS on GLonD3D12!)
//   2 = Rafael PBR neighbour-sample AO
//
#define PARALLAX_AO_METHOD 2
// ============================================================================

// --- Configuration ---
#define PARALLAX_SCALE_TERRAIN 0.04
#define PARALLAX_SCALE_OBJECTS 0.04
#define PARALLAX_BIAS 0.3
#define PARALLAX_NEUTRAL 0.5  // Heightmap value that represents the geometric surface (0.5 = bidirectional)
#define PARALLAX_SHADOW_STRENGTH_TERRAIN 4.0
#define PARALLAX_SHADOW_STRENGTH_OBJECTS 4.0
#define PARALLAX_SHADOW_STEPS 10

// --- Layer Counts (easy to tune) ---
#define PARALLAX_MIN_LAYERS_TERRAIN 4.0
#define PARALLAX_MAX_LAYERS_TERRAIN 32.0
#define PARALLAX_MIN_LAYERS_OBJECTS 4.0
#define PARALLAX_MAX_LAYERS_OBJECTS 32.0

// --- LOD Fading Distances (in cm) ---
#define PARALLAX_FADE_POM_START 3500.0
#define PARALLAX_FADE_POM_END 4000.0
#define PARALLAX_FADE_ITERATIVE_START 5000.0
#define PARALLAX_FADE_ITERATIVE_END 5500.0

// --- Iterative Parallax Mapping ---
#define PARALLAX_IPM_ITERATIONS_NEAR 4
#define PARALLAX_IPM_ITERATIONS_FAR  1

uniform bool PARALLAX_DEBUG_LOD = false; // Set to true to visualize fade regions


// --- TBN Derivation Modes (from Wareya) ---
#define POM_MODE_BASIC 0 // vertex normal and vertex bi/tangent
#define POM_MODE_EXT1 1  // derive bi/tangent from derivative UV info
#define POM_MODE_EXT2 2  // ext1, but force bi/tangent to be exactly perpendicular to normal (not each other)
#define POM_MODE_EXT3 3  // ext1, but force normal to be perpendicular to triangle (instead of vertex)

// TBN mode for all parallax (POM and simple)
#define POM_TBN_MODE POM_MODE_BASIC

float _pom_det(mat2 matrix)
{
    return matrix[0].x * matrix[1].y - matrix[0].y * matrix[1].x;
}

mat3 _pom_inverse(mat3 matrix)
{
    vec3 row0 = matrix[0];
    vec3 row1 = matrix[1];
    vec3 row2 = matrix[2];

    vec3 minors0 = vec3(
        _pom_det(mat2(row1.y, row1.z, row2.y, row2.z)),
        _pom_det(mat2(row1.z, row1.x, row2.z, row2.x)),
        _pom_det(mat2(row1.x, row1.y, row2.x, row2.y))
    );
    vec3 minors1 = vec3(
        _pom_det(mat2(row2.y, row2.z, row0.y, row0.z)),
        _pom_det(mat2(row2.z, row2.x, row0.z, row0.x)),
        _pom_det(mat2(row2.x, row2.y, row0.x, row0.y))
    );
    vec3 minors2 = vec3(
        _pom_det(mat2(row0.y, row0.z, row1.y, row1.z)),
        _pom_det(mat2(row0.z, row0.x, row1.z, row1.x)),
        _pom_det(mat2(row0.x, row0.y, row1.x, row1.y))
    );

    return transpose(mat3(minors0, minors1, minors2)) / dot(row0, minors0);
}

mat3 calculateTBN(vec3 N, vec3 fragPos, vec2 uv, int mode)
{
    vec3 dpdx = dFdx(fragPos);
    vec3 dpdy = dFdy(fragPos);

    if (mode == POM_MODE_EXT3)
    {
        // use triangle normal instead of vertex normal
        N = normalize(cross(dpdx, dpdy));
    }
    
    vec2 duvdx = dFdx(uv);
    vec2 duvdy = dFdy(uv);
    
    float determinant = duvdx.x * duvdy.y - duvdx.y * duvdy.x;
    if (determinant == 0.0) determinant = 0.00001;
    float invDet = 1.0 / determinant;
    
    vec3 T = (dpdx * duvdy.y - dpdy * duvdx.y) * invDet;
    vec3 B = (dpdy * duvdx.x - dpdx * duvdy.x) * invDet;
    
    if (mode == POM_MODE_EXT2)
    {
        // force T and B to be perpendicular to N (not each other). reduces seams.
        float T_len = length(T);
        float B_len = length(B);
        vec3 T_temp = normalize(cross(N, T));
        vec3 B_temp = normalize(cross(N, B));
        T = -normalize(cross(N, T_temp)) * T_len;
        B = -normalize(cross(N, B_temp)) * B_len;
    }
    
    float x = 1.0 / (min(length(T), length(B)) + 0.00001);
    
    // if the scaled matrix is going to be numerically unstable: don't.
    if (x < 0.00001)
    {
        // unstable if scaled, do not scale
    }
    else
    {
        T *= x;
        B *= x;
    }
    
    return mat3(T, -B, normalize(N));
}

// Shadow offset: transforms parallax UV offset back to view-space for shadow map sampling
// Always uses vertex-interpolated normalToViewMatrix for smooth UV-to-world scaling.
// Triangle-derived TBN (EXT modes) causes grainy noise at triangle edges for shadow coords.
vec3 getParallaxShadowOffset(vec2 offset, vec2 uv, mat3 normalToViewMatrix)
{
    vec3 uvWorld = normalToViewMatrix * vec3(uv, 0.0);
    float _x1 = length(dFdx(uvWorld));
    float _y1 = length(dFdy(uvWorld));
    float _x2 = length(dFdx(passViewPos.xyz));
    float _y2 = length(dFdy(passViewPos.xyz));
    float _d = (_x2 + _y2) / (_x1 + _y1);
    
    vec3 _offs = vec3(offset.x, offset.y, 0.0) * _d;  // +Y for 0.51 tangent space (cross(N,T) binormal convention)
    return normalToViewMatrix * _offs;
}

// Legacy/Simple offset helper (optional, but we'll use POM)
vec2 getParallaxOffsetSimple(vec3 eyeDir, float height, float scale)
{
    return vec2(eyeDir.x, (POM_TBN_MODE == POM_MODE_BASIC) ? eyeDir.y : -eyeDir.y) * (height * scale - scale * 0.5);
}

float CalculateParallaxLOD(float d)
{
    d = smoothstep(9000.0f, 5000.0f, d);
    return 1.0f - d * d;
}

float CalculateParallaxSoftShadow(vec2 uv, vec3 lightDirTangent, sampler2D heightMap, float currentHeight, float scale, float strength)
{
    // lightDirTangent.y = -lightDirTangent.y; // Removed for 0.51: tangent space V axis is natively aligned
    const int numSteps = PARALLAX_SHADOW_STEPS;
    const float numStepsInv = 1.0 / float(numSteps);

    if (lightDirTangent.z > 0.999 || lightDirTangent.z <= 0.0)
        return 1.0;
        
    // Early exit for distant/flat objects to save massive performance
    if (strength <= 0.01) 
        return 1.0;
    
    // Calculate total height left to march to clear the surface
    float remainingHeight = 1.0 - currentHeight;
    if (remainingHeight <= 0.01) return 1.0;
    
    // Prevent infinite shadows at glancing angles by limiting Z div
    float safeZ = max(lightDirTangent.z, 0.01);
    
    // Determine exact steps needed to reach exactly 1.0 boundary
    float heightStep = remainingHeight * numStepsInv;
    vec2 uvStep = (lightDirTangent.xy / safeZ) * scale * heightStep;
    
    float shadow = 1.0;
    
    // Add small bias to prevent self-shadowing acne, bypassing adaptiveBias bug
    float rayHeight = currentHeight + (heightStep * 0.1);
    
    for (int i = 1; i <= numSteps; i++)
    {
        uv += uvStep;
        rayHeight += heightStep;
        
        if (rayHeight > 1.0)
            break;

        float sampledHeight = texture2D(heightMap, uv).a;
        if (sampledHeight > rayHeight)
        {
            float heightDiff = sampledHeight - rayHeight;
            // Soft penumbra physically scales occlusion based on how far along the ray the hit is.
            // Hits right next to the current pixel (i=1) = dark. Hits far away (i=10) = soft.
            float distanceAlongRay = float(i) * numStepsInv;
            float occlusion = (heightDiff / distanceAlongRay) * strength;
            shadow = min(shadow, 1.0 - clamp(occlusion, 0.0, 1.0));
        }
    }
    
    return shadow;
}

// Apply parallax self-shadowing using the smooth vertex-interpolated TBN.
// Triangle-derived TBN (EXT modes) causes grainy noise across edges for raymarched effects.
float ApplyParallaxSelfShadow(vec2 uv, sampler2D heightMap, float parallaxRawHeight, float parallaxScale, float shadowStrength, mat3 normalToViewMatrix)
{
    mat3 ntvt = transpose(normalToViewMatrix);
    vec3 lightDirTangent = ntvt * normalize(lcalcPosition(0));
    
    return CalculateParallaxSoftShadow(uv, lightDirTangent, heightMap, parallaxRawHeight, parallaxScale, shadowStrength);
}

// Calculate terrain parallax AO from heightmap.
// Returns an ambient visibility value (1.0 = fully lit, 0.0 = fully occluded).
float CalculateParallaxAO(vec2 baseUV, sampler2D heightMap, float finalRawHeight, float parallaxScale, float lod)
{
    #if PARALLAX_AO_METHOD == 0
        // --- Parallax AO (Cavity Estimation), GLonD3D12-safe ---
        // No texture2DLod. We emulate the two mip levels with manual ring taps
        // through plain texture2D (auto-LOD), which the D3D12 layer handles fine.

        // Ring radii in UV space. Scaled with parallaxScale so it tracks texel density.
        // BASE ring: tight, blurs away micro-noise (gravel, tiny cracks) ~ old LOD 1.
        // MACRO ring: wide, captures broad valleys vs mounds ~ old LOD 6.
        float aoBaseR  = parallaxScale * 0.6;
        float aoMacroR = parallaxScale * 4.0;

        const vec2 kRing[8] = vec2[8](
            vec2( 1.0,  0.0), vec2(-1.0,  0.0),
            vec2( 0.0,  1.0), vec2( 0.0, -1.0),
            vec2( 0.707,  0.707), vec2(-0.707,  0.707),
            vec2( 0.707, -0.707), vec2(-0.707, -0.707)
        );

        // 1. "Base" height: center + tight ring, averaged -> smooths micro-noise.
        float hBase = texture2D(heightMap, baseUV).a;
        for (int i = 0; i < 8; i++)
            hBase += texture2D(heightMap, baseUV + kRing[i] * aoBaseR).a;
        hBase /= 9.0;

        // 2. "Macro" neighborhood: wide ring averaged -> broad topology.
        float hMacro = 0.0;
        for (int i = 0; i < 8; i++)
            hMacro += texture2D(heightMap, baseUV + kRing[i] * aoMacroR).a;
        hMacro /= 8.0;

        // Calculate difference (only positive values mean we are in a valley)
        float diff = max(0.0, hMacro - hBase);

        float aoStrength = smoothstep(0.01, 0.35, diff);
        float rawAO = 1.0 - (aoStrength * 1.5);
        float finalAO = mix(0.2, 1.0, clamp(rawAO, 0.0, 1.0));
        return mix(finalAO, 1.0, lod);

    #elif PARALLAX_AO_METHOD == 1
        // --- Original LOD cavity AO (uses texture2DLod) ---
        // WARNING: explicit mip fetches break on GLonD3D12 (black/garbage AO).
        // Only use on a native OpenGL driver.

        // 1. Sample LOD 1 as our "Base" height (blurs micro-noise).
        float hBase = texture2DLod(heightMap, baseUV, 1.0).a;

        // 2. Sample LOD 6 for the macro neighborhood (broad topology).
        float hMacro = texture2DLod(heightMap, baseUV, 6.0).a;

        float diff = max(0.0, hMacro - hBase);
        float aoStrength = smoothstep(0.01, 0.35, diff);
        float rawAO = 1.0 - (aoStrength * 1.5);
        float finalAO = mix(0.2, 1.0, clamp(rawAO, 0.0, 1.0));
        return mix(finalAO, 1.0, lod);

    #else // 2 = Rafael PBR
        // Integrate AO from Rafael PBR
        float baseAO = smoothstep(-0.25, 0.5, finalRawHeight);

        const vec2 aoSamples[4] = vec2[4](
            vec2( 1.0,  0.0),
            vec2(-1.0,  0.0),
            vec2( 0.0,  1.0),
            vec2( 0.0, -1.0)
        );

        float aoRadius = parallaxScale * 0.25;
        float aoAccum = 0.0;
        for (int i = 0; i < 4; i++)
        {
            vec2 sampleUV = baseUV + aoSamples[i] * aoRadius;
            aoAccum += max(0.0, texture2D(heightMap, sampleUV).a - finalRawHeight);
        }
        return min(baseAO, mix(exp2(-aoAccum * 3.14159), 1.0, lod));
    #endif
}

// Contact Refinement Parallax adapted from https://www.artstation.com/blogs/andreariccardi/3VPo/a-new-approach-for-parallax-mapping-presenting-the-contact-refinement-parallax-mapping-technique
vec2 ComputeParallaxOffset(
    vec2 baseUV,
    sampler2D heightMap,
    vec3 viewVec,
    mat3 normalToViewMatrix,
    out float outHeight,
    out float outRawHeight,
    out float outScale,
    out float outShadowStrength,
    float viewDist,
    out vec3 debugColor
)
{
    // --- 1. Setup ---
    #if POM_TBN_MODE != POM_MODE_BASIC
        mat3 nm = transpose(_pom_inverse(mat3(gl_ModelViewMatrix)));
        mat3 realTBN = calculateTBN(normalize(nm * passNormal), passViewPos, baseUV, POM_TBN_MODE);
        vec3 eyeDir = _pom_inverse(realTBN) * normalize(-viewVec);
    #else
        mat3 ntvt = transpose(normalToViewMatrix);
        vec3 eyeDir = ntvt * (-viewVec);
    #endif

    float parallaxScale;
    float minLayers;
    float maxLayers;
    float shadowStrength;
    
    #if defined(TERRAIN)
        parallaxScale = PARALLAX_SCALE_TERRAIN;
        shadowStrength = PARALLAX_SHADOW_STRENGTH_TERRAIN;
        minLayers = PARALLAX_MIN_LAYERS_TERRAIN;
        maxLayers = PARALLAX_MAX_LAYERS_TERRAIN;
    #else
        parallaxScale = PARALLAX_SCALE_OBJECTS;
        shadowStrength = PARALLAX_SHADOW_STRENGTH_OBJECTS;
        minLayers = PARALLAX_MIN_LAYERS_OBJECTS;
        maxLayers = PARALLAX_MAX_LAYERS_OBJECTS;
    #endif

    // --- LOD Control Logic ---
    vec2 pomOffset = vec2(0.0);
    vec2 ipmOffsetNear = vec2(0.0);
    vec2 ipmOffsetFar = vec2(0.0);
    vec2 finalOffset = vec2(0.0);
    
    float pomHeight = 1.0;
    float pomRawHeight = 0.5;
    float ipmRawHeightNear = 0.5;
    float ipmRawHeightFar = 0.5;
    
    // Defaults for outputs
    outHeight = 1.0;
    outRawHeight = 0.5;
    outScale = parallaxScale;
    outShadowStrength = shadowStrength;
    debugColor = vec3(0.0, 0.0, 1.0); // Default
    
    // 1. Full POM Pass (uses derivative eyeDir)
    if (viewDist < PARALLAX_FADE_POM_END)
    {
        vec2 P;
        #if defined(TERRAIN)
            float safeZ = max(abs(eyeDir.z), 0.001);
            P = (eyeDir.xy / safeZ) * parallaxScale;
        #else
            float bias = PARALLAX_BIAS;
            P = eyeDir.xy * (parallaxScale / (abs(eyeDir.z) + bias));
        #endif

        float layerCount = mix(maxLayers, minLayers, abs(eyeDir.z));
        vec2 deltaUV = P / layerCount;
        float layerHeightStep = 1.0 / layerCount;
        if (POM_TBN_MODE != POM_MODE_BASIC) deltaUV.y *= -1.0;

        vec2 currentUV = baseUV + deltaUV * layerCount * PARALLAX_NEUTRAL;
        float currentLayerHeight = 1.0; 
        float currentTextureHeight = texture2D(heightMap, currentUV).a;

        for (int i = 0; i < int(layerCount); i++)
        {
            if (currentLayerHeight < currentTextureHeight) break;
            currentUV -= deltaUV; 
            currentLayerHeight -= layerHeightStep;
            currentTextureHeight = texture2D(heightMap, currentUV).a;
        }

        vec2 halfDeltaUV = deltaUV * 0.5;
        float halfLayerHeight = layerHeightStep * 0.5;
        currentUV += halfDeltaUV;
        currentLayerHeight += halfLayerHeight;

        for (int i = 0; i < 5; i++)
        {
            halfDeltaUV *= 0.5;
            halfLayerHeight *= 0.5;
            currentTextureHeight = texture2D(heightMap, currentUV).a;

            if (currentLayerHeight < currentTextureHeight)
            {
                currentUV += halfDeltaUV;
                currentLayerHeight += halfLayerHeight;
            }
            else
            {
                currentUV -= halfDeltaUV;
                currentLayerHeight -= halfLayerHeight;
            }
        }
        
        pomOffset = currentUV - baseUV;
        pomHeight = clamp(currentTextureHeight / PARALLAX_NEUTRAL, 0.0, 1.0);
        pomRawHeight = currentTextureHeight;
    }
    
    // 2. Iterative Parallax Mapping Pass (Near + Far)
    if (viewDist >= PARALLAX_FADE_POM_START)
    {
        vec2 baseP;
        #if defined(TERRAIN)
            float safeZ = max(abs(eyeDir.z), 0.001);
            baseP = (eyeDir.xy / safeZ) * parallaxScale;
        #else
            float bias = PARALLAX_BIAS;
            baseP = eyeDir.xy * (parallaxScale / (abs(eyeDir.z) + bias));
        #endif
        baseP.y *= -1.0;

        // Near IPM
        {
            vec2 currentOffset = vec2(0.0);
            for (int i = 0; i < PARALLAX_IPM_ITERATIONS_NEAR; i++)
            {
                vec4 texSample = texture2D(heightMap, baseUV + currentOffset);
                float heightDiff = texSample.a - PARALLAX_NEUTRAL;
                currentOffset = baseP * heightDiff * texSample.z;
            }
            ipmOffsetNear = currentOffset;
            ipmRawHeightNear = texture2D(heightMap, baseUV + currentOffset).a;
        }

        // Far IPM
        #if PARALLAX_IPM_ITERATIONS_FAR != PARALLAX_IPM_ITERATIONS_NEAR
        {
            vec2 currentOffset = vec2(0.0);
            for (int i = 0; i < PARALLAX_IPM_ITERATIONS_FAR; i++)
            {
                vec4 texSample = texture2D(heightMap, baseUV + currentOffset);
                float heightDiff = texSample.a - PARALLAX_NEUTRAL;
                currentOffset = baseP * heightDiff * texSample.z;
            }
            ipmOffsetFar = currentOffset;
            ipmRawHeightFar = texture2D(heightMap, baseUV + currentOffset).a;
        }
        #else
            ipmOffsetFar = ipmOffsetNear;
            ipmRawHeightFar = ipmRawHeightNear;
        #endif
    }
    
    // --- LOD Blending (3 tiers: POM near, IPM Near mid, IPM Far) ---
    if (viewDist < PARALLAX_FADE_POM_START)
    {
        finalOffset = pomOffset;
        outHeight = pomHeight;
        outRawHeight = pomRawHeight;
        debugColor = vec3(1.0, 0.0, 0.0); // Red = POM
    }
    else if (viewDist < PARALLAX_FADE_POM_END)
    {
        float blend = smoothstep(PARALLAX_FADE_POM_START, PARALLAX_FADE_POM_END, viewDist);
        finalOffset = mix(pomOffset, ipmOffsetNear, blend);
        
        outHeight = mix(pomHeight, clamp(ipmRawHeightNear / PARALLAX_NEUTRAL, 0.0, 1.0), blend);
        outRawHeight = mix(pomRawHeight, ipmRawHeightNear, blend);
        
        debugColor = mix(vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), blend); // Red -> Green
    }
    else if (viewDist < PARALLAX_FADE_ITERATIVE_START)
    {
        finalOffset = ipmOffsetNear;
        outHeight = clamp(ipmRawHeightNear / PARALLAX_NEUTRAL, 0.0, 1.0);
        outRawHeight = ipmRawHeightNear;
        debugColor = vec3(0.0, 1.0, 0.0); // Green = IPM Near
    }
    else if (viewDist < PARALLAX_FADE_ITERATIVE_END)
    {
        float blend = smoothstep(PARALLAX_FADE_ITERATIVE_START, PARALLAX_FADE_ITERATIVE_END, viewDist);
        finalOffset = mix(ipmOffsetNear, ipmOffsetFar, blend);
        
        outHeight = mix(clamp(ipmRawHeightNear / PARALLAX_NEUTRAL, 0.0, 1.0), clamp(ipmRawHeightFar / PARALLAX_NEUTRAL, 0.0, 1.0), blend);
        outRawHeight = mix(ipmRawHeightNear, ipmRawHeightFar, blend);
        
        debugColor = mix(vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0), blend); // Green -> Blue
    }
    else
    {
        finalOffset = ipmOffsetFar;
        outHeight = clamp(ipmRawHeightFar / PARALLAX_NEUTRAL, 0.0, 1.0);
        outRawHeight = ipmRawHeightFar;
        debugColor = vec3(0.0, 0.0, 1.0); // Blue = IPM Far
    }

    return finalOffset;
}

#endif // LIB_MATERIAL_PARALLAX
