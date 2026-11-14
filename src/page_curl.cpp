// Page turn: curved page mesh, smoothstep animation curve, page-turn state machine,
// hierarchical book model and desk props.
// Module owner: Alikhan Kenzhebek.
#include "app.hpp"

namespace bn {
void App::updateFlip(double anim) {
    // The page-turn animation lasts 0.85 s of animation time, then the new spread is shown.
    if (flipping && anim - flipStart >= .85) {
        flipping = false;
        choose(targetSpread);
    }
}
void App::flip(int dir) {
    if (flipping || !opened || modal)
        return;
    int next = spread + dir * 2;
    if (next < 0 || next >= int(doc.pages.size()))
        return;
    if (paused) {
        // No animation while paused: turn the spread immediately.
        choose(next);
        return;
    }
    flipping = true;
    backward = dir < 0;
    targetSpread = next;
    flipStart = animNow();
    anchor = cursor;
    textureDirty = true;
}
void App::addDeskProps(std::vector<Draw> &d) {
    auto box = [&](glm::vec3 pos, glm::vec3 scale, glm::vec3 color, GLuint tex = 0, bool casts = true) {
        d.push_back({&g.cube, transform(pos, scale), tex, color, 0, 0, casts});
    };
    // Block-built desk objects use a shared unit cube and hierarchical transforms.
    for (int side : {-1, 1}) {
        float x = side * 3.8f;
        box({x, 1.6f, -.02f}, {.65f, .65f, .65f}, {.35f, .23f, .15f});
        box({x, 1.6f, .40f}, {.16f, .16f, .55f}, {.28f, .35f, .13f});
        box({x, 1.6f, .83f}, {.85f, .7f, .38f}, {.28f, .43f, .21f});
        box({x + .16f, 1.6f, 1.09f}, {.56f, .52f, .25f}, {.38f, .52f, .25f});
    }
    box({3.45f, -1.25f, -.05f}, {.65f, .75f, .35f}, {.12f, .20f, .20f});
    box({3.45f, -1.25f, .18f}, {.38f, .43f, .17f}, {.12f, .13f, .10f});
    auto pen = transform({-3.35f, -1.2f, -.12f}, {1, 1, 1});
    pen = glm::rotate(pen, -.3f, glm::vec3(0, 0, 1));
    d.push_back({&g.cube, glm::scale(pen, {.10f, 1.55f, .10f}), g.white, {.80f, .65f, .32f}});
}
void App::addBook(std::vector<Draw> &d, double time) {
    auto box = [&](glm::vec3 pos, glm::vec3 scale, glm::vec3 color, GLuint tex = 0, bool casts = true) {
        d.push_back({&g.cube, transform(pos, scale), tex, color, 0, 0, casts});
    };
    const glm::vec3 leather = coverColor(doc.cover.color) * 2.f;
    if (opened) {
        box({-1.34f, 0, -.14f}, {2.78f, 3.95f, .17f}, leather, g.leather);
        box({1.34f, 0, -.14f}, {2.78f, 3.95f, .17f}, leather, g.leather);
        box({0, 0, -.13f}, {.16f, 3.95f, .20f}, {.23f, .12f, .06f});
        for (int side : {-1, 1})
            for (int k = 0; k < 5; k++)
                box({side * 1.28f, 0, -.045f + k * .025f}, {2.55f, 3.65f, .018f},
                    {.83f + k * .021f, .78f + k * .019f, .65f + k * .024f});
        GLuint l = pageTex[0], r = pageTex[1];
        if (flipping) {
            if (backward)
                l = pageTex[2];
            else
                r = pageTex[3];
        }
        d.push_back({&g.plane, transform({-1.275f, 0, .10f}, {2.55f, 3.65f, 1}), l, {1, 1, 1}});
        d.push_back({&g.plane, transform({1.275f, 0, .10f}, {2.55f, 3.65f, 1}), r, {1, 1, 1}});
        if (flipping) {
            float t = std::clamp(float((time - flipStart) / .85), 0.f, 1.f);
            t = t * t * (3 - 2 * t);
            g.updateCurl(t, backward);
            d.push_back({&g.curl,
                         glm::mat4(1),
                         backward ? pageTex[0] : pageTex[1],
                         {1, 1, 1},
                         backward ? pageTex[3] : pageTex[2]});
        }
        for (float y = -1.4f; y < 1.5f; y += .7f)
            box({0, y, .115f}, {.07f, .12f, .015f}, {.66f, .47f, .25f});
    } else {
        box({0, 0, -.1f}, {2.8f, 3.95f, .4f}, leather, g.leather);
        box({.03f, 0, .12f}, {2.58f, 3.7f, .13f}, {.83f, .79f, .64f});
        box({0, 0, .23f}, {2.82f, 3.96f, .12f}, leather, g.leather);
        d.push_back({&g.plane, transform({0, 0, .295f}, {2.78f, 3.91f, 1}), coverTex, {1, 1, 1}});
    }
}
void Graphics::updateCurl(float t, bool backward) {
    constexpr float W = 2.55f, H = 3.65f;
    const float pi = 3.14159265f;
    auto pos = [&](float u, float v) {
        float theta = pi * t + std::sin(pi * t) * std::sin(u * pi * .7f) * .48f;
        return glm::vec3(std::cos(theta) * u * W, (v - .5f) * H, .10f + std::sin(theta) * u * W);
    };
    auto vertex = [&](float u, float v) {
        auto p = pos(u, v);
        p.z += .003f;
        auto tangent = pos(u + .0001f, v) - pos(u - .0001f, v);
        glm::vec3 n = glm::normalize(glm::cross(tangent, glm::vec3(0, 1, 0)));
        if (backward) {
            p.x = -p.x;
            n.x = -n.x;
        }
        return Vertex{p, n, {backward ? 1 - u : u, 1 - v}};
    };
    std::vector<Vertex> vertices;
    for (int i = 0; i < 64; i++)
        for (int j = 0; j < 12; j++) {
            float u = i / 64.f, v = j / 12.f;
            auto a = vertex(u, v), b = vertex(u + 1 / 64.f, v), c = vertex(u + 1 / 64.f, v + 1 / 12.f),
                 d = vertex(u, v + 1 / 12.f);
            if (backward)
                vertices.insert(vertices.end(), {a, c, b, a, d, c});
            else
                vertices.insert(vertices.end(), {a, b, c, a, c, d});
        }
    curl.set(vertices);
}
} // namespace bn
