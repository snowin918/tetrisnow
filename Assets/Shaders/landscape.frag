#version 330 core
in vec2 vUV;
out vec4 FragColor;

float hash(float n) { return fract(sin(n * 127.1) * 43758.5453); }
float noise(float x)
{
    float i = floor(x);
    float f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), f);
}

void main()
{
    vec2 uv = vUV;
    vec3 zenith = vec3(0.018, 0.045, 0.095);
    vec3 horizon = vec3(0.13, 0.29, 0.36);
    vec3 color = mix(zenith, horizon, pow(uv.y, 1.35));

    // Fine stars, concentrated above the horizon.
    vec2 starCell = floor(uv * vec2(260.0, 150.0));
    float starSeed = hash(starCell.x + starCell.y * 317.0);
    float star = step(0.988, starSeed) * (1.0 - smoothstep(0.0, 0.60, uv.y));
    color += star * vec3(0.60, 0.82, 1.0) * (0.35 + hash(starCell.y + starCell.x) * 0.65);

    // Layered aurora ribbons with a bright core and soft vertical curtains.
    float wave1 = 0.20 + sin(uv.x * 8.0 + sin(uv.x * 2.7) * 1.8) * 0.045;
    float wave2 = 0.30 + sin(uv.x * 10.5 + 1.7) * 0.055;
    float band1 = exp(-abs(uv.y - wave1) * 34.0);
    float band2 = exp(-abs(uv.y - wave2) * 29.0);
    float curtain = (0.35 + 0.65 * sin(uv.x * 82.0) * sin(uv.x * 82.0));
    color += vec3(0.10, 0.68, 0.52) * band1 * curtain * 0.34;
    color += vec3(0.16, 0.48, 0.78) * band2 * (1.0 - curtain * 0.4) * 0.24;

    // Moon glow behind the central mountain pass.
    vec2 moonP = (uv - vec2(0.50, 0.27)) * vec2(1.0, 1.8);
    float moonDist = length(moonP);
    color += vec3(0.38, 0.56, 0.68) * (1.0 - smoothstep(0.02, 0.18, moonDist)) * 0.28;
    color = mix(color, vec3(0.82, 0.92, 0.94), 1.0 - smoothstep(0.045, 0.052, moonDist));

    // Three crisp mountain ranges; snow catches the moon on the left slopes.
    for (int layer = 0; layer < 3; ++layer) {
        float l = float(layer);
        float x = uv.x * (8.0 + l * 4.0);
        float ridge = 0.48 + l * 0.105 - noise(x + l * 23.0) * (0.15 - l * 0.018);
        ridge += noise(x * 4.7 + 11.0) * 0.018;
        float mask = smoothstep(ridge - 0.002, ridge + 0.002, uv.y);
        float slope = noise(x * 5.0 + l * 7.0);
        vec3 rock = mix(vec3(0.17, 0.28, 0.34), vec3(0.045, 0.12, 0.17), l / 2.0);
        float snow = (1.0 - smoothstep(0.014, 0.065, uv.y - ridge)) * smoothstep(0.35, 0.8, slope);
        vec3 mountain = mix(rock, vec3(0.53, 0.68, 0.72), snow * (0.9 - l * 0.2));
        color = mix(color, mountain, mask);
    }

    // Quiet frozen valley with subtle reflection bands.
    float ground = 0.78 + sin(uv.x * 13.0) * 0.012 + sin(uv.x * 31.0) * 0.007;
    float groundMask = smoothstep(ground, ground + 0.003, uv.y);
    vec3 valley = mix(vec3(0.20, 0.38, 0.43), vec3(0.48, 0.65, 0.67), uv.y);
    valley += sin(uv.y * 250.0 + uv.x * 9.0) * 0.012;
    color = mix(color, valley, groundMask);

    // Dark pine silhouettes ground the scene without competing with the boards.
    float treeX = uv.x * 120.0;
    float treeId = floor(treeX);
    float treeH = 0.035 + hash(treeId + 91.0) * 0.075;
    float treeBase = 0.81 + hash(treeId * 2.0) * 0.018;
    float treeY = (uv.y - treeBase + treeH) / treeH;
    float branches = (0.08 + treeY * 0.38) * (0.72 + 0.28 * fract(treeY * 5.0));
    float tree = step(0.0, treeY) * step(treeY, 1.0)
        * (1.0 - smoothstep(branches, branches + 0.025, abs(fract(treeX) - 0.5)));
    color = mix(color, vec3(0.025, 0.10, 0.105), tree * 0.9);

    FragColor = vec4(color, 1.0);
}
