#pragma once
#include "document.hpp"
#include "stb_truetype.h"
#include <cstdint>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>
namespace bn {
struct Canvas {
    int w, h;
    std::vector<unsigned char> pixels;
    Canvas(int width, int height, glm::vec3 color);
    void rect(int x, int y, int width, int height, glm::vec3 color, float alpha = 1);
};
struct Glyph {
    int x, y, w, h, ox, oy;
    float advance;
};
// A rasterized glyph at one pixel size, kept so text is not re-rasterized on every redraw.
struct CachedGlyph {
    int w = 0, h = 0, ox = 0, oy = 0;
    float advance = 0;
    std::vector<unsigned char> alpha;
};
class Font {
    std::vector<unsigned char> bytes;
    stbtt_fontinfo info{};
    std::unordered_map<std::uint64_t, CachedGlyph> cache;
    const CachedGlyph &glyph(char32_t c, float size);

  public:
    GLuint atlas = 0;
    std::unordered_map<char32_t, Glyph> glyphs;
    void init(const std::string &path, bool withAtlas = true);
    void paint(Canvas &canvas, const std::u32string &text, int x, int baseline, float size,
               glm::vec3 color);
    float measure(const std::u32string &text, float size);
    // Paints text horizontally centred on centerX.
    void paintCentered(Canvas &canvas, const std::u32string &text, int centerX, int baseline,
                       float size, glm::vec3 color);
    ~Font();
};
GLuint texture(const Canvas &canvas, GLuint existing = 0, bool nearest = false);
// Paints a page; characters use their own colour/font. [selFrom, selTo) is highlighted.
Canvas paper(Font &ui, Font *const fonts[PageFonts], const Page *page, int number, bool selected,
             std::size_t cursor, bool caret, std::size_t selFrom = 0, std::size_t selTo = 0);
float glyphSize(Font &font, const Grid &grid);
// Text row under a y coordinate of the page texture (may be out of range; clamp it).
int rowAt(const Grid &grid, float y);
const char *pageFontName(int index);
Canvas coverArt(Font &font, const std::u32string &title, const std::u32string &author, bool signedBook,
                const CoverStyle &style);
glm::vec3 inkColor(int index);
glm::vec3 coverColor(int index);
glm::vec3 foilColor(int index);
const char *coverColorName(int index);
const char *frameName(int index);
const char *emblemName(int index);
const char *foilName(int index);
} // namespace bn
