#version 450

// One antialiased UI shape per draw: a rounded rectangle (per-corner radii) with an optional diagonal cut at its
// right end, filled or as a border, with soft edges for shadows and glows, and optionally a texture under the
// colour (frosted glass over the blurred background, rounded thumbnails). Coordinates come from the quad's
// texcoords (0..1 across it), so the shader doesn't depend on the backend's framebuffer origin.

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexcoord;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D tex0;

layout(std140, set = 3, binding = 0) uniform ShapeParams {
    vec4 quad;   // xy: size of the drawn quad (px), zw: the shape's top-left inside the quad (px)
    vec4 shape;  // xy: shape size (px), z: edge softness (px; ~1 antialiases, more gives a shadow or glow), w: border width (0 = filled)
    vec4 radii;  // corner radii (px): top-left, top-right, bottom-right, bottom-left
    vec4 extra;  // x: diagonal cut at the right end (how much shorter the bottom edge is, px), y: 1 = texture under the colour,
                 // z: diagonal cut at the left end (how much shorter the top edge is, px): both cuts lean like '/'
    vec4 uvmap;  // texture coordinates: xy at the quad's top-left, zw across the whole quad
    vec4 col;    // colour multiplier (the engine's current colour)
} sp;

float sd_round_box(vec2 p, vec2 b, vec4 r) {
    // p relative to the centre (y down), b the half size, r = (top-left, top-right, bottom-right, bottom-left)
    float rad = (p.x > 0.0) ? ((p.y > 0.0) ? r.z : r.y) : ((p.y > 0.0) ? r.w : r.x);
    vec2 q = abs(p) - b + rad;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - rad;
}

void main() {
    vec2 px = fragTexcoord * sp.quad.xy - sp.quad.zw;
    vec2 half_size = sp.shape.xy * 0.5;
    float d = sd_round_box(px - half_size, half_size, sp.radii);

    if (sp.extra.x > 0.0) {
        // the right edge runs from the top-right corner down to (width - cut, height)
        vec2 n = normalize(vec2(sp.shape.y, sp.extra.x));
        d = max(d, dot(px - vec2(sp.shape.x, 0.0), n));
    }
    if (sp.extra.z > 0.0) {
        // the left edge runs from the bottom-left corner up to (cut, 0)
        vec2 n = normalize(vec2(-sp.shape.y, -sp.extra.z));
        d = max(d, dot(px - vec2(0.0, sp.shape.y), n));
    }

    float soft = max(sp.shape.z, 0.75);
    float cover = 1.0 - smoothstep(-0.5 * soft, 0.5 * soft, d);
    if (sp.shape.w > 0.0) {
        cover *= smoothstep(-sp.shape.w - 0.5 * soft, -sp.shape.w + 0.5 * soft, d);
    }

    vec4 c = fragColor;
    float a;
    if (sp.extra.y > 0.5) {
        vec4 t = texture(tex0, sp.uvmap.xy + fragTexcoord * sp.uvmap.zw);
        c.rgb = mix(t.rgb, c.rgb, c.a);
        a = t.a;
    } else {
        a = c.a;
    }
    outColor = vec4(c.rgb * sp.col.rgb, a * sp.col.a * cover);
}
