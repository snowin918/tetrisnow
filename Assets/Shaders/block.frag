#version 330 core

// Renders a filled Tetris cell (locked stack or the active piece) as a
// faceted ice cube instead of a flat color square: a rounded-square
// silhouette, an inset bevel groove, a diagonal glossy highlight, a small
// sparkle glint, and a little frost grain. Shares Assets/Shaders/quad.vert
// with the plain "quad" shader (same vertex layout/uniforms), so vUV is
// already the quad-local [0,1] coordinate — uUvOffset/uUvScale are left at
// (0,0)/(1,1) for this program (see Renderer::initialize()) since blocks
// never sample a texture atlas.

in vec2 vUV;
out vec4 FragColor;

uniform vec4 uTint;

// Cheap hash for a bit of per-pixel frost grain — no texture lookup needed.
float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void main()
{
    vec2 uv = vUV;
    vec2 p = uv - 0.5;

    // Rounded-square silhouette so adjacent blocks read as individual
    // cubes rather than one flat mass.
    const float radius = 0.10;
    vec2 d = abs(p) - vec2(0.5 - radius);
    float corner = length(max(d, vec2(0.0))) - radius;
    float shapeAlpha = 1.0 - smoothstep(-0.015, 0.015, corner);

    // Soft volumetric shading: brighter near the top face, darker toward
    // the bottom, like light catching the top of a cube.
    vec3 color = uTint.rgb * mix(1.2, 0.8, uv.y);

    // Inset bevel groove near the edge, darker than the face.
    float distFromEdge = min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y));
    float bevel = smoothstep(0.0, 0.11, distFromEdge);
    color = mix(color * 0.55, color, bevel);

    // Diagonal glossy highlight streak, like light reflecting off ice.
    float diag = uv.x - uv.y;
    float band = clamp(1.0 - abs(diag - 0.32) * 4.5, 0.0, 1.0);
    color += vec3(1.0) * band * 0.30;

    // A small sparkle glint near the upper-left corner.
    float glint = smoothstep(0.09, 0.0, length(uv - vec2(0.26, 0.22)));
    color += vec3(1.0) * glint * 0.55;

    // Faint frost grain so faces aren't perfectly flat.
    color += (hash(floor(uv * 20.0)) - 0.5) * 0.05;

    FragColor = vec4(color, uTint.a * shapeAlpha);
}
