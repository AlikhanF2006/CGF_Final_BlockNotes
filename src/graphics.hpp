#pragma once
#include "font.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <string>
namespace bn {
struct Vertex {
    glm::vec3 p, n;
    glm::vec2 uv;
};
struct Mesh {
    GLuint vao = 0, vbo = 0;
    int count = 0;
    void set(const std::vector<Vertex> &vertices);
    void draw() const;
    void release();
};
struct Draw {
    Mesh *mesh;
    glm::mat4 model;
    GLuint tex;
    glm::vec3 tint{1};
    GLuint back = 0;
    float emission = 0;
    bool casts = true;
};
class Graphics {
    GLuint sceneShader = 0, depthShader = 0, uiShader = 0, shadowFbo = 0, shadowTex = 0;
    Mesh uiMesh;
    GLint uiColor = -1;

  public:
    Mesh cube, plane, curl;
    bool cullingOn = true; // back-face culling for solid cubes
    GLuint white = 0, wood = 0, leather = 0;
    glm::mat4 vp{1}, lightMatrix{1};
    glm::vec3 eye{0, -2, 10}, light{-3, 4, 7};
    int pixelW = 1440, pixelH = 900;
    void init(const std::string &dir);
    void render(const std::vector<Draw> &draws, bool lighting, bool shadows);
    void beginUI();
    void rect(float x, float y, float w, float h, glm::vec4 color, GLuint tex = 0,
              glm::vec4 uv = {0, 0, 1, 1});
    void text(Font &font, const std::u32string &value, float x, float y, float size, glm::vec4 color);
    void text(Font &font, const std::string &value, float x, float y, float size, glm::vec4 color) {
        text(font, decode(value), x, y, size, color);
    }
    void updateCurl(float t, bool backward);
    void screenshot(const std::string &path);
    void release();
};
glm::mat4 transform(glm::vec3 p, glm::vec3 scale);
} // namespace bn
