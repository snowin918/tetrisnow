#version 330 core

// A soft, round, faintly glowing disc instead of a flat quad — shared by
// every particle (ambient snow, clear shards, dust, puffs, trails) and
// flying attack projectiles alike, replacing the hard-edged square
// drawQuad() previously used for those. Shares Assets/Shaders/quad.vert
// (same vertex layout/uniforms as the other shaders here), so vUV is
// already the quad-local [0,1] coordinate.

in vec2 vUV;
out vec4 FragColor;

uniform vec4 uTint;

void main()
{
    vec2 p = vUV - 0.5;
    float dist = length(p) * 2.0; // 0 at center, 1 at the quad's inscribed-circle edge

    // Soft, anti-aliased circular silhouette — no hard square corners.
    float shapeAlpha = 1.0 - smoothstep(0.75, 1.0, dist);
    if (shapeAlpha <= 0.0) {
        discard;
    }

    // Brighter core fading toward the rim, like a lit snowball/ice mote
    // rather than a flat disc.
    vec3 color = uTint.rgb * mix(1.35, 0.7, clamp(dist, 0.0, 1.0));

    // A brighter hot core for a glossy, rounded feel — centered (radially
    // symmetric in `dist`) rather than offset to one side. An off-center
    // version previously caused a visible strobe/"blink" on any shape that
    // both rotates and is non-circular (elongated), like the flying attack
    // bullets tracking their curved flight path: a fixed offset point in
    // local UV space sweeps asymmetrically once the quad is stretched
    // non-uniformly and spun, since this shader has no notion of the
    // world-space aspect ratio to compensate for. A centered highlight is
    // rotation-invariant by construction, so it can't do that.
    float highlight = 1.0 - smoothstep(0.0, 0.35, dist);
    color += vec3(1.0) * highlight * 0.35;

    // Faint outward glow just past the hard edge, so it reads as soft
    // light rather than a die-cut sticker.
    float glow = (1.0 - smoothstep(0.6, 1.0, dist)) * 0.25;
    color += uTint.rgb * glow;

    FragColor = vec4(color, uTint.a * shapeAlpha);
}
