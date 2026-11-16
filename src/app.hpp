#pragma once
// BlockNotes application state and main interface.
// Module owners: window, input and interface - Farkhad Imanbayev.
// Functions declared here but defined elsewhere are listed at the top of struct App.
#include <glad/gl.h>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include "document.hpp"
#include "graphics.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace bn {
namespace fs = std::filesystem;
const glm::vec4 Cream{.91f, .88f, .77f, 1}, Muted{.57f, .65f, .57f, 1}, Dark{.065f, .095f, .078f, .97f},
    Gold{.83f, .66f, .34f, 1};
struct Button {
    float x, y, w, h;
    std::function<void()> action;
    bool enabled;
};
struct Snapshot {
    Document doc;
    int active;
    std::size_t cursor;
};
// What a page texture currently shows; a slot is only repainted when this changes.
struct SlotKey {
    int page = -2;
    bool exists = false;
    std::u32string text;
    int size = 1;
    std::vector<CharStyle> styles;
    bool selected = false, caret = false;
    std::size_t cursor = 0, selFrom = 0, selTo = 0;
    bool operator==(const SlotKey &o) const {
        return page == o.page && exists == o.exists && size == o.size && selected == o.selected &&
               caret == o.caret && cursor == o.cursor && selFrom == o.selFrom && selTo == o.selTo &&
               text == o.text && styles == o.styles;
    }
};
struct CoverKey {
    std::u32string title, author;
    bool signedBook = false, valid = false;
    CoverStyle style;
    bool operator==(const CoverKey &o) const {
        return valid == o.valid && signedBook == o.signedBook && style == o.style && title == o.title &&
               author == o.author;
    }
};
struct App {
    // --- module functions defined in other files ---
    // camera_scene.cpp (Alikhan Faizrakhman)
    void updateCamera();
    void drawLightControls();
    // writing_desk.cpp (Alikhan Faizrakhman): right panel - ink, font, size, scene, light
    void drawWritingDesk();
    // smoke_test.cpp (Alikhan Kenzhebek): automated screenshot / interaction test
    bool smokeStep(int step, const std::string &captures);
    // page_curl.cpp (Alikhan Kenzhebek)
    void updateFlip(double anim);
    void addDeskProps(std::vector<Draw> &d);
    void addBook(std::vector<Draw> &d, double time);
    // dialogs.cpp (Alexander Tarulin)
    void drawDialogs();
    GLFWwindow *window = nullptr;
    Graphics g;
    Font font;                  // UI + Classic page font (DejaVu Sans Mono)
    Font pixelFont, handFont;   // page fonts: Press Start 2P, Victor Mono Italic
    Font &pageFont(int i) { return i == 1 ? pixelFont : i == 2 ? handFont : font; }
    Document doc;
    fs::path savePath;
    int active = 0, spread = 0;
    std::size_t cursor = 0;
    std::deque<Snapshot> undo, redo;
    std::vector<Button> buttons;
    std::vector<BookInfo> books;
    fs::path deleteTarget;
    std::u32string deleteTitle;
    bool deleteIsCurrent = false;
    int bookListOffset = 0;
    GLuint pageTex[4]{}, coverTex = 0, previewTex = 0;
    SlotKey slotKeys[4];
    CoverKey coverKey;
    CoverStyle coverDraft;
    bool previewDirty = true;
    double typingSince = -10, typingLast = -10;
    int typingPage = -1;
    // Selection is [min(anchor, cursor), max(anchor, cursor)) on the active page.
    std::size_t anchor = 0;
    CharStyle pen;          // colour/font for newly typed text
    bool selecting = false; // left mouse held on the paper
    bool textureDirty = true, lightOn = true, shadowsOn = true, particlesOn = true,
         opened = true, help = false, dragging = false, loadBlocked = false;
    bool lastCaret = false;
    // 1 sign, 2 delete page, 3 new notebook, 4 load list, 5 delete book, 6 edit signed book, 7 cover
    int modal = 0, field = 0;
    std::u32string signTitle, signAuthor;
    std::string status = "Click a page and start writing.";
    double statusUntil = 0, lastEdit = 0;
    bool flipping = false, backward = false;
    int targetSpread = 0;
    double flipStart = 0;
    float yaw = 0, pitch = -.19f, distance = 8.7f;
    // Movable light: azimuth around the book (radius 5, height 7). Default = original light.
    float lightAngle = 2.2143f;
    glm::vec3 lightPos() const;
    // Animation clock: particles, page turn and caret use it; pausing stops only this clock.
    bool paused = false;
    double pausedAt = 0, animOffset = 0;
    double animNow() const;
    void togglePause();
    void rotateLight(float delta);
    double lastX = 0, lastY = 0, mx = 0, my = 0;
    int fps = 0, frames = 0;
    double fpsStart = 0;
    bool smoke = false;
    int smokeFrame = 0;
    std::string captureDir;

