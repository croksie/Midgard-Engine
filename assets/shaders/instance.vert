#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aCol;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4 aInstanceModel; 

layout (location = 0) out vec3 ourColor;
layout (location = 1) out vec2 TexCoord;

layout (std140, set = 0, binding = 0) uniform GlobalData {
    mat4 view;
    mat4 projection;
} u_Global;

void main() {
    gl_Position = u_Global.projection * u_Global.view * aInstanceModel * vec4(aPos, 1.0);
    ourColor = aCol;
    TexCoord = aTexCoord;
}