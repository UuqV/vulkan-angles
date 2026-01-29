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

const float renderScale = 2.0;

void main() {
    vec2 center = vec2(0.5, 0.5);
    vec2 coord = fragTexCoord - center;  // -0.5 to 0.5
    
    float r2 = dot(coord, coord) * 4.0;  // Normalize so corners = 1
    float r4 = r2 * r2;
    float distortion = 1.0 + lens.k1 * r2 + lens.k2 * r4;
    
    float edgeDistortion = 1.0 + lens.k1 + lens.k2;
    vec2 distorted = coord * distortion / edgeDistortion;
    
    // Map to center of 2x texture
    vec2 uv = distorted / renderScale + 0.5;
    
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        outColor = texture(screenTexture, uv);
    }

}