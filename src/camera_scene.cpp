// Camera, scene composition, particles, movable light, animation pause,
// mouse picking on the page and text selection.
// Module owner: Alikhan Faizrakhman.
#include "app.hpp"

namespace bn {
glm::vec3 App::lightPos() const {
    // Light orbits the book: radius 5, height 7; lightAngle is its azimuth.
    return {std::cos(lightAngle) * 5.f, std::sin(lightAngle) * 5.f, 7.f};
}
double App::animNow() const { return (paused ? pausedAt : glfwGetTime()) - animOffset; }
void App::updateCamera() {
    // Orbit camera around the book; projection follows the framebuffer aspect ratio.
    float aspect = float(g.pixelW) / g.pixelH;
    g.eye = {std::sin(yaw) * distance, std::sin(pitch) * distance,
             std::cos(yaw) * std::cos(pitch) * distance};
    g.vp = glm::perspective(glm::radians(43.f), aspect, .1f, 40.f) *
           glm::lookAt(g.eye, glm::vec3(0), glm::vec3(0, 1, 0));
    g.light = lightPos();
}
std::vector<Draw> App::scene(double time) {
    // Scene composition: desk, desk props, the book, then particles.
    std::vector<Draw> d;
    auto box = [&](glm::vec3 pos, glm::vec3 scale, glm::vec3 color, GLuint tex = 0, bool casts = true) {
        d.push_back({&g.cube, transform(pos, scale), tex, color, 0, 0, casts});
    };
    box({0, 0, -.43f}, {18, 12, .28f}, {.82f, .86f, .78f}, g.wood, false);
    addDeskProps(d);
    addBook(d, time);
    if (particlesOn)
        for (int i = 0; i < 25; i++) {
            float a = float(time) * .15f + i * 2.399f, r = 3.2f + .6f * std::sin(i * 1.7f);
            glm::vec3 p{std::cos(a) * r, std::sin(a) * 2.6f,
                        .8f + .45f * std::sin(float(time) * .7f + i)};
            d.push_back({&g.cube,
                         transform(p, {.025f, .025f, .025f}),
                         g.white,
                         {.88f, .65f, .25f},
                         0,
                         .6f,
                         false});
        }
    return d;
}
void App::drawLightControls() {
// Light direction: arrows rotate it; the compass shows where it shines from
// (top view: the small rectangle is the book).
label("Light direction", 1225, 608, 12, Muted);
button("<", 1221, 626, 44, 44, [&] { rotateLight(.26f); });
}
void App::togglePause() {
    double now = glfwGetTime();
    if (paused)
        animOffset += now - pausedAt;
    else
        pausedAt = now;
    paused = !paused;
    textureDirty = true;
    tell(paused ? "Animation paused (F7 to resume)." : "Animation resumed.");
}
void App::rotateLight(float delta) {
    constexpr float TwoPi = 6.2831853f;
    lightAngle = std::fmod(lightAngle + delta + TwoPi, TwoPi);
}
std::pair<std::size_t, std::size_t> App::selection() const {
    auto n = doc.pages[active].text.size();
    auto a = std::min(anchor, n), b = std::min(cursor, n);
    return {std::min(a, b), std::max(a, b)};
}
void App::penFromCursor() {
    const auto &page = doc.pages[active];
    if (cursor > 0 && cursor <= page.text.size())
        pen = page.styleAt(cursor - 1);
    else if (!page.text.empty())
        pen = page.styleAt(0);
}
CharStyle App::shownStyle() const {
    return hasSelection() ? doc.pages[active].styleAt(selection().first) : pen;
}
std::u32string App::selectedText() const {
    auto [a, b] = selection();
    return doc.pages[active].text.substr(a, b - a);
}
void App::applyStyle(int ink, int fontIndex) {
    if (!canEdit())
        return;
    if (ink >= 0)
        pen.ink = std::uint8_t(ink);
    if (fontIndex >= 0)
        pen.font = std::uint8_t(fontIndex);
    if (hasSelection()) {
        auto [a, b] = selection();
        bool pushed = checkpoint();
        if (doc.restyle(active, a, b, ink, fontIndex))
            changed();
        else if (pushed)
            undo.pop_back();
    } else if (fontIndex >= 0)
        tell(std::string("New text: ") + pageFontName(fontIndex) + ".");
}
void App::moveCursor(int key, bool ctrl, bool shift) {
    auto &text = doc.pages[active].text;
    if (!shift && hasSelection() && (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT)) {
        // Left/Right without Shift jump to the edge of the selection.
        auto [a, b] = selection();
        cursor = anchor = key == GLFW_KEY_LEFT ? a : b;
        penFromCursor();
        textureDirty = true;
        return;
    }
    auto cells = layout(text, gridFor(doc.pages[active].size).cols);
    auto at = cells[cursor];
    if (key == GLFW_KEY_LEFT && cursor > 0)
        cursor--;
    if (key == GLFW_KEY_RIGHT && cursor < text.size())
        cursor++;
    if (key == GLFW_KEY_HOME) {
        if (ctrl)
            cursor = 0;
        else
            while (cursor > 0 && cells[cursor - 1].row == at.row)
                cursor--;
    }
    if (key == GLFW_KEY_END) {
        if (ctrl)
            cursor = text.size();
        else
            while (cursor < text.size() && cells[cursor + 1].row == at.row)
                cursor++;
    }
    if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN) {
        int row = at.row + (key == GLFW_KEY_UP ? -1 : 1), best = 99999;
        std::size_t pick = cursor;
        for (std::size_t i = 0; i < cells.size(); i++)
            if (cells[i].row == row) {
                int delta = std::abs(cells[i].col - at.col);
                if (delta < best) {
                    best = delta;
                    pick = i;
                }
            }
        cursor = pick;
    }
    if (!shift)
        anchor = cursor;
    penFromCursor();
    textureDirty = true;
}
void App::pointer(double x, double y) {
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    mx = x * 1440 / std::max(w, 1);
    my = y * 900 / std::max(h, 1);
    if (dragging) {
        yaw = std::clamp(yaw + float(x - lastX) * .004f, -.45f, .45f);
        pitch = std::clamp(pitch - float(y - lastY) * .004f, -.55f, .25f);
    }
    lastX = x;
    lastY = y;
    if (selecting) {
        // Dragging with the left button extends the selection on the same page.
        int index;
        std::size_t pos;
        if (hitPaper(index, pos) && index == active && pos != cursor) {
            cursor = pos;
            textureDirty = true;
        }
    }
}
bool App::hitPaper(int &index, std::size_t &pos) {
    auto inv = glm::inverse(g.vp);
    glm::vec4 a = inv * glm::vec4(float(mx / 1440 * 2 - 1), float(1 - my / 900 * 2), -1, 1),
              b = inv * glm::vec4(float(mx / 1440 * 2 - 1), float(1 - my / 900 * 2), 1, 1);
    a /= a.w;
    b /= b.w;
    glm::vec3 dir = glm::vec3(b - a);
    if (std::abs(dir.z) < .0001f)
        return false;
    glm::vec3 p = glm::vec3(a) + dir * ((.10f - a.z) / dir.z);
    if (std::abs(p.y) > 1.825f || std::abs(p.x) > 2.55f)
        return false;
    int side = p.x < 0 ? 0 : 1;
    index = spread + side;
    pos = 0;
    if (index >= int(doc.pages.size()))
        return true;
    float u = (p.x + (side == 0 ? 2.55f : 0)) / 2.55f, v = (1.825f - p.y) / 3.65f;
    auto grid = gridFor(doc.pages[index].size);
    int col = int(std::round((u * 640 - 72) / grid.cellW)), row = rowAt(grid, v * 900);
    col = std::clamp(col, 0, grid.cols);
    row = std::clamp(row, 0, grid.rows - 1);
    auto cells = layout(doc.pages[index].text, grid.cols);
    int best = 99999;
    for (std::size_t i = 0; i < cells.size(); i++) {
        int d = std::abs(cells[i].row - row) * 100 + std::abs(cells[i].col - col);
        if (d < best) {
            best = d;
            pos = i;
        }
    }
    return true;
}
void App::click(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        dragging = action == GLFW_PRESS;
        return;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        selecting = false;
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
        return;
    for (auto it = buttons.rbegin(); it != buttons.rend(); ++it)
        if (mx >= it->x && mx < it->x + it->w && my >= it->y && my < it->y + it->h) {
            if (it->enabled) {
                auto fn = it->action;
                fn();
            }
            return;
        }
    if (modal || help || !opened || flipping)
        return;
    int index;
    std::size_t pos;
    if (!hitPaper(index, pos))
        return;
    if (index >= int(doc.pages.size())) {
        addPage();
        return;
    }
    bool extend = (mods & GLFW_MOD_SHIFT) && index == active;
    active = index;
    cursor = pos;
    if (!extend)
        anchor = cursor;
    selecting = true;
    penFromCursor();
    textureDirty = true;
}
} // namespace bn
