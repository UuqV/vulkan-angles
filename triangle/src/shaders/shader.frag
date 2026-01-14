#version 450
layout(location = 0) in vec2 ndc;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Camera
{
    mat4 view;
    mat4 proj;
    mat4 invView;
    mat4 invProj;
} cam;

vec3 getRayDir(vec2 ndc)
{
    vec4 clip = vec4(ndc, -1.0, 1.0);
    vec4 view = cam.invProj * clip;
    view = vec4(view.xy, -1.0, 0.0);

    vec3 worldDir = (cam.invView * view).xyz;
    return normalize(worldDir);
}

vec3 getFisheyeRay(vec2 ndc)
{
    float r = length(ndc);
    if (r > 1.0) discard;

    float theta = r * radians(90.0);  // 180° fisheye
    float phi = atan(ndc.y, ndc.x);

    vec3 dir;
    dir.x = sin(theta) * cos(phi);
    dir.y = sin(theta) * sin(phi);
    dir.z = cos(theta);

    return (cam.invView * vec4(dir, 0)).xyz;
}

struct Tri {
    vec3 a;
    vec3 b;
    vec3 c;
};

layout(set = 0, binding = 1) readonly buffer Tris {
    Tri tris[];
};

bool intersect(vec3 ro, vec3 rd, Tri t, out float dist)
{
    vec3 e1 = t.b - t.a;
    vec3 e2 = t.c - t.a;
    vec3 h = cross(rd, e2);
    float a = dot(e1, h);
    if (abs(a) < 1e-5) return false;

    float f = 1.0 / a;
    vec3 s = ro - t.a;
    float u = f * dot(s, h);
    if (u < 0 || u > 1) return false;

    vec3 q = cross(s, e1);
    float v = f * dot(rd, q);
    if (v < 0 || u + v > 1) return false;

    dist = f * dot(e2, q);
    return dist > 0;
}

void main()
{
    vec3 ro = cam.invView[3].xyz;
    vec3 rd = getFisheyeRay(ndc);

    float closest = 1e30;
    bool hit = false;

    for (int i = 0; i < tris.length(); i++) {
        float d;
        if (intersect(ro, rd, tris[i], d) && d < closest) {
            closest = d;
            hit = true;
        }
    }

    if (!hit) {
        outColor = vec4(0,0,0,1);  // black background
    } else {
        outColor = vec4(1,1,1,1);  // or shaded
    }
}
