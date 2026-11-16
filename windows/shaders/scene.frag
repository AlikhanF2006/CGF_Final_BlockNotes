#version 330 core
in vec3 position,normal;
in vec2 uv;
in vec4 shadowPosition;
out vec4 fragColor;
uniform sampler2D surface,shadowMap,backSurface;
uniform vec3 tint,eye,lightPos;
uniform bool lighting,shadows,twoSided;
uniform float emission;
float shadowFactor(vec3 n,vec3 l){
    vec3 p=shadowPosition.xyz/shadowPosition.w*0.5+0.5;
    if(p.z>1.0||p.z<0.0||p.x<0.0||p.x>1.0||p.y<0.0||p.y>1.0)return 0.0;
    float bias=max(0.002*(1.0-dot(n,l)),0.0005);
    vec2 texel=1.0/vec2(textureSize(shadowMap,0));float s=0.0;
    for(int x=-1;x<=1;x++)for(int y=-1;y<=1;y++)s+=p.z-bias>texture(shadowMap,p.xy+vec2(x,y)*texel).r?1.0:0.0;
    return s/9.0;
}
void main(){
    vec3 n=normalize(normal);if(!gl_FrontFacing)n=-n;
    vec3 base=texture(surface,uv).rgb*tint;
    if(twoSided&&!gl_FrontFacing)base=texture(backSurface,vec2(1.0-uv.x,uv.y)).rgb*tint;
    vec3 l=normalize(lightPos-position),v=normalize(eye-position);
    float diffuse=max(dot(n,l),0.0);
    float spec=pow(max(dot(n,normalize(l+v)),0.0),32.0)*0.10;
    float shadow=shadows?shadowFactor(n,l):0.0;
    vec3 color=lighting?base*(0.54+(1.0-shadow)*0.52*diffuse)+(1.0-shadow)*spec:base;
    color+=base*emission;fragColor=vec4(color,1.0);
}
