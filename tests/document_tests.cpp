#include "document.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace bn;
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
int main() {
    try {
        Document d;
        std::size_t cursor = 0;
        std::string error;
        check(d.insert(0, cursor, U"First note\nSecond line"), "Insert failed");
        check(cursor == 22, "Cursor after insertion");
        cursor = 5;
        check(d.insert(0, cursor, U" edited"), "Mid-text insertion");
        check(d.pages[0].text == U"First edited note\nSecond line", "Inserted in wrong position");
        check(d.erase(0, cursor), "Backspace failed");
        check(d.erase(0, cursor, true), "Forward delete failed");
        auto unicode = U"Unicode: \u0410\u043b\u0438\u0445\u0430\u043d \u00e9 \U0001f680";
        check(decode(encode(unicode)) == unicode, "UTF-8 roundtrip");
        bool rejected = false;
        try {
            decode("\xC0\xAF");
        } catch (...) {
            rejected = true;
        }
        check(rejected, "Overlong UTF-8 accepted");
        check(!d.remove(0), "Removed last page");
        check(d.add(0), "Add page");
        cursor = 0;
        check(d.insert(1, cursor, U"Another page"), "Edit second page");
        d.pages[1].ink = 4;
        auto path = std::filesystem::temp_directory_path() / "blocknotes_test_roundtrip.json";
        check(d.save(path, error), "Save failed");
        Document loaded;
        check(loaded.load(path, error), "Load failed");
        check(loaded.pages.size() == 2 && loaded.pages[1].text == U"Another page" &&
                  loaded.pages[1].ink == 4,
              "Roundtrip lost page data");
        check(!loaded.sign(U"", U"Author"), "Empty title accepted");
        check(!loaded.sign(U"Title", U"   "), "Blank author accepted");
        check(loaded.sign(U"Field notes", U"A. Explorer"), "Signing failed");
        cursor = 0;
        check(loaded.insert(0, cursor, U"X") && loaded.erase(0, cursor), "Signed book should stay editable");
        loaded.setReadOnly(true);
        check(!loaded.insert(0, cursor, U"X"), "Read-only insert allowed");
        check(!loaded.erase(0, cursor, true), "Read-only delete allowed");
        check(!loaded.add(0) && !loaded.remove(0), "Read-only page mutation allowed");
        check(loaded.save(path, error), "Signed save failed");
        check(loaded.editSignature(U"Field notes II", U"B. Explorer"), "Edit signature failed");
        check(loaded.signedBook && loaded.title == U"Field notes II", "Edit signature lost title");
        check(!loaded.editSignature(U"", U"X"), "Empty edited title accepted");
        {
            Document temp = loaded;
            check(temp.unsign() && !temp.signedBook && temp.readOnly, "Unsign must not change the mode");
            temp.setReadOnly(false);
            cursor = 0;
            check(temp.insert(0, cursor, U"Y"), "Edit mode still locked");
            check(temp.rename(U"  Renamed  ") && temp.title == U"Renamed", "Rename/trim failed");
        }
        CoverStyle style;
        style.color = 2;
        style.frame = 1;
        style.emblem = 3;
        style.foil = 1;
        style.brand = false;
        check(loaded.setCover(style), "Cover style rejected");
        CoverStyle bad;
        bad.color = 99;
        check(!loaded.setCover(bad), "Invalid cover accepted");
        check(loaded.save(path, error), "Signed save failed");
        Document signedCopy;
        check(signedCopy.load(path, error) && signedCopy.signedBook && signedCopy.readOnly,
              "Signed/read-only state not persisted");
        check(signedCopy.cover == style && signedCopy.title == U"Field notes II", "Cover not persisted");
        check(peekBook(path).readable && peekBook(path).title == U"Field notes II", "Peek failed");
        {
            auto old = std::filesystem::temp_directory_path() / "blocknotes_test_old.json";
            std::ofstream(old) << R"({"format":"BlockNotes","version":1,"title":"Old","author":"",)"
                               << R"("signed":false,"pages":[{"text":"hi","ink":0}]})";
            Document legacy;
            check(legacy.load(old, error) && legacy.cover == CoverStyle{} && !legacy.readOnly,
                  "Old file without cover failed");
            std::ofstream(old) << R"({"format":"BlockNotes","version":1,"title":"Old","author":"A",)"
                               << R"("signed":true,"pages":[{"text":"hi","ink":0}]})";
            check(legacy.load(old, error) && legacy.signedBook && legacy.readOnly,
                  "Old signed file should open in Read mode");
            check(deleteBook(old, error) && !std::filesystem::exists(old), "Delete book failed");
            check(!deleteBook(old, error), "Deleting missing book succeeded");
        }
        std::ofstream(path) << "{broken json";
        auto old = signedCopy.pages[0].text;
        check(!signedCopy.load(path, error), "Corrupt file accepted");
        check(signedCopy.pages[0].text == old && signedCopy.signedBook, "Failed load mutated document");
        Document full;
        cursor = 0;
        check(full.insert(0, cursor, std::u32string(Columns * Rows, U'x')), "Valid capacity rejected");
        auto oldCursor = cursor;
        check(!full.insert(0, cursor, U"x"), "Overflow accepted");
        check(cursor == oldCursor, "Overflow moved cursor");
        {
            // Word wrap: "hello" does not fit after 32 chars on a 35-column line, so it moves down.
            auto cells = layout(std::u32string(32, U'a') + U" hello", Columns);
            check(cells[33].row == 1 && cells[33].col == 0, "Word was split instead of wrapped");
            // A full line followed by Enter must not leave an empty line.
            cells = layout(std::u32string(Columns, U'a') + U"\nb", Columns);
            check(cells.back().row == 1, "Full line + Enter produced a blank line");
            // Words longer than a line are still split by characters.
            cells = layout(std::u32string(40, U'z'), Columns);
            check(cells[35].row == 1 && cells[35].col == 0, "Long word not split");
            Document styled;
            std::size_t c = 0;
            check(styled.insert(0, c, std::u32string(30 * 20, U'q')), "Fill for style test");
            check(!styled.setSize(0, 2), "Large size accepted although text does not fit");
            check(styled.setSize(0, 0) && styled.pages[0].size == 0, "Size change failed");
            check(styled.restyle(0, 10, 20, 3, 2), "Restyle failed");
            c = 5;
            check(styled.insert(0, c, U"NEW", CharStyle{4, 1}), "Styled insert failed");
            check(styled.pages[0].styles.size() == styled.pages[0].text.size(), "Styles out of sync");
            check(styled.pages[0].styleAt(5) == CharStyle{4, 1} && styled.pages[0].styleAt(4) == CharStyle{},
                  "Styled insert put style in wrong place");
            check(styled.pages[0].styleAt(13) == CharStyle{3, 2} && styled.pages[0].styleAt(23) == CharStyle{},
                  "Restyle range shifted wrongly");
            check(styled.eraseRange(0, 5, 8) && styled.pages[0].styleAt(5) == CharStyle{},
                  "Erase range failed");
            auto sp = std::filesystem::temp_directory_path() / "blocknotes_style.json";
            check(styled.save(sp, error), "Style save");
            Document back;
            check(back.load(sp, error) && back.pages[0].size == 0 && back.pages[0].styleAt(12) == CharStyle{3, 2} &&
                      back.pages[0].styleAt(9) == CharStyle{} && back.pages[0].styles.size() == back.pages[0].text.size(),
                  "Styles not saved");
            std::filesystem::remove(sp);
            std::filesystem::remove(sp.string() + ".bak");
        }
        Document limit;
        for (int i = 1; i < MaxPages; i++)
            check(limit.add(i - 1), "Early page limit");
        check(!limit.add(MaxPages - 1), "Page limit bypassed");
        std::filesystem::remove(path);
        std::filesystem::remove(path.string() + ".bak");
        std::cout
            << "PASS: editing, UTF-8, page limits, JSON roundtrip, signing, read/edit mode, fonts/sizes, word wrap, cover, delete, corrupt-file protection.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << "\n";
        return 1;
    }
}
