// Dialogs: sign / edit signature, cover editor, load and delete books, page and notebook prompts.
// Module owner: Alexander Tarulin.
#include "app.hpp"

namespace bn {
void App::drawDialogs() {
if (modal) {
    buttons.clear();
    g.rect(0, 0, 1440, 900, {0, 0, 0, .73f});
    if (modal != 7)
        g.rect(390, 240, 660, 420, Dark);
    if (modal == 1) {
        label("SIGN YOUR BOOK", 430, 267, 27, Gold);
        label("Adds your name to the cover. You can keep writing.", 430, 312, 17);
        label("Title", 430, 352, 15, Muted);
        label("Author", 430, 438, 15, Muted);
        for (int i = 0; i < 2; i++) {
            float y = 380.f + i * 86;
            g.rect(430, y, 580, 42, {.13f, .19f, .14f, 1});
            g.rect(430, y + 40, 580, 2, i == field ? Gold : Muted);
            label((i == 0 ? signTitle : signAuthor) + (i == field ? U"|" : U""), 440, y + 8, 18);
            buttons.push_back({430, y, 580, 42, [&, i] { field = i; }, true});
        }
        label("Backspace to edit. Tab switches fields.", 430, 521, 15, Muted);
        button("Cancel", 430, 583, 180, 43, [&] { modal = 0; });
        button(
            "Sign book", 630, 583, 380, 43,
            [&] {
                if (doc.sign(signTitle, signAuthor)) {
                    textureDirty = true;
                    if (save()) {
                        modal = 0;
                        tell("Book signed.");
                    } else {
                        doc.signedBook = false;
                        doc.dirty = true;
                    }
                } else
                    tell("Enter a title and author (1-40 characters).");
            },
            !signTitle.empty() && !signAuthor.empty(), true);
    } else if (modal == 2) {
        label("DELETE PAGE " + std::to_string(active + 1) + "?", 430, 282, 27, Gold);
        label("The selected page will be removed.", 430, 350, 19);
        label("You can undo this with Undo.", 430, 388, 18, Muted);
        button("Cancel", 430, 555, 245, 47, [&] { modal = 0; });
        button(
            "Delete", 695, 555, 315, 47,
            [&] {
                checkpoint();
                doc.remove(active);
                choose(std::min(active, int(doc.pages.size()) - 1));
                changed();
                modal = 0;
            },
            true, true);
    } else if (modal == 4) {
        label("LOAD A NOTEBOOK", 430, 265, 27, Gold);
        label("Saved books in the current folder", 430, 307, 16, Muted);
        if (books.empty())
            label("No saved books yet.", 430, 365, 18);
        for (int i = bookListOffset; i < std::min(bookListOffset + 4, int(books.size())); i++) {
            const auto &book = books[i];
            float y = 347.f + (i - bookListOffset) * 47;
            button("", 430, y, 512, 39, [&, i] { loadBook(books[i].path); });
            if (book.path.lexically_normal() == savePath.lexically_normal())
                g.rect(430, y, 4, 39, Gold);
            std::u32string title = book.readable ? book.title : U"(unreadable file)";
            if (title.size() > 22)
                title = title.substr(0, 20) + U"..";
            label(title, 446, y + 9, 17, book.readable ? Cream : Muted);
            auto name = book.path.filename().string();
            if (name.size() > 26)
                name = name.substr(0, 11) + ".." + name.substr(name.size() - 13);
            label(name, 940 - name.size() * 13 * .517f - 8, y + 13, 13, Muted);
            if (book.signedBook)
                label("signed", 446 + title.size() * 17 * .517f + 12, y + 13, 12, Gold);
            // Red "X" deletes this book (after confirmation).
            float dx = 950;
            bool hover = mx >= dx && mx < dx + 60 && my >= y && my < y + 39;
            g.rect(dx, y + 3, 60, 39, {0, 0, 0, .45f});
            g.rect(dx, y, 60, 39, hover ? glm::vec4(.62f, .20f, .17f, 1) : glm::vec4(.42f, .14f, .12f, 1));
            label("X", dx + 24, y + 9, 18, Cream);
            buttons.push_back({dx, y, 60, 39,
                               [&, i] {
                                   deleteTarget = books[i].path;
                                   deleteTitle = books[i].readable ? books[i].title
                                                                   : U"(unreadable file)";
                                   deleteIsCurrent = samePath(deleteTarget, savePath);
                                   modal = 5;
                               },
                               true});
        }
        label("You can also drop a JSON file onto the window.", 430, 548, 15, Muted);
        button(
            "<", 430, 592, 65, 40, [&] { bookListOffset = std::max(0, bookListOffset - 4); },
            bookListOffset > 0);
        button("Close", 515, 592, 410, 40, [&] { modal = 0; });
        button(
            ">", 945, 592, 65, 40, [&] { bookListOffset += 4; },
            bookListOffset + 4 < int(books.size()));
    } else if (modal == 5) {
        label("DELETE THIS BOOK?", 430, 270, 27, Gold);
        label(deleteTitle, 430, 330, 21);
        label(deleteTarget.filename().string(), 430, 365, 15, Muted);
        label("The file and its backup will be removed.", 430, 420, 18);
        label("This cannot be undone.", 430, 452, 17, {.86f, .45f, .38f, 1});
        if (deleteIsCurrent)
            label("This is the book that is open right now.", 430, 490, 15, Gold);
        button("Cancel", 430, 555, 245, 47, [&] { modal = 4; });
        button("Delete forever", 695, 555, 315, 47, [&] { removeBook(deleteTarget); }, true, true);
    } else if (modal == 6) {
        label("EDIT SIGNATURE", 430, 267, 27, Gold);
        label("Change the title and author, or remove the signature.", 430, 312, 17);
        label("Title", 430, 352, 15, Muted);
        label("Author", 430, 438, 15, Muted);
        for (int i = 0; i < 2; i++) {
            float y = 380.f + i * 86;
            g.rect(430, y, 580, 42, {.13f, .19f, .14f, 1});
            g.rect(430, y + 40, 580, 2, i == field ? Gold : Muted);
            label((i == 0 ? signTitle : signAuthor) + (i == field ? U"|" : U""), 440, y + 8, 18);
            buttons.push_back({430, y, 580, 42, [&, i] { field = i; }, true});
        }
        label("Read/Edit mode is switched at the top right.", 430, 521, 14, Muted);
        button("Cancel", 430, 583, 150, 43, [&] { modal = 0; });
        button("Remove signature", 595, 583, 200, 43, [&] {
            doc.unsign();
            textureDirty = true;
            modal = 0;
            if (save())
                tell("Signature removed.");
        });
        button(
            "Save changes", 810, 583, 200, 43,
            [&] {
                if (!doc.editSignature(signTitle, signAuthor)) {
                    tell("Enter a title and author (1-40 characters).");
                    return;
                }
                textureDirty = true;
                modal = 0;
                if (save())
                    tell("Signature updated.");
            },
            validName(signTitle) && validName(signAuthor), true);
    } else if (modal == 7) {
        g.rect(290, 118, 860, 672, Dark);
        label("CUSTOMIZE COVER", 330, 140, 27, Gold);
        if (previewDirty) {
            previewTex = texture(coverArt(font, signTitle.empty() ? doc.title : signTitle,
                                          doc.author, doc.signedBook, coverDraft),
                                 previewTex);
            previewDirty = false;
        }
        g.rect(336, 201, 300, 422, {0, 0, 0, .5f});
        g.rect(330, 195, 300, 422, {1, 1, 1, 1}, previewTex);
        label("Live preview", 330, 632, 13, Muted);
        const float rx = 670, rw = 440;
        label("Title", rx, 192, 15, Muted);
        g.rect(rx, 216, rw, 42, {.13f, .19f, .14f, 1});
        g.rect(rx, 256, rw, 2, Gold);
        label(signTitle + U"|", rx + 10, 225, 18);
        label(std::string("Leather: ") + coverColorName(coverDraft.color), rx, 284, 15, Muted);
        for (int i = 0; i < CoverColors; i++) {
            float x = rx + i * 55, y = 308;
            auto c = coverColor(i);
            g.rect(x - 2, y - 2, 48, 44, coverDraft.color == i ? Gold : glm::vec4(.3f, .35f, .29f, 1));
            g.rect(x, y, 44, 40, glm::vec4(c * 1.15f, 1));
            buttons.push_back({x, y, 44, 40,
                               [&, i] {
                                   coverDraft.color = i;
                                   previewDirty = true;
                               },
                               true});
        }
        auto chooser = [&](const char *name, const char *value, float y, int &v, int count) {
            label(name, rx, y, 15, Muted);
            float by = y + 24;
            button("<", rx, by, 56, 38, [&v, count, this] {
                v = (v + count - 1) % count;
                previewDirty = true;
            });
            g.rect(rx + 66, by, rw - 132, 38, {.13f, .19f, .14f, 1});
            std::string text = value;
            label(text, rx + 66 + (rw - 132 - text.size() * 18 * .517f) / 2, by + 8, 18, Cream);
            button(">", rx + rw - 56, by, 56, 38, [&v, count, this] {
                v = (v + 1) % count;
                previewDirty = true;
            });
        };
        chooser("Frame", frameName(coverDraft.frame), 368, coverDraft.frame, CoverFrames);
        chooser("Emblem", emblemName(coverDraft.emblem), 444, coverDraft.emblem, CoverEmblems);
        chooser("Lettering", foilName(coverDraft.foil), 520, coverDraft.foil, CoverFoils);
        button(coverDraft.brand ? "BLOCKNOTES line: ON" : "BLOCKNOTES line: OFF", rx, 606, rw, 40,
               [&] {
                   coverDraft.brand = !coverDraft.brand;
                   previewDirty = true;
               });
        label("Title is centred and resized automatically.", 330, 672, 14, Muted);
        button("Cancel", 330, 715, 300, 46, [&] { modal = 0; });
        button(
            "Apply cover", rx, 715, rw, 46,
            [&] {
                bool renamed = signTitle != doc.title;
                if (renamed && !validName(signTitle)) {
                    tell("Title must be 1-40 characters.");
                    return;
                }
                if (!renamed && coverDraft == doc.cover) {
                    modal = 0;
                    return;
                }
                checkpoint();
                if (renamed)
                    doc.rename(signTitle);
                doc.setCover(coverDraft);
                changed();
                modal = 0;
                tell("Cover updated.");
            },
            true, true);
    } else if (modal == 3) {
        label("START A NEW NOTEBOOK?", 430, 282, 27, Gold);
        label("Your current book will be saved first.", 430, 350, 18);
        label("A new file is created; old books stay safe.", 430, 388, 17, Muted);
        button("Cancel", 430, 555, 245, 47, [&] { modal = 0; });
        button(
            "Create new", 695, 555, 315, 47,
            [&] {
                if (!loadBlocked && !save())
                    return;
                auto dir = savePath.parent_path();
                auto n = std::chrono::system_clock::now().time_since_epoch().count();
                savePath = dir / ("notebook_" + std::to_string(n) + ".json");
                loadBlocked = false;
                doc = Document{};
                doc.dirty = true;
                undo.clear();
                redo.clear();
                choose(0);
                opened = true;
                modal = 0;
                save();
            },
            true, true);
    }
}
}
void App::loadBook(const fs::path &path) {
    if (flipping)
        return;
    if (doc.dirty && !loadBlocked && !save())
        return;
    Document next;
    std::string error;
    if (!next.load(path, error)) {
        tell("Load failed: " + error);
        return;
    }
    doc = std::move(next);
    savePath = path;
    loadBlocked = false;
    undo.clear();
    redo.clear();
    choose(0);
    opened = true;
    modal = 0;
    tell("Loaded " + path.filename().string());
}
void App::scanBooks() {
    books.clear();
    std::error_code ec;
    for (auto &entry : fs::directory_iterator(savePath.parent_path(), ec))
        if (entry.is_regular_file() && entry.path().extension() == ".json")
            books.push_back(peekBook(entry.path()));
    std::sort(books.begin(), books.end(),
              [](const BookInfo &a, const BookInfo &b) { return a.path < b.path; });
    bookListOffset = std::clamp(bookListOffset, 0, std::max(0, (int(books.size()) - 1) / 4 * 4));
}
void App::listBooks() {
    if (doc.dirty && !loadBlocked && !save())
        return;
    bookListOffset = 0;
    scanBooks();
    modal = 4;
}
bool App::samePath(const fs::path &a, const fs::path &b) {
    std::error_code ec;
    if (fs::exists(a, ec) && fs::exists(b, ec))
        return fs::equivalent(a, b, ec);
    return fs::absolute(a).lexically_normal() == fs::absolute(b).lexically_normal();
}
void App::removeBook(const fs::path &path) {
    bool current = samePath(path, savePath);
    std::string error;
    if (!deleteBook(path, error)) {
        tell("Delete failed: " + error);
        modal = 4;
        return;
    }
    auto name = path.filename().string();
    if (current) {
        // The open book is gone: switch to another saved book, or start a blank one.
        doc.dirty = false;
        scanBooks();
        auto next = std::find_if(books.begin(), books.end(), [](const BookInfo &b) { return b.readable; });
        if (next != books.end()) {
            loadBook(next->path);
        } else {
            doc = Document{};
            savePath = savePath.parent_path() / "notebook.json";
            loadBlocked = false;
            undo.clear();
            redo.clear();
            choose(0);
        }
    }
    scanBooks();
    modal = 4;
    tell("Deleted " + name + ".");
}
std::u32string * App::activeField() {
    if (modal == 1 || modal == 6)
        return field == 0 ? &signTitle : &signAuthor;
    if (modal == 7)
        return &signTitle;
    return nullptr;
}
void App::openCover() {
    if (flipping || loadBlocked)
        return;
    coverDraft = doc.cover;
    signTitle = doc.title;
    previewDirty = true;
    modal = 7;
}
void App::openEditSigned() {
    signTitle = doc.title;
    signAuthor = doc.author;
    field = 0;
    modal = 6;
}
} // namespace bn
