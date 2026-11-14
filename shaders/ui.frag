#version 330 core
in vec2 uv;
out vec4 fragColor;
uniform sampler2D surface;
uniform vec4 color;
void main(){fragColor=texture(surface,uv)*color;}
