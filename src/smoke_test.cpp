// Automated smoke test (--smoke DIR): drives the app step by step, takes screenshots of
// every feature and checks selection, styling and undo. Used to verify builds without a user.
// Module owner: Alikhan Kenzhebek.
#include "app.hpp"

namespace bn {
bool App::smokeStep(int step, const std::string &captures) {
    if (step == 5)
        g.screenshot(captures + "/01-open.ppm");
    if (step == 8) {
        choose(1);
        type(U"\nTest 123");
        save();
        flip(1);
    }
    if (step == 9) {
        flipStart = animNow() - .42;
        frame();
        g.screenshot(captures + "/02-turn.ppm");
    }
    if (step == 12) {
        flipping = false;
        choose(2);
        frame();
        g.screenshot(captures + "/03-next.ppm");
    }
    if (step == 15) {
        opened = false;
        frame();
        g.screenshot(captures + "/04-cover.ppm");
    }
    if (step == 18) {
        opened = true;
        modal = 1;
        signTitle = U"My notebook";
        signAuthor = U"Student";
        frame();
        g.screenshot(captures + "/05-sign.ppm");
    }
    if (step == 20) {
        modal = 0;
        help = true;
        frame();
        g.screenshot(captures + "/06-help.ppm");
    }
    if (step == 22) {
        help = false;
        openCover();
        signTitle = U"Field notes from the northern hills";
        coverDraft = {2, 1, 3, 1, true};
        previewDirty = true;
        frame();
        g.screenshot(captures + "/07-cover-dialog.ppm");
    }
    if (step == 24) {
        doc.rename(signTitle);
        doc.setCover(coverDraft);
        doc.sign(doc.title, U"Daulet");
        doc.setReadOnly(true);
        modal = 0;
        opened = false;
        textureDirty = true;
        frame();
        g.screenshot(captures + "/08-custom-cover.ppm");
    }
    if (step == 26) {
        save();
        listBooks();
        frame();
        g.screenshot(captures + "/09-load-delete.ppm");
    }
    if (step == 28) {
        opened = true;
        openEditSigned();
        frame();
        g.screenshot(captures + "/10-edit-signed.ppm");
    }
    if (step == 30) {
        // Timing: repaint after one typed character (what used to lag).
        modal = 0;
        doc.setReadOnly(false);
        choose(0);
        frame();
        auto t0 = std::chrono::steady_clock::now();
        for (int k = 0; k < 50; k++) {
            type(U"a");
            refreshTextures(true);
        }
        auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0)
                      .count() / 50;
        std::cout << "Keystroke repaint: " << ms << " ms\n";
        t0 = std::chrono::steady_clock::now();
        for (int k = 0; k < 20; k++) {
            textureDirty = true;
            slotKeys[0] = slotKeys[1] = {};
            coverKey = {};
            refreshTextures(true);
        }
        ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0)
                 .count() / 20;
        std::cout << "Full repaint (2 pages + cover): " << ms << " ms\n";
    }
    if (step == 31) {
        // Mixed styles on one page plus a visible selection.
        doc.restyle(0, 0, 31, 1, 2);
        doc.restyle(0, 40, 70, 2, 1);
        doc.restyle(1, 0, 17, 4, 1);
        choose(1);
        anchor = 20;
        cursor = 60;
        textureDirty = true;
        frame();
        g.screenshot(captures + "/11-signed-edit-mode.ppm");
    }
    if (step == 32) {
        doc.setSize(2, 2);
        doc.restyle(2, 0, doc.pages[2].text.size(), -1, 1);
        doc.setSize(3, 0);
        choose(2);
        frame();
        g.screenshot(captures + "/12-pixel-large-small.ppm");
    }
    if (step == 33) {
        // Simulated mouse drag on the left page of the current spread, then style + cut.
        choose(2);
        frame();
        pointer(470, 330);
        click(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
        auto start = cursor;
        pointer(640, 380);
        click(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE);
        auto [from, to] = selection();
        std::cout << "Drag select: page " << active + 1 << " start " << start << " -> "
                  << from << ".." << to << "\n";
        auto before = doc.pages[active].text.size();
        applyStyle(1, 2);
        bool styled = to > from && doc.pages[active].styleAt(from) == CharStyle{1, 2};
        deleteText(false);
        std::cout << "Restyle selection: " << (styled ? "ok" : "FAIL") << ", delete removed "
                  << before - doc.pages[active].text.size() << " chars\n";
        history(false);
        std::cout << "Undo restored: "
                  << (doc.pages[active].text.size() == before ? "ok" : "FAIL") << "\n";
    }
    if (step == 34) {
        // Light rotated to the right-front, animation paused.
        lightAngle = 5.6f;
        togglePause();
        frame();
        g.screenshot(captures + "/13-light-paused.ppm");
        togglePause();
    }
    return step + 1 >= 35;

}
} // namespace bn
