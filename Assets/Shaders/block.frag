#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform vec4 uTint;

void main()
{
    vec2 p = vUV - 0.5;
    vec2 q = abs(p) - vec2(0.445);
    float sd = length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - 0.035;
    float aa = max(fwidth(sd), 0.001);
    float alpha = 1.0 - smoothstep(-aa, aa, sd);
    float edge = min(min(vUV.x, 1.0-vUV.x), min(vUV.y, 1.0-vUV.y));
    float face = smoothstep(0.035, 0.105, edge);
    // A cool deep bevel, bright upper rim, and broad translucent inner face.
    vec3 body = uTint.rgb * mix(1.05, 0.68, vUV.y);
    vec3 rim = uTint.rgb * (vUV.x + vUV.y < 1.0 ? 1.28 : 0.46);
    vec3 color = mix(rim, body, face);
    float reflection = 1.0 - smoothstep(0.025, 0.12, abs(vUV.x * 0.65 + vUV.y - 0.42));
    color = mix(color, vec3(0.83, 0.95, 1.0), reflection * face * 0.24);
    float lip = (1.0-smoothstep(0.012, 0.035, abs(vUV.y-0.085)))
        * smoothstep(0.07, 0.15, vUV.x) * (1.0-smoothstep(0.80, 0.93, vUV.x));
    color += vec3(0.30, 0.38, 0.40) * lip;
    FragColor = vec4(color, uTint.a * alpha);
}
