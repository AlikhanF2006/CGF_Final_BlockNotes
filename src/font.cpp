#define STB_TRUETYPE_IMPLEMENTATION
#include "font.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
namespace bn {
Canvas::Canvas(int width, int height, glm::vec3 color)
    : w(width), h(height), pixels(std::size_t(width) * height * 4, 255) {
    rect(0, 0, w, h, color);
}
void Canvas::rect(int x, int y, int width, int height, glm::vec3 color, float alpha) {
    int x0 = std::max(0, x), x1 = std::min(w, x + width), y0 = std::max(0, y), y1 = std::min(h, y + height);
    if (x0 >= x1 || y0 >= y1 || alpha <= 0)
        return;
    int a = int(std::clamp(alpha, 0.f, 1.f) * 256 + .5f), inv = 256 - a;
    int col[3];
    for (int k = 0; k < 3; k++)
        col[k] = int(std::clamp(color[k], 0.f, 1.f) * 255 + .5f) * a;
    for (int py = y0; py < y1; py++) {
        unsigned char *row = pixels.data() + (std::size_t(py) * w + x0) * 4;
        for (int px = x0; px < x1; px++, row += 4)
            for (int k = 0; k < 3; k++)
                row[k] = static_cast<unsigned char>((row[k] * inv + col[k]) >> 8);
    }
}
void Font::init(const std::string &path, bool withAtlas) {
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Font missing: " + path);
    bytes.assign(std::istreambuf_iterator<char>(file), {});
    if (!stbtt_InitFont(&info, bytes.data(), 0))
        throw std::runtime_error("Invalid font: " + path);
    if (!withAtlas)
        return; // page-only fonts are painted from the glyph cache, no UI atlas needed
    Canvas canvas(1024, 1024, {1, 1, 1});
    for (std::size_t i = 3; i < canvas.pixels.size(); i += 4)
        canvas.pixels[i] = 0;
    int cell = 0;
    float scale = stbtt_ScaleForPixelHeight(&info, 24);
    for (char32_t c = 32; c < 0x530; c++) {
        if (c > 126 && c < 0x400)
            continue;
        int w, h, ox, oy, adv, lsb;
        unsigned char *bitmap = stbtt_GetCodepointBitmap(&info, 0, scale, c, &w, &h, &ox, &oy);
        int x = (cell % 32) * 32, y = (cell / 32) * 40;
        cell++;
        stbtt_GetCodepointHMetrics(&info, c, &adv, &lsb);
        glyphs[c] = {x, y, w, h, ox, oy, adv * scale};
        for (int j = 0; j < h; j++)
            for (int i = 0; i < w; i++)
                canvas.pixels[((y + j) * 1024 + x + i) * 4 + 3] = bitmap[j * w + i];
        stbtt_FreeBitmap(bitmap, nullptr);
    }
    atlas = texture(canvas);
}
const CachedGlyph &Font::glyph(char32_t c, float size) {
    auto key = (std::uint64_t(c) << 20) | std::uint64_t(size * 16);
    auto it = cache.find(key);
    if (it != cache.end())
        return it->second;
    CachedGlyph g;
    float scale = stbtt_ScaleForPixelHeight(&info, size);
    unsigned char *bitmap = stbtt_GetCodepointBitmap(&info, 0, scale, c, &g.w, &g.h, &g.ox, &g.oy);
    if (bitmap) {
        g.alpha.assign(bitmap, bitmap + std::size_t(g.w) * g.h);
        stbtt_FreeBitmap(bitmap, nullptr);
    } else
        g.w = g.h = 0;
    int adv, lsb;
    stbtt_GetCodepointHMetrics(&info, c, &adv, &lsb);
    g.advance = adv * scale;
    return cache.emplace(key, std::move(g)).first->second;
}
void Font::paint(Canvas &canvas, const std::u32string &text, int x, int baseline, float size,
                 glm::vec3 color) {
    int col[3];
    for (int k = 0; k < 3; k++)
        col[k] = int(std::clamp(color[k], 0.f, 1.f) * 255 + .5f);
    float pen = float(x);
    for (char32_t c : text) {
        const auto &g = glyph(c, size);
        int gx = int(pen) + g.ox, gy = baseline + g.oy;
        for (int j = 0; j < g.h; j++) {
            int py = gy + j;
            if (py < 0 || py >= canvas.h)
                continue;
            for (int i = 0; i < g.w; i++) {
                int px = gx + i;
                int a = g.alpha[std::size_t(j) * g.w + i];
                if (px < 0 || px >= canvas.w || a == 0)
                    continue;
                unsigned char *d = canvas.pixels.data() + (std::size_t(py) * canvas.w + px) * 4;
                for (int k = 0; k < 3; k++)
                    d[k] = static_cast<unsigned char>((d[k] * (255 - a) + col[k] * a + 127) / 255);
            }
        }
        pen += g.advance;
    }
}
float Font::measure(const std::u32string &text, float size) {
    float w = 0;
    for (char32_t c : text)
        w += glyph(c, size).advance;
    return w;
}
void Font::paintCentered(Canvas &canvas, const std::u32string &text, int centerX, int baseline,
                         float size, glm::vec3 color) {
    paint(canvas, text, int(std::round(centerX - measure(text, size) / 2)), baseline, size, color);
}
Font::~Font() {
    if (atlas)
        glDeleteTextures(1, &atlas);
}
GLuint texture(const Canvas &c, GLuint existing, bool nearest) {
    GLuint t = existing;
    if (!t)
        glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    if (existing) {
        // Same-size update: no reallocation of GPU memory.
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, c.w, c.h, GL_RGBA, GL_UNSIGNED_BYTE, c.pixels.data());
        return t;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, c.w, c.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, c.pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nearest ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, nearest ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return t;
}
glm::vec3 inkColor(int i) {
    static glm::vec3 c[] = {{.16f, .13f, .10f}, {.58f, .16f, .13f}, {.13f, .29f, .43f},
                            {.20f, .34f, .16f}, {.42f, .22f, .48f}, {.56f, .36f, .08f}};
    return c[std::clamp(i, 0, 5)];
}
namespace {
// Text row geometry for a grid: baseline of row r, ruled line under it, caret box.
int baselineY(const Grid &g, int row) { return 127 + g.rowH - 7 + row * g.rowH; }
int ruleOffset(const Grid &g) { return int(std::lround(g.rowH * .38f)); }
// The paper grain, edge shading and ruled lines never change, so they are drawn once per size.
const Canvas &paperBase(Font &font, int size) {
    static std::unique_ptr<Canvas> bases[PageSizes];
    size = std::clamp(size, 0, PageSizes - 1);
    auto &base = bases[size];
    if (!base) {
        base = std::make_unique<Canvas>(640, 900, glm::vec3{.95f, .91f, .80f});
        Canvas &c = *base;
        unsigned rng = 1927;
        for (int y = 0; y < 900; y += 6)
            for (int x = 0; x < 640; x += 6) {
                rng = 1664525 * rng + 1013904223;
                float n = float((rng >> 24) & 255) / 255.f;
                c.rect(x, y, 6, 6, {.68f, .58f, .36f}, .018f + n * .025f);
            }
        for (int x = 0; x < 32; x++) {
            float a = .15f * (1 - x / 32.f);
            c.rect(x, 0, 1, 900, {.35f, .20f, .10f}, a);
            c.rect(639 - x, 0, 1, 900, {.35f, .20f, .10f}, a * .5f);
        }
        c.rect(45, 45, 550, 2, {.63f, .53f, .33f});
        c.rect(45, 837, 550, 2, {.63f, .53f, .33f});
        font.paint(c, U"BLOCKNOTES", 65, 83, 19, {.46f, .42f, .30f});
        auto g = gridFor(size);
        for (int r = 0; r < g.rows; r++)
            c.rect(70, baselineY(g, r) + ruleOffset(g), 500, 1, {.71f, .66f, .53f}, .25f);
    }
    return *base;
}
} // namespace
float glyphSize(Font &font, const Grid &g) {
    // Fit the font's character width into the grid cell, but never taller than the row.
    float perPixel = font.measure(U"M", 100) / 100.f;
    float size = perPixel > 0 ? (g.cellW - 1) / perPixel : 25.f;
    return std::min(size, g.rowH * .86f);
}
int rowAt(const Grid &g, float y) { return int(std::floor((y - 125) / g.rowH)); }
Canvas paper(Font &ui, Font *const fonts[PageFonts], const Page *page, int number, bool selected,
             std::size_t cursor, bool caret, std::size_t selFrom, std::size_t selTo) {
    Canvas c = paperBase(ui, page ? page->size : 1);
    ui.paint(c, decode("/ " + std::to_string(number)), 510, 83, 19, {.46f, .42f, .30f});
    if (page) {
        auto g = gridFor(page->size);
        auto cells = layout(page->text, g.cols);
        float sizes[PageFonts];
        for (int f = 0; f < PageFonts; f++)
            sizes[f] = glyphSize(*fonts[f], g);
        int caretUp = int(std::lround(g.rowH * .72f)), caretH = int(std::lround(g.rowH * .86f));
        std::u32string one(1, U' ');
        for (std::size_t i = 0; i < page->text.size(); i++) {
            auto at = cells[i];
            int x = 72 + at.col * g.cellW, y = baselineY(g, at.row);
            if (selected && i >= selFrom && i < selTo && page->text[i] != U'\n')
                c.rect(x, y - caretUp, g.cellW, g.rowH, {.45f, .65f, .55f}, .45f);
            if (page->text[i] != U'\n' && page->text[i] != U' ') {
                one[0] = page->text[i];
                auto st = page->styleAt(i);
                int f = std::min<int>(st.font, PageFonts - 1);
                fonts[f]->paint(c, one, x, y, sizes[f], inkColor(st.ink));
            }
        }
        if (selected && caret) {
            auto at = cells[std::min(cursor, page->text.size())];
            c.rect(72 + at.col * g.cellW, baselineY(g, at.row) - caretUp, 2, caretH, {.28f, .36f, .21f});
        }
        ui.paint(c, decode(std::to_string(number)), 310, 872, 21, {.41f, .36f, .26f});
    } else
        ui.paint(c, U"A new story awaits.", 167, 455, 25, {.62f, .56f, .44f});
    return c;
}
glm::vec3 coverColor(int i) {
    static const glm::vec3 c[CoverColors] = {{.24f, .13f, .075f}, {.38f, .08f, .09f}, {.09f, .14f, .28f},
                                             {.10f, .23f, .13f},  {.25f, .10f, .27f}, {.12f, .12f, .13f},
                                             {.06f, .24f, .25f},  {.50f, .39f, .23f}};
    return c[std::clamp(i, 0, CoverColors - 1)];
}
glm::vec3 foilColor(int i) {
    static const glm::vec3 c[CoverFoils] = {{.86f, .69f, .37f}, {.82f, .84f, .86f}, {.83f, .50f, .31f}};
    return c[std::clamp(i, 0, CoverFoils - 1)];
}
const char *coverColorName(int i) {
    static const char *n[CoverColors] = {"Saddle brown", "Crimson", "Navy",     "Forest",
                                         "Plum",         "Charcoal", "Teal", "Sand"};
    return n[std::clamp(i, 0, CoverColors - 1)];
}
const char *frameName(int i) {
    static const char *n[CoverFrames] = {"Classic", "Double", "Corners", "None"};
    return n[std::clamp(i, 0, CoverFrames - 1)];
}
const char *emblemName(int i) {
    static const char *n[CoverEmblems] = {"None", "Block", "Diamond", "Star", "Tree", "Heart"};
    return n[std::clamp(i, 0, CoverEmblems - 1)];
}
const char *pageFontName(int i) {
    static const char *n[PageFonts] = {"Classic", "Pixel", "Handwritten"};
    return n[std::clamp(i, 0, PageFonts - 1)];
}
const char *foilName(int i) {
    static const char *n[CoverFoils] = {"Gold", "Silver", "Copper"};
    return n[std::clamp(i, 0, CoverFoils - 1)];
}
namespace {
// 11x11 pixel emblems: '#' foil, '+' darker foil, '-' shadow, '.' empty.
const char *const Emblems[CoverEmblems][11] = {
    {},
    {"###########", "#---------#", "#-##------#", "#-##------#", "#-##------#", "#-##------#",
     "#-##------#", "#-##------#", "#-##------#", "#---------#", "###########"},
    {".....#.....", "....###....", "...##+##...", "..##+++##..", ".##+++++##.", "##+++++++##",
     ".##+++++##.", "..##+++##..", "...##+##...", "....###....", ".....#....."},
    {".....#.....", ".....#.....", "....###....", "....#+#....", "###########", ".##+++++##.",
     "..#+++++#..", "...#####...", "..###.###..", ".##.....##.", "##.......##"},
    {".....#.....", "....###....", "...##+##...", "..###+###..", "...#####...", "..##+#+##..",
     ".###+++###.", "..#######..", ".##+###+##.", ".....+.....", "....+++...."},
    {"...........", ".###...###.", "##+##.##+##", "#+++###+++#", "#+++++++++#", "#+++++++++#",
     ".#+++++++#.", "..#+++++#..", "...#+++#...", "....#+#....", ".....#....."}};
std::vector<std::u32string> wrapTitle(Font &font, const std::u32string &title, float size, float width) {
    std::vector<std::u32string> words, lines;
    std::u32string word;
    for (char32_t c : title) {
        if (c == U' ') {
            if (!word.empty())
                words.push_back(word);
            word.clear();
        } else
            word += c;
    }
    if (!word.empty())
        words.push_back(word);
    std::u32string line;
    for (auto &w : words) {
        auto candidate = line.empty() ? w : line + U" " + w;
        if (font.measure(candidate, size) <= width) {
            line = candidate;
            continue;
        }
        if (!line.empty())
            lines.push_back(line);
        line.clear();
        // A single word wider than the cover is split by characters.
        std::u32string part;
        for (char32_t c : w) {
            if (!part.empty() && font.measure(part + c, size) > width) {
                lines.push_back(part);
                part.clear();
            }
            part += c;
        }
        line = part;
    }
    if (!line.empty())
        lines.push_back(line);
    return lines;
}
} // namespace
Canvas coverArt(Font &font, const std::u32string &title, const std::u32string &author, bool signedBook,
                const CoverStyle &style) {
    const glm::vec3 base = coverColor(style.color), foil = foilColor(style.foil);
    const glm::vec3 dimFoil = foil * .78f, shade = base * .6f;
    Canvas cover(640, 900, base);
    // Subtle leather grain and a darker spine edge on the left.
    unsigned rng = 4211;
    for (int y = 0; y < 900; y += 5)
        for (int x = 0; x < 640; x += 5) {
            rng = 1664525 * rng + 1013904223;
            cover.rect(x, y, 5, 5, {0, 0, 0}, float((rng >> 24) & 255) / 255.f * .07f);
        }
    for (int x = 0; x < 26; x++)
        cover.rect(x, 0, 1, 900, {0, 0, 0}, .28f * (1 - x / 26.f));
    switch (style.frame) {
    case 0: // classic: four thin nested lines
        for (int k = 0; k < 4; k++) {
            cover.rect(30 + k * 4, 35 + k * 4, 580 - k * 8, 3, dimFoil);
            cover.rect(30 + k * 4, 862 - k * 4, 580 - k * 8, 3, dimFoil);
            cover.rect(30 + k * 4, 35 + k * 4, 3, 830 - k * 8, dimFoil);
            cover.rect(607 - k * 4, 35 + k * 4, 3, 830 - k * 8, dimFoil);
        }
        break;
    case 1: // double: thick outer rule and a thin inner one
        cover.rect(28, 30, 584, 9, dimFoil);
        cover.rect(28, 861, 584, 9, dimFoil);
        cover.rect(28, 30, 9, 840, dimFoil);
        cover.rect(603, 30, 9, 840, dimFoil);
        cover.rect(52, 54, 536, 2, dimFoil);
        cover.rect(52, 844, 536, 2, dimFoil);
        cover.rect(52, 54, 2, 792, dimFoil);
        cover.rect(586, 54, 2, 792, dimFoil);
        break;
    case 2: // corner brackets with block studs
        for (int sx : {0, 1})
            for (int sy : {0, 1}) {
                int x = sx ? 610 : 30, y = sy ? 870 : 30, dx = sx ? -1 : 1, dy = sy ? -1 : 1;
                cover.rect(dx > 0 ? x : x - 110, dy > 0 ? y : y - 8, 110, 8, dimFoil);
                cover.rect(dx > 0 ? x : x - 8, dy > 0 ? y : y - 110, 8, 110, dimFoil);
                cover.rect(dx > 0 ? x + 20 : x - 36, dy > 0 ? y + 20 : y - 36, 16, 16, foil);
            }
        break;
    default:
        break;
    }
    if (style.brand)
        font.paintCentered(cover, U"B L O C K N O T E S", 320, 120, 22, dimFoil);
    int titleTop = 230;
    if (style.emblem > 0 && style.emblem < CoverEmblems) {
        const int px = 12, left = 320 - 11 * px / 2, top = style.brand ? 165 : 140;
        for (int r = 0; r < 11; r++)
            for (int c = 0; c < 11; c++) {
                char ch = Emblems[style.emblem][r][c];
                if (ch == '#')
                    cover.rect(left + c * px, top + r * px, px, px, foil);
                else if (ch == '+')
                    cover.rect(left + c * px, top + r * px, px, px, dimFoil * .85f);
                else if (ch == '-')
                    cover.rect(left + c * px, top + r * px, px, px, shade);
            }
        titleTop = top + 11 * px + 40;
    }
    // Title: auto-sized, word-wrapped, centred in the space between emblem and signature.
    const int titleBottom = 640;
    const float width = 470;
    std::vector<std::u32string> lines;
    float size = 30;
    for (float s : {64.f, 56.f, 48.f, 42.f, 36.f, 30.f}) {
        lines = wrapTitle(font, title, s, width);
        if (lines.size() <= 3) {
            size = s;
            break;
        }
    }
    if (lines.size() > 3) {
        lines.resize(3);
        lines[2] += U"...";
    }
    float lineH = size * 1.22f;
    float blockH = lines.size() * lineH;
    float y = titleTop + (titleBottom - titleTop - blockH) / 2 + size * .82f;
    for (auto &line : lines) {
        font.paintCentered(cover, line, 320, int(y), size, foil);
        y += lineH;
    }
    cover.rect(320 - 60, int(y - lineH + size * .5f), 120, 3, dimFoil);
    if (signedBook)
        font.paintCentered(cover, U"SIGNED EDITION", 320, 718, 22, dimFoil);
    if (!author.empty()) {
        auto byline = U"by " + author;
        float s = 28;
        while (s > 16 && font.measure(byline, s) > 500)
            s -= 2;
        font.paintCentered(cover, byline, 320, 766, s, foil * .95f);
    }
    return cover;
}
} // namespace bn
