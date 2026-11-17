# Changes (Week 7 fixes)

1. **Delete books** - Load book now shows each book's title, and a red **X** deletes it
   (with confirmation). Its `.bak` backup is removed as well. If the open book is deleted,
   the app switches to another saved book or starts a blank one.
2. **Signature and Read/Edit mode are separate** - signing only puts the title and
   "by Author" on the cover; you can keep writing. **Edit signature** changes or removes it.
   A **Make read-only / Allow editing** button (top right, F6) switches the mode at any time;
   the top bar shows e.g. `SIGNED / EDIT` or `SIGNED / READ ONLY`. The mode is saved as
   `readOnly` in the JSON; older signed books open in Read mode.
3. **Performance** - typing or pressing a button no longer repaints all 4 page textures
   and the cover. Glyphs and the paper background are cached, only changed pages are
   repainted, textures are updated in place, typing is grouped into undo steps.
   Release build: ~34 ms -> ~1.8 ms per keystroke repaint. `CMakePresets.json` adds an
   **x64-Release** configuration for Visual Studio (Debug builds are much slower).
4. **Cover customization** - new **Cover** button (or click the title in the left panel):
   title, 8 leather colours, 4 frames, 6 pixel emblems, gold/silver/copper lettering,
   optional BLOCKNOTES line, live preview. Title is centred, word-wrapped and auto-sized;
   "by Author" is centred under it. Saved as an optional `cover` object in the JSON
   (old notebooks still load).
5. **Fonts, sizes and text styling** - every character has its own ink colour and font:
   Classic (DejaVu Sans Mono), Pixel (Press Start 2P) or Handwritten (Victor Mono Italic).
   Size S (41x27) / M (35x23) / L (27x18) is per page. Styles are stored as `runs`
   (`[count, ink, font]`) in each page of the JSON; older files still load.
7. **Text selection** - mouse drag, Shift+click, Shift+arrows, Ctrl+A; copy / cut / paste,
   delete or type over the selection; colour and font apply to the selection or to new text.
6. **Word wrap** - words move to the next line whole; a full line followed by Enter
   no longer leaves an empty line.
8. **Course requirements** - back-face culling for solid cubes (paper sheets stay two-sided);
   movable light (Alt+Left/Right or the arrows in SCENE, compass shows its direction);
   F7 / Pause button freezes the animation clock (particles, page turn, caret) while
   editing keeps working; team names, group and headline technique on screen and in the
   window title.
9. **Module structure** - the application is split into files by team module
   (`app.hpp`/`main.cpp`, `camera_scene.cpp`, `writing_desk.cpp`, `page_curl.cpp`, `dialogs.cpp`,
   `smoke_test.cpp`); behaviour is unchanged. Module descriptions are in `docs/modules/`.
