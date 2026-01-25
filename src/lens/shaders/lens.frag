#version 450

layout(binding = 0) uniform sampler2D screenTexture;
layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform LensParams {
    float k1;
    float k2;
    float centerX;
    float centerY;
} lens;

void main() {
    vec2 center = vec2(lens.centerX, lens.centerY);
    vec2 coord = fragTexCoord - center;
    
    float r2 = dot(coord, coord);
    float r4 = r2 * r2;
    float distortion = 1.0 + lens.k1 * r2 + lens.k2 * r4;
    
    vec2 distorted = center + coord * distortion;
    
    if (distorted.x < 0.0 || distorted.x > 1.0 || 
        distorted.y < 0.0 || distorted.y > 1.0) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        outColor = texture(screenTexture, distorted);
    }
}