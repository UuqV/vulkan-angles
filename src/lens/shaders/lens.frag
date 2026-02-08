#version 450

layout(binding = 0) uniform samplerCube envMap;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform LensParams {
    float fov;      // Total FOV in radians (pi = 180 degrees)
    float unused;
    float centerX;
    float centerY;
} lens;

void main() {
    vec2 center = vec2(lens.centerX, lens.centerY);
    vec2 coord = (fragTexCoord - center) * 2.0;  // -1 to 1
    
    float r = length(coord);
    
    if (r > 1.0) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    
    // Fisheye: map radius to angle
    float theta = r * lens.fov * 0.5;  // Angle from forward
    float phi = atan(coord.y, coord.x);  // Angle around
    
    // Spherical to cartesian (Z-up)
    vec3 dir;
    dir.x = sin(theta) * cos(phi);
    dir.y = sin(theta) * sin(phi);
    dir.z = cos(theta);
    
    outColor = texture(envMap, dir);
}