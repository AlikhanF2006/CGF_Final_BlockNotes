// Right "Writing desk" panel: ink colour, font and size of the text, scene toggles,
// pause, light direction and camera reset.
// Module owner: Alikhan Faizrakhman.
#include "app.hpp"

namespace bn {
void App::drawWritingDesk() {
    g.rect(1206, 103, 210, 641, Dark);
    label("WRITING DESK", 1225, 121, 14, Muted);
    label("Ink colour", 1225, 157, 18);
    for (int i = 0; i < 6; i++) {
        int x = 1225 + (i % 3) * 56, y = 193 + (i / 3) * 48;
        auto c = inkColor(i);
        g.rect(float(x - 2), float(y - 2), 40, 36,
               shownStyle().ink == i ? Gold : glm::vec4(.3f, .35f, .29f, 1));
        g.rect(float(x), float(y), 36, 32, glm::vec4(c, 1));
        buttons.push_back({float(x), float(y), 36, 32,
                           [&, i] { applyStyle(i, -1); }, canEdit()});
    }
    {
        const auto &page = doc.pages[active];
        bool editable = canEdit();
        button(std::string("Font: ") + pageFontName(shownStyle().font), 1221, 287, 180, 36,
               [&] { applyStyle(-1, (shownStyle().font + 1) % PageFonts); }, editable);
        const char *sizes[] = {"S", "M", "L"};
        for (int i = 0; i < PageSizes; i++)
            button(sizes[i], 1221 + i * 62, 331, 56, 36, [&, i] { setPageSize(i); }, editable,
                   page.size == i);
    }
    label(hasSelection() ? "Ink, font: selected text" : "Ink, font: new text", 1225, 376, 12,
          hasSelection() ? Gold : Muted);
    label("S / M / L: whole page", 1225, 393, 12, Muted);
    g.rect(1225, 414, 170, 1, {.33f, .40f, .32f, .65f});
    label("SCENE", 1225, 424, 14, Muted);
    button(lightOn ? "Light: ON" : "Light: OFF", 1221, 446, 180, 34, [&] { lightOn = !lightOn; });
    button(shadowsOn ? "Shadows: ON" : "Shadows: OFF", 1221, 486, 180, 34,
           [&] { shadowsOn = !shadowsOn; });
    button(particlesOn ? "Particles: ON" : "Particles: OFF", 1221, 526, 180, 34,
           [&] { particlesOn = !particlesOn; });
    button(paused ? "Pause: ON" : "Pause: OFF", 1221, 566, 180, 34, [&] { togglePause(); }, true, paused);
    {
        drawLightControls();
        button(">", 1357, 626, 44, 44, [&] { rotateLight(-.26f); });
        const float cx = 1311, cy = 648, r = 20;
        for (int k = 0; k < 24; k++) {
            float a = k * 6.2831853f / 24;
            g.rect(cx + std::cos(a) * r - 1, cy - std::sin(a) * r - 1, 2, 2, {.45f, .52f, .42f, 1});
        }
        g.rect(cx - 5, cy - 7, 10, 14, {.83f, .79f, .64f, 1});
        float lx = cx + std::cos(lightAngle) * r, ly = cy - std::sin(lightAngle) * r;
        g.rect(lx - 5, ly - 5, 10, 10, Gold);
    }
    button("Reset view", 1221, 680, 180, 32, [&] {
        yaw = 0;
        pitch = -.19f;
        distance = 8.7f;
        lightAngle = 2.2143f;
    });
    if (fps > 0) // not shown until the first one-second measurement
        label(std::to_string(fps) + " FPS", 1225, 722, 13, Gold);
}
} // namespace bn
