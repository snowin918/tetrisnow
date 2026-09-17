#version 330 core
in vec2 vUV;
out vec4 FragColor;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main()
{
    vec2 uv = vUV;
    float edge = min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y));
    float cloudy = sin(uv.x * 17.0 + sin(uv.y * 9.0)) * sin(uv.y * 22.0 - uv.x * 4.0);
    cloudy = smoothstep(0.15, 0.95, cloudy * 0.5 + 0.5);
    float grain = hash(floor(uv * vec2(70.0, 120.0)));
    float veinA = 1.0 - smoothstep(0.0, 0.014, abs(uv.x - 0.24 - sin(uv.y * 13.0) * 0.025));
    float veinB = 1.0 - smoothstep(0.0, 0.011, abs(uv.y - 0.68 - sin(uv.x * 21.0) * 0.018));
    float rim = 1.0 - smoothstep(0.015, 0.055, edge);
    vec3 color = vec3(0.16, 0.39, 0.47);
    color += cloudy * vec3(0.06, 0.13, 0.15);
    color += (veinA + veinB) * vec3(0.35, 0.68, 0.72);
    color += rim * vec3(0.50, 0.88, 0.92);
    color += (grain - 0.5) * 0.025;
    float alpha = 0.34 + cloudy * 0.08 + rim * 0.34 + (veinA + veinB) * 0.12;
    FragColor = vec4(color, alpha);
}
