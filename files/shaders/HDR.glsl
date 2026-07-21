// Convert colour textures from display/sRGB-like space to linear space before lighting.
// Final tone mapping and gamma encoding are performed once by postprocess_fragment.glsl.
vec3 preLight(vec3 x)
{
    return pow(max(x, vec3(0.0)), vec3(2.2));
}
