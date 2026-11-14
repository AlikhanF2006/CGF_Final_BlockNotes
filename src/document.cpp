#include "document.hpp"
#include "json.hpp"
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace bn {
std::u32string decode(const std::string &s) {
    std::u32string out;
    for (std::size_t i = 0; i < s.size();) {
        unsigned char c = s[i++];
        char32_t cp;
        int n;
        if (c < 128) {
            cp = c;
            n = 0;
        } else if ((c & 0xE0) == 0xC0) {
            cp = c & 31;
            n = 1;
        } else if ((c & 0xF0) == 0xE0) {
            cp = c & 15;
            n = 2;
        } else if ((c & 0xF8) == 0xF0) {
            cp = c & 7;
            n = 3;
        } else
            throw std::runtime_error("Invalid UTF-8");
        for (int j = 0; j < n; j++) {
            if (i >= s.size() || (static_cast<unsigned char>(s[i]) & 0xC0) != 0x80)
                throw std::runtime_error("Invalid UTF-8");
            cp = (cp << 6) | (s[i++] & 63);
        }
        if ((n == 1 && cp < 128) || (n == 2 && cp < 2048) || (n == 3 && cp < 65536) || cp > 0x10FFFF ||
            (cp >= 0xD800 && cp <= 0xDFFF))
            throw std::runtime_error("Invalid UTF-8");
        out += cp;
    }
    return out;
}
std::string encode(const std::u32string &s) {
    std::string out;
    for (char32_t c : s) {
        if (c < 128)
            out += char(c);
        else if (c < 2048) {
            out += char(0xC0 | (c >> 6));
            out += char(0x80 | (c & 63));
        } else if (c < 65536) {
            out += char(0xE0 | (c >> 12));
            out += char(0x80 | ((c >> 6) & 63));
            out += char(0x80 | (c & 63));
        } else {
            out += char(0xF0 | (c >> 18));
            out += char(0x80 | ((c >> 12) & 63));
            out += char(0x80 | ((c >> 6) & 63));
            out += char(0x80 | (c & 63));
        }
    }
    return out;
}
Grid gridFor(int size) {
    static const Grid grids[PageSizes] = {{41, 27, 12, 25}, {35, 23, 14, 29}, {27, 18, 18, 37}};
    return grids[std::clamp(size, 0, PageSizes - 1)];
}
std::vector<Cell> layout(const std::u32string &text, int cols) {
    std::vector<Cell> cells;
    cells.reserve(text.size() + 1);
    int col = 0, row = 0;
    for (std::size_t i = 0; i < text.size(); i++) {
        char32_t c = text[i];
        if (c == U'\n') {
            cells.push_back({col, row});
            col = 0;
            row++;
            continue;
        }
        if (c == U' ') {
            // A space at the end of a full line hangs in the margin instead of starting the next line.
            cells.push_back({col, row});
            if (col < cols)
                col++;
            continue;
        }
        bool wordStart = i == 0 || text[i - 1] == U' ' || text[i - 1] == U'\n';
        if (wordStart && col > 0) {
            int len = 0;
            for (std::size_t j = i; j < text.size() && text[j] != U' ' && text[j] != U'\n'; j++)
                len++;
            if (col + len > cols && len <= cols) {
                col = 0;
                row++;
            }
        }
        if (col >= cols) {
            col = 0;
            row++;
        }
        cells.push_back({col, row});
        col++;
    }
    cells.push_back({col, row});
    return cells;
}
bool fits(const std::u32string &text, int size) {
    auto g = gridFor(size);
    return layout(text, g.cols).back().row < g.rows;
}
namespace {
void fillStyles(Page &page) {
    page.styles.resize(page.text.size(), CharStyle{std::uint8_t(page.ink), std::uint8_t(page.font)});
}
} // namespace
bool Document::insert(std::size_t p, std::size_t &cursor, const std::u32string &text) {
    if (p >= pages.size())
        return false;
    CharStyle style = cursor > 0 ? pages[p].styleAt(cursor - 1)
                                 : CharStyle{std::uint8_t(pages[p].ink), std::uint8_t(pages[p].font)};
    return insert(p, cursor, text, style);
}
bool Document::insert(std::size_t p, std::size_t &cursor, const std::u32string &text, CharStyle style) {
    if (readOnly || p >= pages.size() || cursor > pages[p].text.size() || style.ink > 5 ||
        style.font >= PageFonts)
        return false;
    auto next = pages[p].text;
    std::u32string clean;
    for (auto c : text)
        if (c == U'\n' || (c >= 32 && c != 127))
            clean += c;
    if (clean.empty())
        return false;
    next.insert(cursor, clean);
    if (!fits(next, pages[p].size))
        return false;
    fillStyles(pages[p]);
    pages[p].text = next;
    pages[p].styles.insert(pages[p].styles.begin() + std::ptrdiff_t(cursor), clean.size(), style);
    cursor += clean.size();
    dirty = true;
    return true;
}
bool Document::erase(std::size_t p, std::size_t &cursor, bool forward) {
    if (readOnly || p >= pages.size() || cursor > pages[p].text.size())
        return false;
    auto &text = pages[p].text;
    fillStyles(pages[p]);
    if (forward) {
        if (cursor == text.size())
            return false;
    } else {
        if (cursor == 0)
            return false;
        --cursor;
    }
    text.erase(cursor, 1);
    pages[p].styles.erase(pages[p].styles.begin() + std::ptrdiff_t(cursor));
    dirty = true;
    return true;
}
bool Document::eraseRange(std::size_t p, std::size_t from, std::size_t to) {
    if (readOnly || p >= pages.size() || from >= to || to > pages[p].text.size())
        return false;
    fillStyles(pages[p]);
    pages[p].text.erase(from, to - from);
    pages[p].styles.erase(pages[p].styles.begin() + std::ptrdiff_t(from),
                          pages[p].styles.begin() + std::ptrdiff_t(to));
    dirty = true;
    return true;
}
bool Document::restyle(std::size_t p, std::size_t from, std::size_t to, int ink, int font) {
    if (readOnly || p >= pages.size() || from >= to || to > pages[p].text.size() || ink > 5 ||
        font >= PageFonts)
        return false;
    fillStyles(pages[p]);
    for (std::size_t i = from; i < to; i++) {
        if (ink >= 0)
            pages[p].styles[i].ink = std::uint8_t(ink);
        if (font >= 0)
            pages[p].styles[i].font = std::uint8_t(font);
    }
    dirty = true;
    return true;
}
bool Document::add(std::size_t after) {
    if (readOnly || pages.size() >= MaxPages || after >= pages.size())
        return false;
    pages.insert(pages.begin() + after + 1, Page{});
    dirty = true;
    return true;
}
bool Document::setSize(std::size_t p, int size) {
    if (readOnly || p >= pages.size() || size < 0 || size >= PageSizes)
        return false;
    if (!fits(pages[p].text, size))
        return false;
    if (pages[p].size != size) {
        pages[p].size = size;
        dirty = true;
    }
    return true;
}
bool Document::remove(std::size_t p) {
    if (readOnly || p >= pages.size() || pages.size() == 1)
        return false;
    pages.erase(pages.begin() + p);
    dirty = true;
    return true;
}
bool validName(const std::u32string &s) {
    return !s.empty() && s.size() <= 40 &&
           std::any_of(s.begin(), s.end(), [](char32_t c) { return c > 32; });
}
namespace {
std::u32string trim(const std::u32string &s) {
    auto b = s.find_first_not_of(U' '), e = s.find_last_not_of(U' ');
    return b == std::u32string::npos ? std::u32string{} : s.substr(b, e - b + 1);
}
bool validCover(const CoverStyle &c) {
    return c.color >= 0 && c.color < CoverColors && c.frame >= 0 && c.frame < CoverFrames &&
           c.emblem >= 0 && c.emblem < CoverEmblems && c.foil >= 0 && c.foil < CoverFoils;
}
} // namespace
bool Document::sign(const std::u32string &t, const std::u32string &a) {
    if (signedBook || !validName(t) || !validName(a))
        return false;
    title = trim(t);
    author = trim(a);
    signedBook = true;
    dirty = true;
    return true;
}
bool Document::editSignature(const std::u32string &t, const std::u32string &a) {
    if (!signedBook || !validName(t) || !validName(a))
        return false;
    title = trim(t);
    author = trim(a);
    dirty = true;
    return true;
}
bool Document::unsign() {
    if (!signedBook)
        return false;
    signedBook = false;
    dirty = true;
    return true;
}
void Document::setReadOnly(bool value) {
    if (readOnly != value) {
        readOnly = value;
        dirty = true;
    }
}
bool Document::rename(const std::u32string &t) {
    if (!validName(t))
        return false;
    title = trim(t);
    dirty = true;
    return true;
}
bool Document::setCover(const CoverStyle &style) {
    if (!validCover(style))
        return false;
    cover = style;
    dirty = true;
    return true;
}
bool Document::save(const std::filesystem::path &path, std::string &error) {
    try {
        nlohmann::json j = {{"format", "BlockNotes"}, {"version", 1},
                            {"title", encode(title)}, {"author", encode(author)},
                            {"signed", signedBook},   {"readOnly", readOnly},
                            {"pages", nlohmann::json::array()}};
        j["cover"] = {{"color", cover.color}, {"frame", cover.frame}, {"emblem", cover.emblem},
                      {"foil", cover.foil},   {"brand", cover.brand}};
        for (auto &p : pages)
        {
            // Character styles are stored as runs: [count, ink, font].
            nlohmann::json runs = nlohmann::json::array();
            for (std::size_t i = 0; i < p.text.size();) {
                auto st = p.styleAt(i);
                std::size_t n = 1;
                while (i + n < p.text.size() && p.styleAt(i + n) == st)
                    n++;
                runs.push_back({n, st.ink, st.font});
                i += n;
            }
            j["pages"].push_back({{"text", encode(p.text)},
                                  {"ink", p.ink},
                                  {"font", p.font},
                                  {"size", p.size},
                                  {"runs", runs}});
        }
        if (!path.parent_path().empty())
            std::filesystem::create_directories(path.parent_path());
        auto tmp = path;
        tmp += ".tmp";
        std::ofstream file(tmp, std::ios::binary | std::ios::trunc);
        if (!file)
            throw std::runtime_error("Cannot create save file");
        file << j.dump(2);
        file.flush();
        if (!file)
            throw std::runtime_error("Cannot write save file");
        file.close();
        // Keep the last good file until the replacement has been written completely.
        auto backup = path;
        backup += ".bak";
        if (std::filesystem::exists(path))
            std::filesystem::copy_file(path, backup, std::filesystem::copy_options::overwrite_existing);
        std::error_code ec;
        std::filesystem::rename(tmp, path, ec);
        if (ec) {
            std::filesystem::remove(path);
            std::filesystem::rename(tmp, path);
        }
        dirty = false;
        error.clear();
        return true;
    } catch (const std::exception &e) {
        error = e.what();
        return false;
    }
}
bool Document::load(const std::filesystem::path &path, std::string &error) {
    try {
        if (std::filesystem::file_size(path) > 4 * 1024 * 1024)
            throw std::runtime_error("File is too large");
        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("Cannot open file");
        nlohmann::json j;
        file >> j;
        if (j.at("format") != "BlockNotes" || j.at("version") != 1)
            throw std::runtime_error("Unsupported notebook format");
        Document candidate;
        candidate.pages.clear();
        candidate.title = decode(j.at("title").get<std::string>());
        candidate.author = decode(j.at("author").get<std::string>());
        candidate.signedBook = j.at("signed").get<bool>();
        // Older files had no separate mode: a signed book was always locked.
        candidate.readOnly = j.value("readOnly", candidate.signedBook);
        if (candidate.title.size() > 40 || candidate.author.size() > 40)
            throw std::runtime_error("Invalid title or author");
        if (j.contains("cover")) {
            const auto &c = j.at("cover");
            candidate.cover.color = c.value("color", 0);
            candidate.cover.frame = c.value("frame", 0);
            candidate.cover.emblem = c.value("emblem", 0);
            candidate.cover.foil = c.value("foil", 0);
            candidate.cover.brand = c.value("brand", true);
            if (!validCover(candidate.cover))
                throw std::runtime_error("Invalid cover style");
        }
        const auto &arr = j.at("pages");
        if (!arr.is_array() || arr.empty() || arr.size() > MaxPages)
            throw std::runtime_error("Invalid page count");
        for (auto &item : arr) {
            Page p{decode(item.at("text").get<std::string>()), item.at("ink").get<int>(),
                   item.value("font", 0), item.value("size", 1)};
            if (p.ink < 0 || p.ink > 5 || p.font < 0 || p.font >= PageFonts || p.size < 0 ||
                p.size >= PageSizes)
                throw std::runtime_error("Invalid page data");
            // Pages from older versions were wrapped by characters; with word wrap a nearly
            // full page may need the small size to fit.
            if (!fits(p.text, p.size)) {
                if (!fits(p.text, 0))
                    throw std::runtime_error("Invalid page data");
                p.size = 0;
            }
            for (auto c : p.text)
                if ((c < 32 && c != U'\n') || c == 127)
                    throw std::runtime_error("Invalid text character");
            fillStyles(p);
            if (item.contains("runs")) {
                std::size_t at = 0;
                for (auto &run : item.at("runs")) {
                    auto n = run.at(0).get<std::size_t>();
                    int ink = run.at(1).get<int>(), font = run.at(2).get<int>();
                    if (ink < 0 || ink > 5 || font < 0 || font >= PageFonts || n > p.text.size() - at)
                        throw std::runtime_error("Invalid text style");
                    for (std::size_t k = 0; k < n; k++)
                        p.styles[at + k] = {std::uint8_t(ink), std::uint8_t(font)};
                    at += n;
                }
                if (at != p.text.size())
                    throw std::runtime_error("Invalid text style");
            }
            candidate.pages.push_back(p);
        }
        *this = std::move(candidate);
        error.clear();
        return true;
    } catch (const std::exception &e) {
        error = e.what();
        return false;
    }
}
BookInfo peekBook(const std::filesystem::path &path) {
    BookInfo info;
    info.path = path;
    try {
        if (std::filesystem::file_size(path) > 4 * 1024 * 1024)
            return info;
        std::ifstream file(path, std::ios::binary);
        nlohmann::json j;
        file >> j;
        if (j.at("format") != "BlockNotes")
            return info;
        info.title = decode(j.at("title").get<std::string>());
        info.signedBook = j.value("signed", false);
        info.readable = true;
    } catch (...) {
    }
    return info;
}
bool deleteBook(const std::filesystem::path &path, std::string &error) {
    std::error_code ec;
    if (!std::filesystem::remove(path, ec) || ec) {
        error = ec ? ec.message() : "File not found";
        return false;
    }
    for (const char *suffix : {".bak", ".tmp"}) {
        auto extra = path;
        extra += suffix;
        std::filesystem::remove(extra, ec);
    }
    error.clear();
    return true;
}
} // namespace bn
