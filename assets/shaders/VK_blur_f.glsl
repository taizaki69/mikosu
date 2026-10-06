#version 450

// One pass of a separable gaussian blur (sigma 4 texels, 17 texels wide, 9 fetches through linear filtering).
// Used once per background change to build the frosted-glass backdrop, never per frame.

layout(location = 0) in vec2 fragTexcoord;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D tex0;

layout(std140, set = 3, binding = 0) uniform BlurParams {
    vec4 blur_step;  // xy: one texel along the blur direction, in texture coordinates
} bp;

void main() {
    vec2 s = bp.blur_step.xy;
    vec4 c = texture(tex0, fragTexcoord) * 0.1414;
    c += (texture(tex0, fragTexcoord + s * 1.4533) + texture(tex0, fragTexcoord - s * 1.4533)) * 0.2429;
    c += (texture(tex0, fragTexcoord + s * 3.3922) + texture(tex0, fragTexcoord - s * 3.3922)) * 0.1326;
    c += (texture(tex0, fragTexcoord + s * 5.3348) + texture(tex0, fragTexcoord - s * 5.3348)) * 0.0445;
    c += (texture(tex0, fragTexcoord + s * 7.2826) + texture(tex0, fragTexcoord - s * 7.2826)) * 0.0092;
    outColor = vec4(c.rgb, 1.0);
}
