#version 450 core
layout (location = 0) in vec3 ourColor;
layout (location = 1) in vec2 TexCoord;

layout (location = 0) out vec4 FragColor;

layout (set = 1, binding = 0) uniform sampler2D u_Texture;
void main() {
    FragColor = texture(u_Texture, TexCoord) * vec4(ourColor, 1.0);
}