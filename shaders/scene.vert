#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
uniform mat4 model, viewProjection, lightMatrix;
out vec3 position, normal;
out vec2 uv;
out vec4 shadowPosition;
void main(){
    vec4 p=model*vec4(aPos,1.0);
    position=p.xyz;normal=mat3(transpose(inverse(model)))*aNormal;
    uv=aUV;shadowPosition=lightMatrix*p;gl_Position=viewProjection*p;
}
