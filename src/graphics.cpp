#include "graphics.hpp"
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>
namespace bn {
namespace {
GLuint compile(GLenum type, const std::string &file) {
    std::ifstream in(file);
    if (!in)
        throw std::runtime_error("Shader missing: " + file);
    std::ostringstream s;
    s << in.rdbuf();
    auto source = s.str();
    const char *ptr = source.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &ptr, nullptr);
    glCompileShader(shader);
    int ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(shader, 4096, nullptr, log);
        throw std::runtime_error(file + ": " + log);
    }
    return shader;
}
GLuint program(const std::string &path, const std::string &name) {
    auto vs = compile(GL_VERTEX_SHADER, path + "/" + name + ".vert"),
         fs = compile(GL_FRAGMENT_SHADER, path + "/" + name + ".frag");
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    glDeleteShader(vs);
    glDeleteShader(fs);
    int ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok)
        throw std::runtime_error("Shader link failed: " + name);
    return p;
}
void matrix(GLuint p, const char *name, const glm::mat4 &m) {
    glUniformMatrix4fv(glGetUniformLocation(p, name), 1, GL_FALSE, glm::value_ptr(m));
}
void integer(GLuint p, const char *name, int n) {
    glUniform1i(glGetUniformLocation(p, name), n);
}
void vector3(GLuint p, const char *name, glm::vec3 v) {
    glUniform3fv(glGetUniformLocation(p, name), 1, glm::value_ptr(v));
}
void quad(std::vector<Vertex> &out, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 n) {
    Vertex va{a, n, {0, 1}}, vb{b, n, {1, 1}}, vc{c, n, {1, 0}}, vd{d, n, {0, 0}};
    out.insert(out.end(), {va, vb, vc, va, vc, vd});
}
} // namespace
glm::mat4 transform(glm::vec3 p, glm::vec3 scale) {
    return glm::scale(glm::translate(glm::mat4(1), p), scale);
}
void Mesh::set(const std::vector<Vertex> &v) {
    if (!vao) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
    }
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(v.size() * sizeof(Vertex)), v.data(), GL_DYNAMIC_DRAW);
    count = int(v.size());
    for (int i = 0; i < 3; i++)
        glEnableVertexAttribArray(i);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, p));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, n));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, uv));
}
void Mesh::draw() const {
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, count);
}
void Mesh::release() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    vao = vbo = 0;
}
void Graphics::init(const std::string &dir) {
    sceneShader = program(dir, "scene");
    depthShader = program(dir, "depth");
    uiShader = program(dir, "ui");
    uiColor = glGetUniformLocation(uiShader, "color");
    std::vector<Vertex> v;
    quad(v, {-.5, -.5, .5}, {.5, -.5, .5}, {.5, .5, .5}, {-.5, .5, .5}, {0, 0, 1});
    quad(v, {.5, -.5, -.5}, {-.5, -.5, -.5}, {-.5, .5, -.5}, {.5, .5, -.5}, {0, 0, -1});
    quad(v, {-.5, -.5, -.5}, {-.5, -.5, .5}, {-.5, .5, .5}, {-.5, .5, -.5}, {-1, 0, 0});
    quad(v, {.5, -.5, .5}, {.5, -.5, -.5}, {.5, .5, -.5}, {.5, .5, .5}, {1, 0, 0});
    quad(v, {-.5, .5, .5}, {.5, .5, .5}, {.5, .5, -.5}, {-.5, .5, -.5}, {0, 1, 0});
    quad(v, {-.5, -.5, -.5}, {.5, -.5, -.5}, {.5, -.5, .5}, {-.5, -.5, .5}, {0, -1, 0});
    cube.set(v);
    v.clear();
    quad(v, {-.5, -.5, 0}, {.5, -.5, 0}, {.5, .5, 0}, {-.5, .5, 0}, {0, 0, 1});
    plane.set(v);
    white = texture(Canvas(1, 1, {1, 1, 1}));
    Canvas woodCanvas(128, 128, {.28f, .20f, .13f}), leatherCanvas(64, 64, {.5f, .5f, .5f});
    unsigned rng = 17;
    for (int y = 0; y < 128; y += 4)
        for (int x = 0; x < 128; x += 4) {
            rng = rng * 1664525 + 1013904223;
            float n = ((rng >> 24) & 255) / 255.f;
            woodCanvas.rect(x, y, 4, 4, {.48f, .34f, .20f}, n * .22f);
        }
    for (int y = 0; y < 128; y += 32)
        woodCanvas.rect(0, y, 128, 2, {.13f, .09f, .06f});
    wood = texture(woodCanvas, 0, true);
    for (int y = 0; y < 64; y += 4)
        for (int x = 0; x < 64; x += 4) {
            rng = rng * 1664525 + 1013904223;
            leatherCanvas.rect(x, y, 4, 4, {.95f, .95f, .95f}, ((rng >> 24) & 255) / 1100.f);
        }
    leather = texture(leatherCanvas, 0, true);
    glGenFramebuffers(1, &shadowFbo);
    glGenTextures(1, &shadowTex);
    glBindTexture(GL_TEXTURE_2D, shadowTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 2048, 2048, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Shadow framebuffer incomplete");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void Graphics::render(const std::vector<Draw> &draws, bool lighting, bool shadows) {
    lightMatrix = glm::ortho(-7.f, 7.f, -7.f, 7.f, 1.f, 22.f) *
                  glm::lookAt(light, glm::vec3(0), glm::vec3(0, 1, 0));
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    // Back-face culling: solid cubes only. Paper sheets (plane/curl) are seen from both sides
    // while a page turns, so culling stays off for them. The shadow pass draws all faces.
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glDisable(GL_CULL_FACE);
    if (shadows && lighting) {
        glViewport(0, 0, 2048, 2048);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);
        glClear(GL_DEPTH_BUFFER_BIT);
        glUseProgram(depthShader);
        matrix(depthShader, "lightMatrix", lightMatrix);
        for (auto &d : draws)
            if (d.casts) {
                matrix(depthShader, "model", d.model);
                d.mesh->draw();
            }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    glViewport(0, 0, pixelW, pixelH);
    glClearColor(.075f, .105f, .085f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(sceneShader);
    matrix(sceneShader, "viewProjection", vp);
    matrix(sceneShader, "lightMatrix", lightMatrix);
    vector3(sceneShader, "eye", eye);
    vector3(sceneShader, "lightPos", light);
    integer(sceneShader, "lighting", lighting);
    integer(sceneShader, "shadows", shadows && lighting);
    integer(sceneShader, "surface", 0);
    integer(sceneShader, "shadowMap", 1);
    integer(sceneShader, "backSurface", 2);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowTex);
    for (auto &d : draws) {
        if (d.mesh == &cube && cullingOn)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);
        matrix(sceneShader, "model", d.model);
        vector3(sceneShader, "tint", d.tint);
        integer(sceneShader, "twoSided", d.back != 0);
        glUniform1f(glGetUniformLocation(sceneShader, "emission"), d.emission);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, d.tex ? d.tex : white);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, d.back ? d.back : white);
        d.mesh->draw();
    }
    glDisable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE0);
}
void Graphics::beginUI() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(uiShader);
    matrix(uiShader, "projection", glm::ortho(0.f, 1440.f, 900.f, 0.f));
    integer(uiShader, "surface", 0);
    glActiveTexture(GL_TEXTURE0);
}
void Graphics::rect(float x, float y, float w, float h, glm::vec4 color, GLuint tex, glm::vec4 uv) {
    glm::vec3 n{0, 0, 1};
    std::vector<Vertex> v = {{{x, y, 0}, n, {uv.x, uv.y}},         {{x + w, y, 0}, n, {uv.z, uv.y}},
                             {{x + w, y + h, 0}, n, {uv.z, uv.w}}, {{x, y, 0}, n, {uv.x, uv.y}},
                             {{x + w, y + h, 0}, n, {uv.z, uv.w}}, {{x, y + h, 0}, n, {uv.x, uv.w}}};
    uiMesh.set(v);
    glUniform4fv(uiColor, 1, glm::value_ptr(color));
    glBindTexture(GL_TEXTURE_2D, tex ? tex : white);
    uiMesh.draw();
}
void Graphics::text(Font &font, const std::u32string &value, float x, float y, float size,
                    glm::vec4 color) {
    float scale = size / 24.f, original = x;
    std::vector<Vertex> v;
    for (char32_t c : value) {
        if (c == U'\n') {
            x = original;
            y += size * 1.4f;
            continue;
        }
        auto it = font.glyphs.find(c);
        if (it == font.glyphs.end())
            it = font.glyphs.find(U'?');
        auto g = it->second;
        float px = x + g.ox * scale, py = y + size + g.oy * scale, w = g.w * scale, h = g.h * scale;
        glm::vec3 n{0, 0, 1};
        float u = g.x / 1024.f, t = g.y / 1024.f, uu = (g.x + g.w) / 1024.f, tt = (g.y + g.h) / 1024.f;
        v.insert(v.end(), {{{px, py, 0}, n, {u, t}},
                           {{px + w, py, 0}, n, {uu, t}},
                           {{px + w, py + h, 0}, n, {uu, tt}},
                           {{px, py, 0}, n, {u, t}},
                           {{px + w, py + h, 0}, n, {uu, tt}},
                           {{px, py + h, 0}, n, {u, tt}}});
        x += g.advance * scale;
    }
    uiMesh.set(v);
    glUniform4fv(uiColor, 1, glm::value_ptr(color));
    glBindTexture(GL_TEXTURE_2D, font.atlas);
    uiMesh.draw();
}
void Graphics::screenshot(const std::string &path) {
    std::vector<unsigned char> pixels(pixelW * pixelH * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, pixelW, pixelH, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::ofstream f(path, std::ios::binary);
    f << "P6\n" << pixelW << " " << pixelH << "\n255\n";
    for (int y = pixelH - 1; y >= 0; y--)
        f.write(reinterpret_cast<char *>(pixels.data() + y * pixelW * 3), pixelW * 3);
}
void Graphics::release() {
    cube.release();
    plane.release();
    curl.release();
    uiMesh.release();
    GLuint textures[] = {white, wood, leather, shadowTex};
    glDeleteTextures(4, textures);
    glDeleteFramebuffers(1, &shadowFbo);
    glDeleteProgram(sceneShader);
    glDeleteProgram(depthShader);
    glDeleteProgram(uiShader);
}
} // namespace bn
