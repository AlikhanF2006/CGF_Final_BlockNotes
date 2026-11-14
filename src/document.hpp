#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace bn {
constexpr int MaxPages = 200;
constexpr int PageFonts = 3, PageSizes = 3; // Classic / Pixel / Handwritten; S / M / L
// Character grid of a page for one text size. Pixel values are in the 640x900 page texture.
struct Grid {
    int cols, rows, cellW, rowH;
};
Grid gridFor(int size);
// Default (M) grid, kept for older code and tests.
constexpr int Columns = 35, Rows = 23;
constexpr int CoverColors = 8, CoverFrames = 4, CoverEmblems = 6, CoverFoils = 3;
std::u32string decode(const std::string &s);
std::string encode(const std::u32string &s);
struct Cell {
    int col, row;
};
// Word-wrapped layout: one cell per character plus the end position.
// Words move to the next line whole; only words longer than a line are split.
std::vector<Cell> layout(const std::u32string &text, int cols = Columns);
// Colour and font of one character.
struct CharStyle {
    std::uint8_t ink = 0, font = 0; // ink 0..5; font 0 Classic, 1 Pixel, 2 Handwritten
    bool operator==(const CharStyle &o) const { return ink == o.ink && font == o.font; }
    bool operator!=(const CharStyle &o) const { return !(*this == o); }
};
struct Page {
    Page() = default;
    Page(std::u32string t, int inkIndex = 0, int fontIndex = 0, int sizeIndex = 1)
        : text(std::move(t)), ink(inkIndex), font(fontIndex), size(sizeIndex) {}
    std::u32string text;
    int ink = 0;  // style for text without per-character styles (older files)
    int font = 0;
    int size = 1; // 0 S, 1 M, 2 L - the grid is per page
    std::vector<CharStyle> styles; // one per character of text
    CharStyle styleAt(std::size_t i) const {
        return i < styles.size() ? styles[i] : CharStyle{std::uint8_t(ink), std::uint8_t(font)};
    }
};
bool fits(const std::u32string &text, int size);
// Cosmetic look of the notebook cover. Stored as an optional "cover" object in the JSON,
// so older files without it still load with the classic look.
struct CoverStyle {
    int color = 0;   // leather colour, 0..CoverColors-1
    int frame = 0;   // border style, 0..CoverFrames-1
    int emblem = 0;  // pixel emblem, 0..CoverEmblems-1
    int foil = 0;    // gold / silver / copper lettering
    bool brand = true; // small "BLOCKNOTES" line at the top
    bool operator==(const CoverStyle &o) const {
        return color == o.color && frame == o.frame && emblem == o.emblem && foil == o.foil &&
               brand == o.brand;
    }
    bool operator!=(const CoverStyle &o) const { return !(*this == o); }
};
struct BookInfo {
    std::filesystem::path path;
    std::u32string title;
    bool signedBook = false;
    bool readable = false;
};
bool validName(const std::u32string &s);
class Document {
  public:
    std::vector<Page> pages{{U"", 0}};
    std::u32string title = U"Untitled notebook", author;
    CoverStyle cover;
    bool signedBook = false; // has a title/author signature (shown on the cover)
    bool readOnly = false;   // Read mode: page text cannot be changed
    bool dirty = false;
    bool insert(std::size_t page, std::size_t &cursor, const std::u32string &text);
    bool insert(std::size_t page, std::size_t &cursor, const std::u32string &text, CharStyle style);
    bool erase(std::size_t page, std::size_t &cursor, bool forward = false);
    // Removes characters [from, to).
    bool eraseRange(std::size_t page, std::size_t from, std::size_t to);
    // Recolours / changes font of characters [from, to); -1 keeps that property.
    bool restyle(std::size_t page, std::size_t from, std::size_t to, int ink, int font);
    bool add(std::size_t after);
    bool remove(std::size_t page);
    // Changes the text size (grid) of one page; fails if the text would not fit.
    bool setSize(std::size_t page, int size);
    bool sign(const std::u32string &newTitle, const std::u32string &newAuthor);
    // Change title/author of an already signed book; it stays signed.
    bool editSignature(const std::u32string &newTitle, const std::u32string &newAuthor);
    // Remove the signature (author line and SIGNED EDITION on the cover).
    bool unsign();
    // Switch between Read mode (pages locked) and Edit mode. Independent of the signature.
    void setReadOnly(bool value);
    // Book title (cover dialog); allowed in any mode.
    bool rename(const std::u32string &newTitle);
    bool setCover(const CoverStyle &style);
    bool save(const std::filesystem::path &path, std::string &error);
    bool load(const std::filesystem::path &path, std::string &error);
};
// Reads only the title of a saved book for the Load dialog.
BookInfo peekBook(const std::filesystem::path &path);
// Deletes a saved book together with its .bak / .tmp companions.
bool deleteBook(const std::filesystem::path &path, std::string &error);
} // namespace bn
