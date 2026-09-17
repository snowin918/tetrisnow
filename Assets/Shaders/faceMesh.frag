#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTexture;

// Phase 3's per-emotion "shader effects": a multiplicative color tint
// (e.g. Frozen's blue cast, Victory's warm glow) and a contrast knob
// (e.g. Angry's harsher look, Defeat's washed-out look). Neutral is
// tint (1,1,1) / contrast 1.0, i.e. a no-op.
uniform vec3 uTintColor;
uniform float uContrast;

void main()
{
    vec4 sample = texture(uTexture, vUV);
    vec3 color = (sample.rgb - 0.5) * uContrast + 0.5;
    color *= uTintColor;
    FragColor = vec4(color, sample.a);
}
