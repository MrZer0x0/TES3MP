#version 120

// Интеграция файла с функциями волн и каустики
#include "water_waves.glsl"
    
varying vec3  screenCoordsPassthrough;
varying vec4  position;
varying float linearDepth;
uniform float osg_SimulationTime;
uniform mat4 osg_ViewMatrixInverse;
uniform bool isInterior;
uniform float waterWaveStrength;
uniform float waterWaveChoppiness;
uniform float waterLargeWaveScale;
uniform float waterMediumWaveScale;
uniform float waterSmallWaveScale;

#include "shadows_vertex.glsl"

void main(void)
{
    vec4 glvertice = gl_Vertex;
    
    vec4 campos = osg_ViewMatrixInverse * vec4(0.0, 0.0, 0.0, 1.0);
    vec4 viewPos = gl_ModelViewMatrix * gl_Vertex;
    float euclideanDepth = length(viewPos.xyz);
    
    float frequency = 2.0 * 3.1415 / 0.1;
    float phase = 0.01 * frequency;

    // Three travelling wave trains replace the old single-direction sine.
    // All layers are kept below the cell water level, so shore geometry does
    // not flicker through upward-displaced vertices. The distance fade avoids
    // a visible hard edge at the geometry-wave limit.
    if (euclideanDepth < 600000.0 && !isInterior)
    {
        glvertice.xy *= 0.03;

        float t = osg_SimulationTime * phase;
        float largeWave = 0.5 + 0.5 * sin(
            dot(vec2(0.63, 0.38), glvertice.xy) * frequency * 0.58 * waterLargeWaveScale + t * 0.72);
        float mediumWave = 0.5 + 0.5 * sin(
            dot(vec2(-0.31, 0.95), glvertice.xy) * frequency * 1.13 * waterMediumWaveScale + t * 1.07 + 1.7);
        float smallWave = 0.5 + 0.5 * sin(
            dot(vec2(0.91, -0.42), glvertice.xy) * frequency * 2.05 * waterSmallWaveScale + t * 1.43 + 3.1);

        largeWave = pow(largeWave, mix(1.55, 3.10, clamp(waterWaveChoppiness * 0.4, 0.0, 1.0)));
        mediumWave = pow(mediumWave, 2.05);
        smallWave = pow(smallWave, 1.65);

        float waveShape = largeWave * 0.58 + mediumWave * 0.28 + smallWave * 0.14;
        float distanceFade = 1.0 - smoothstep(420000.0, 600000.0, euclideanDepth);
        float amplitude = 6.25 * waterWaveStrength * waterWaveChoppiness * distanceFade;
        glvertice.z -= amplitude * waveShape;
    }
    
    if(campos.z < -1.0)
        glvertice.z += 3.25;  // было 12.5, уменьшено в 2 раза для консистентности
    
    
    viewPos = gl_ModelViewMatrix * glvertice;
    gl_Position = gl_ModelViewProjectionMatrix * glvertice;

    mat4 scalemat = mat4(0.5, 0.0, 0.0, 0.0,
                         0.0, -0.5, 0.0, 0.0,
                         0.0, 0.0, 0.5, 0.0,
                         0.5, 0.5, 0.5, 1.0);

    vec4 texcoordProj = ((scalemat) * ( gl_Position));
    screenCoordsPassthrough = texcoordProj.xyw -
        vec3(0.0,0.0,0.0);

    position = glvertice;

    linearDepth = gl_Position.z;

#if (@shadows_enabled)
    vec3 viewNormal = normalize((gl_NormalMatrix * gl_Normal).xyz);
    setupShadowCoords(viewPos, viewNormal);
#endif
}
