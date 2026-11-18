# Module: Window, input and interface

**Owner:** Farkhad Imanbayev

## Files

| File | Contents |
|---|---|
| `src/app.hpp` | `struct App`: application state, keyboard input, typing / deleting, undo / redo, saving and autosave, Read/Edit mode, page texture refresh, left panel, header, bottom bar, help screen, frame loop |
| `src/main.cpp` | Entry point: command-line options (`--file`, `--demo`, `--smoke`), GLFW window and OpenGL 3.3 core context, GLAD loading, input callbacks, main loop, saving on close |
| `CMakeLists.txt`, `CMakePresets.json`, `cmake/mingw.cmake` | Build: executable, unit tests, copying assets/shaders, Visual Studio presets, MinGW cross-compilation |
| `START_BLOCKNOTES.bat` | One-click launch of the prebuilt executable with the sample book (`--demo`) |
| `windows/` | Prebuilt 64-bit executable with assets and shaders |
| `third_party/` | External libraries (GLFW, GLAD, GLM, nlohmann/json, stb) — not team code |

## How it works

### Window and context
GLFW creates a resizable 1440×900 window with an OpenGL 3.3 core-profile context; GLAD loads
the GL functions. V-sync is on. Callbacks (keys, characters, mouse buttons, cursor, scroll,
file drop, window close) forward events to the `App` object stored as the window user pointer.

### Frame loop (`App::frame`)
1. Finish a page turn if its time is over; autosave 1.5 s after the last edit.
2. Repaint page textures that changed (caret blink, typing).
3. Update camera and light, build the scene, render it.
4. Count FPS, draw the interface.

### Keyboard input and editing
Character events insert text at the cursor with the current pen style; Enter, Backspace,
Delete, arrows, Home/End and shortcuts (Ctrl+A/C/X/V/Z/Y/S/N, F1–F7, PageUp/PageDown, Esc)
are handled in `App::key`. Fast typing on one page is grouped into one undo step (up to 3 s
or until a 1 s pause), so the whole book is not copied on every key. Undo/redo keep up to 100
steps and never change the Read/Edit mode.

### Interface
Immediate-mode style: every frame the interface is drawn again and buttons register a rectangle
and an action; a click runs the action of the button under the mouse. Header (project, group,
team, Read/Edit switch), left panel (notebook title, page list, new/delete page), bottom bar
(Save, Undo, Sign, Done, New book, Load book, Cover), status line and F1 help.

### Saving
Books are saved as JSON in the user's application-data folder (see README). Saving happens on
Ctrl+S, 1.5 s after the last edit, before loading another book and when the window closes; if
saving fails, the window stays open so nothing is lost.

## Demo checklist
- Run `START_BLOCKNOTES.bat`, or build in Visual Studio (x64-Release) and run.
- F1 help; type, undo/redo, save; close and reopen — the book is restored.
- F6 / top-right button: Read mode blocks typing.

## Likely defence questions
- *Why OpenGL 3.3 core profile?* — Required by the course; no fixed-function pipeline, everything goes through our shaders.
- *How do GLFW callbacks reach the C++ object?* — The `App` pointer is stored with `glfwSetWindowUserPointer` and read back in each callback.
- *How is lag avoided while typing?* — Grouped undo steps, glyph cache and repainting only changed pages.
- *What happens if the program is closed with unsaved changes?* — The close callback saves first and cancels closing if saving fails.