    void tell(const std::string &s) {
        status = s;
        statusUntil = glfwGetTime() + 5;
    }
    void loadBook(const fs::path &path);
    void scanBooks();
    void listBooks();
    bool samePath(const fs::path &a, const fs::path &b);
    void removeBook(const fs::path &path);
    bool save() {
        std::string error;
        if (loadBlocked) {
            tell("Load failed: use New book or recover the .bak file first.");
            return false;
        }
        if (doc.save(savePath, error)) {
            tell("Saved to " + savePath.filename().string());
            return true;
        }
        tell("SAVE FAILED: " + error);
        return false;
    }
    // Returns true if a new undo step was pushed. Fast typing on one page is grouped into one
    // step (until a 1 s pause or 3 s of typing) so the whole book is not copied on every key.
    bool checkpoint(bool typing = false) {
        double now = glfwGetTime();
        if (typing && typingPage == active && !undo.empty() && now - typingLast < 1.0 &&
            now - typingSince < 3.0) {
            typingLast = now;
            redo.clear();
            return false;
        }
        undo.push_back({doc, active, cursor});
        if (undo.size() > 100)
            undo.pop_front();
        redo.clear();
        typingPage = typing ? active : -1;
        typingSince = typingLast = typing ? now : -10;
        return true;
    }
    std::u32string * activeField();
    void openCover();
    void openEditSigned();
    void changed() {
        textureDirty = true;
        lastEdit = glfwGetTime();
    }
    bool hasSelection() const { return anchor != cursor; }
    std::pair<std::size_t, std::size_t> selection() const;
    // The pen takes the style of the character before the cursor, like a word processor.
    void penFromCursor();
    CharStyle shownStyle() const;
    void choose(int p) {
        active = std::clamp(p, 0, int(doc.pages.size()) - 1);
        spread = (active / 2) * 2;
        cursor = doc.pages[active].text.size();
        anchor = cursor;
        penFromCursor();
        textureDirty = true;
    }
    void history(bool forward) {
        if (doc.readOnly || flipping)
            return;
        auto &source = forward ? redo : undo;
        auto &dest = forward ? undo : redo;
        if (source.empty())
            return;
        typingPage = -1;
        dest.push_back({doc, active, cursor});
        auto s = source.back();
        source.pop_back();
        bool mode = doc.readOnly; // undo never flips Read/Edit mode
        doc = s.doc;
        doc.readOnly = mode;
        active = s.active;
        cursor = s.cursor;
        anchor = cursor;
        spread = (active / 2) * 2;
        doc.dirty = true;
        changed();
    }
    void toggleMode() {
        if (flipping || loadBlocked)
            return;
        doc.setReadOnly(!doc.readOnly);
        anchor = cursor;
        textureDirty = true;
        lastEdit = glfwGetTime();
        tell(doc.readOnly ? "Read mode: pages are locked." : "Edit mode: click a page and write.");
    }
    void type(const std::u32string &text) {
        if (auto *s = activeField()) {
            for (auto c : text)
                if (c >= 32 && c != 127 && s->size() < 40)
                    *s += c;
            previewDirty = true;
            return;
        }
        if (modal || help || !opened || doc.readOnly || flipping || loadBlocked)
            return;
        // Typing over a selection replaces it (as its own undo step).
        bool replacing = hasSelection();
        bool pushed = checkpoint(!replacing);
        Page before = doc.pages[active];
        auto beforeCursor = cursor;
        if (replacing) {
            auto [a, b] = selection();
            doc.eraseRange(active, a, b);
            cursor = a;
        }
        if (doc.insert(active, cursor, text, pen)) {
            anchor = cursor;
            changed();
        } else {
            doc.pages[active] = before;
            cursor = beforeCursor;
            if (pushed)
                undo.pop_back();
            tell("Page full. Add a new page to continue.");
        }
    }
    void deleteText(bool forward) {
        if (auto *s = activeField()) {
            if (!s->empty())
                s->pop_back();
            previewDirty = true;
            return;
        }
        if (modal || help || !opened || doc.readOnly || flipping || loadBlocked)
            return;
        if (hasSelection()) {
            auto [a, b] = selection();
            checkpoint();
            if (doc.eraseRange(active, a, b)) {
                cursor = anchor = a;
                changed();
            }
            return;
        }
        bool pushed = checkpoint(true);
        if (doc.erase(active, cursor, forward)) {
            anchor = cursor;
            changed();
        } else if (pushed)
            undo.pop_back();
    }
    std::u32string selectedText() const;
    void addPage() {
        if (flipping || doc.readOnly || loadBlocked)
            return;
        checkpoint();
        if (doc.add(active)) {
            choose(active + 1);
            changed();
            tell("New page added.");
        } else {
            undo.pop_back();
            tell("Maximum 200 pages.");
        }
    }
    void flip(int dir);
    bool canEdit() const { return !doc.readOnly && !flipping && !loadBlocked; }
    // Colour/font go to the selected text, or to the text typed next when nothing is selected.
    void applyStyle(int ink, int fontIndex);
    void setPageSize(int size) {
        if (!canEdit() || size == doc.pages[active].size)
            return;
        bool pushed = checkpoint();
        if (doc.setSize(active, size)) {
            changed();
            tell(std::string("Page ") + std::to_string(active + 1) + ": size " + "SML"[size] + ".");
        } else {
            if (pushed)
                undo.pop_back();
            tell("The text does not fit at this size. Use a smaller size or move text to a new page.");
        }
    }
    void moveCursor(int key, bool ctrl, bool shift = false);
    void key(int key, int action, int mods) {
        if (action != GLFW_PRESS && action != GLFW_REPEAT)
            return;
        bool ctrl = (mods & GLFW_MOD_CONTROL) || (mods & GLFW_MOD_SUPER);
        bool shift = mods & GLFW_MOD_SHIFT;
        if (key == GLFW_KEY_ESCAPE) {
            if (modal)
                modal = 0;
            else if (help)
                help = false;
            else {
                if (doc.dirty && !save())
                    return;
                opened = !opened;
            }
            return;
        }
        if (key == GLFW_KEY_F1) {
            help = !help;
            return;
        }
        if (help)
            return;
        if (modal) {
            if (activeField()) {
                if (key == GLFW_KEY_TAB && (modal == 1 || modal == 6))
                    field = 1 - field;
                if (key == GLFW_KEY_BACKSPACE)
                    deleteText(false);
                if (ctrl && key == GLFW_KEY_V) {
                    const char *s = glfwGetClipboardString(window);
                    if (s)
                        try {
                            type(decode(s));
                        } catch (...) {
                        }
                }
            }
            return;
        }
        if (ctrl) {
            if (key == GLFW_KEY_S)
                save();
            if (key == GLFW_KEY_Z)
                history(false);
            if (key == GLFW_KEY_Y)
                history(true);
            if (key == GLFW_KEY_N)
                addPage();
            if (key == GLFW_KEY_A && opened) {
                anchor = 0;
                cursor = doc.pages[active].text.size();
                textureDirty = true;
            }
            if ((key == GLFW_KEY_C || key == GLFW_KEY_X) && hasSelection()) {
                glfwSetClipboardString(window, encode(selectedText()).c_str());
                if (key == GLFW_KEY_X)
                    deleteText(false);
            }
            if (key == GLFW_KEY_V) {
                const char *s = glfwGetClipboardString(window);
                if (s)
                    try {
                        type(decode(s));
                    } catch (...) {
                        tell("Clipboard text is not valid UTF-8.");
                    }
            }
            if (key == GLFW_KEY_HOME || key == GLFW_KEY_END)
                moveCursor(key, true, shift);
            return;
        }
        if (key == GLFW_KEY_F2)
            lightOn = !lightOn;
        if (key == GLFW_KEY_F3)
            shadowsOn = !shadowsOn;
        if (key == GLFW_KEY_F4)
            particlesOn = !particlesOn;
        if (key == GLFW_KEY_F6)
            toggleMode();
        if (key == GLFW_KEY_F7)
            togglePause();
        if ((mods & GLFW_MOD_ALT) && (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT)) {
            rotateLight(key == GLFW_KEY_LEFT ? .12f : -.12f);
            return;
        }
        if (key == GLFW_KEY_F5) {
            yaw = 0;
            pitch = -.19f;
            distance = 8.7f;
        }
        if (key == GLFW_KEY_PAGE_DOWN)
            flip(1);
        if (key == GLFW_KEY_PAGE_UP)
            flip(-1);
        if (!opened || flipping)
            return;
        if (key == GLFW_KEY_BACKSPACE)
            deleteText(false);
        if (key == GLFW_KEY_DELETE)
            deleteText(true);
        if (key == GLFW_KEY_ENTER)
            type(U"\n");
        if (key == GLFW_KEY_TAB)
            type(U"    ");
        if (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT || key == GLFW_KEY_UP ||
            key == GLFW_KEY_DOWN || key == GLFW_KEY_HOME || key == GLFW_KEY_END)
            moveCursor(key, false, shift);
    }
    void pointer(double x, double y);
    // Ray from the mouse to the open book: which page and which character position.
    bool hitPaper(int &index, std::size_t &pos);
    void click(int button, int action, int mods = 0);
    void refreshTextures(bool caret) {
        int ids[] = {spread, spread + 1, flipping ? targetSpread : spread,
                     flipping ? targetSpread + 1 : spread + 1};
        // Slots 2 and 3 are only shown while a page is turning.
        int slots = flipping ? 4 : 2;
        for (int i = 0; i < slots; i++) {
            int n = ids[i];
            SlotKey key;
            key.page = n;
            key.exists = n < int(doc.pages.size());
            if (key.exists) {
                key.text = doc.pages[n].text;
                key.styles = doc.pages[n].styles;
                key.size = doc.pages[n].size;
            }
            key.selected = n == active;
            if (key.selected) {
                key.caret = caret && !doc.readOnly && !flipping;
                key.cursor = cursor;
                std::tie(key.selFrom, key.selTo) = selection();
            }
            if (pageTex[i] && key == slotKeys[i])
                continue;
            Font *fonts[PageFonts] = {&font, &pixelFont, &handFont};
            auto c = paper(font, fonts, key.exists ? &doc.pages[n] : nullptr, n + 1, key.selected, key.cursor,
                           key.caret, key.selFrom, key.selTo);
            pageTex[i] = texture(c, pageTex[i]);
            slotKeys[i] = std::move(key);
        }
        CoverKey ck{doc.title, doc.author, doc.signedBook, true, doc.cover};
        if (!(ck == coverKey)) {
            coverTex = texture(coverArt(font, doc.title, doc.author, doc.signedBook, doc.cover), coverTex);
            coverKey = std::move(ck);
        }
        textureDirty = false;
    }
    std::vector<Draw> scene(double time);
    void label(const std::string &text, float x, float y, float size = 19, glm::vec4 color = Cream) {
        g.text(font, text, x, y, size, color);
    }
    void label(const std::u32string &text, float x, float y, float size = 19, glm::vec4 color = Cream) {
        g.text(font, text, x, y, size, color);
    }
    void button(const std::string &title, float x, float y, float w, float h, std::function<void()> fn,
                bool enabled = true, bool accent = false) {
        bool hover = mx >= x && mx < x + w && my >= y && my < y + h;
        glm::vec4 fill = accent ? glm::vec4(.32f, .43f, .24f, 1) : glm::vec4(.18f, .23f, .19f, 1);
        if (hover && enabled)
            fill += glm::vec4(.08f, .08f, .06f, 0);
        if (!enabled)
            fill = glm::vec4(.12f, .15f, .13f, 1);
        g.rect(x, y + 3, w, h, {0, 0, 0, .45f});
        g.rect(x, y, w, h, fill);
        g.rect(x, y, w, 2, {.65f, .70f, .54f, enabled ? .35f : .10f});
        g.rect(x, y + h - 2, w, 2, {0, 0, 0, .3f});
        float size = 18, tw = float(title.size()) * size * .517f;
        label(title, x + (w - tw) / 2, y + (h - size) / 2 - 2, size, enabled ? Cream : Muted * .65f);
        buttons.push_back({x, y, w, h, std::move(fn), enabled});
    }
    void interface() {
        buttons.clear();
        g.beginUI();
        g.rect(0, 0, 1440, 75, Dark);
        g.rect(0, 74, 1440, 1, {.38f, .46f, .34f, .6f});
        g.rect(29, 22, 27, 31, Gold);
        g.rect(33, 26, 18, 22, {.20f, .27f, .18f, 1});
        g.rect(35, 29, 3, 17, Gold);
        label("BLOCKNOTES", 70, 17, 29);
        label("INTERACTIVE 3D NOTEBOOK  /  HEADLINE: SHADOW MAPPING (F3)", 300, 19, 13, Muted);
        label("SE-2408  /  Alikhan Faizrakhman, Farkhad Imanbayev, Alexander Tarulin, Alikhan Kenzhebek",
              300, 42, 12, Muted);
        if (paused)
            label("PAUSED", 940, 27, 15, Gold);
        {
            std::string mode = doc.readOnly ? "READ ONLY" : "EDIT";
            std::string state = doc.signedBook ? "SIGNED / " + mode : mode;
            label(state, 1172 - state.size() * 15 * .517f, 27, 15, doc.readOnly ? Gold : Cream);
            button(doc.readOnly ? "Allow editing" : "Make read-only", 1186, 18, 164, 38, [&] { toggleMode(); },
                   !flipping && !loadBlocked, doc.readOnly);
        }
        button("?", 1358, 18, 44, 38, [&] { help = !help; });
        g.rect(24, 103, 210, 641, Dark);
        label("YOUR NOTEBOOK", 43, 121, 14, Muted);
        // Clicking the title opens the cover dialog (title + look).
        bool titleHover = mx >= 39 && mx < 219 && my >= 143 && my < 175 && !modal && !help;
        if (titleHover)
            g.rect(39, 143, 180, 32, {.18f, .23f, .19f, 1});
        buttons.push_back({39, 143, 180, 32,
                           [&] { openCover(); }, !flipping && !loadBlocked});
        label(doc.title.size() > 17 ? doc.title.substr(0, 14) + U"..." : doc.title, 43, 150, 19,
              titleHover ? Gold : Cream);
        label(std::to_string(doc.pages.size()) + " pages", 43, 181, 15, Gold);
        g.rect(43, 222, 170, 1, {.33f, .40f, .32f, .65f});
        label("PAGES", 43, 237, 14, Muted);
        int first = std::max(0, active - 2);
        first = std::min(first, std::max(0, int(doc.pages.size()) - 6));
        for (int i = first; i < std::min(first + 6, int(doc.pages.size())); i++) {
            float y = 268 + (i - first) * 51;
            button(
                "", 39, y, 180, 44,
                [&, i] {
                    if (!flipping) {
                        choose(i);
                        opened = true;
                    }
                },
                !flipping);
            if (i == active)
                g.rect(39, y, 3, 44, Gold);
            label((i + 1 < 10 ? "0" : "") + std::to_string(i + 1), 50, y + 10, 17,
                  i == active ? Gold : Muted);
            auto excerpt = doc.pages[i].text.substr(0, 12);
            std::replace(excerpt.begin(), excerpt.end(), U'\n', U' ');
            if (excerpt.empty())
                excerpt = U"Empty page";
            label(excerpt, 85, y + 10, 16);
        }
        button(
            "+ New page", 39, 596, 180, 41, [&] { addPage(); },
            !doc.readOnly && !flipping && !loadBlocked, true);
        button(
            "Delete page", 39, 648, 180, 36, [&] { modal = 2; },
            !doc.readOnly && doc.pages.size() > 1 && !flipping && !loadBlocked);
        label("200 pages maximum", 43, 707, 13, Muted);
        drawWritingDesk();
        g.rect(260, 103, 920, 37, {.06f, .09f, .07f, .72f});
        label(doc.readOnly ? "READ MODE / pages are locked  -  press Allow editing (top right) or F6"
              : hasSelection()
                  ? "EDIT MODE / " + std::to_string(selection().second - selection().first) +
                        " selected  -  colour, font, Ctrl+C / Ctrl+X, Delete, or type to replace"
                  : std::string("EDIT MODE / click to place the cursor, drag or Shift+arrows to select"),
              280, 111, 15, Muted);
        if (!opened)
            button("Open notebook", 605, 674, 230, 48, [&] { opened = true; }, true, true);
        else {
            button("< Previous", 275, 704, 145, 42, [&] { flip(-1); }, spread > 0 && !flipping);
            label("Page " + std::to_string(active + 1) + " of " + std::to_string(doc.pages.size()), 608,
                  713, 19, Cream);
            button(
                "Next >", 1020, 704, 145, 42, [&] { flip(1); },
                spread + 2 < int(doc.pages.size()) && !flipping);
        }
        g.rect(0, 775, 1440, 125, Dark);
        g.rect(0, 775, 1440, 1, {.38f, .46f, .34f, .6f});
        button("Save", 275, 796, 126, 44, [&] { save(); }, !loadBlocked, true);
        button(
            "Undo", 417, 796, 115, 44, [&] { history(false); },
            !undo.empty() && !doc.readOnly && !flipping);
        button(
            doc.signedBook ? "Edit signature" : "Sign book", 548, 796, 158, 44,
            [&] {
                if (doc.signedBook) {
                    openEditSigned();
                    return;
                }
                modal = 1;
                signTitle = doc.title;
                signAuthor = doc.author;
                field = 0;
            },
            !flipping && !loadBlocked);
        button(
            opened ? "Done" : "Open", 722, 796, 126, 44,
            [&] {
                if (doc.dirty && !save())
                    return;
                opened = !opened;
            },
            !flipping);
        button("New book", 864, 796, 142, 44, [&] { modal = 3; }, !flipping);
        button("Load book", 1022, 796, 143, 44, [&] { listBooks(); }, !flipping);
        button("Cover", 1181, 796, 126, 44, [&] { openCover(); }, !flipping && !loadBlocked);
        label(loadBlocked ? "LOAD ERROR"
              : doc.dirty ? "UNSAVED"
                          : "SAVED",
              31, 805, 15, doc.dirty ? Gold : Muted);
        label(status.substr(0, 112), 275, 855, 14, Muted);
        label("F1 HELP", 1271, 855, 13, Muted);
        if (help) {
            buttons.clear();
            g.rect(0, 75, 1440, 825, {0, 0, 0, .66f});
            g.rect(350, 170, 740, 530, Dark);
            label("MAKE YOURSELF AT HOME", 390, 201, 27, Gold);
            const char *lines[] = {"Click / drag paper  Place the cursor / select text",
                                   "Enter / Backspace   New line / erase",
                                   "Arrows (+Shift)     Move the cursor (+select)",
                                   "Ctrl+A / C / X / V  Select all / copy / cut / paste",
                                   "Ctrl+Z / Ctrl+Y     Undo / redo",
                                   "Ctrl+S / Ctrl+N     Save / add page",
                                   "PgUp / PgDn         Turn the spread",
                                   "F2 / F3 / F4        Light / shadows (headline) / particles",
                                   "Alt+Left / Right    Rotate the light",
                                   "F7                  Pause / resume animation",
                                   "Right mouse drag    Orbit the book; scroll to zoom",
                                   "F5 / Esc            Reset view / close book",
                                   "Cover / title click Customize cover and rename",
                                   "F6 / top right      Switch Read / Edit mode"};
            for (int i = 0; i < 14; i++)
                label(lines[i], 390, 252 + i * 26, 17, Cream);
            button("Back to book", 573, 616, 294, 44, [&] { help = false; }, true, true);
        }
        drawDialogs();
    }
    void frame() {
        double now = glfwGetTime(), anim = animNow();
        updateFlip(anim);
        if (doc.dirty && !loadBlocked && now - lastEdit > 1.5 && modal == 0) {
            save();
            lastEdit = now;
        }
        bool caret = paused || int(anim * 2) % 2 == 0;
        // Only pages whose content/caret actually changed are repainted (see SlotKey).
        if (textureDirty || caret != lastCaret)
            refreshTextures(caret);
        lastCaret = caret;
        glfwGetFramebufferSize(window, &g.pixelW, &g.pixelH);
        if (g.pixelW <= 0 || g.pixelH <= 0)
            return;
        updateCamera();
        auto draws = scene(anim);
        g.render(draws, lightOn, shadowsOn);
        frames++;
        if (now - fpsStart >= 1) {
            fps = frames;
            frames = 0;
            fpsStart = now;
        }
        interface();
    }
    void release() {
        glDeleteTextures(4, pageTex);
        glDeleteTextures(1, &coverTex);
        if (previewTex)
            glDeleteTextures(1, &previewTex);
        g.release();
    }
};
} // namespace bn
