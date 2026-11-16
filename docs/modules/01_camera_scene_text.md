# Module: Camera, scene, mouse input and page text

**Owner:** Alikhan Faizrakhman

## Files

| File | Contents |
|---|---|
| `src/camera_scene.cpp` | Light position and animation clock, orbit camera, scene composition, particles, pause, light rotation, mouse picking on the page, cursor movement, text selection and clipboard helpers, text styling |
| `src/writing_desk.cpp` | Right "Writing desk" panel: ink colour, font, S/M/L size, scene toggles, pause, light compass, camera reset |
| `src/font.hpp`, `src/font.cpp` | Glyph rasterisation and cache, text measuring, procedural paper and cover textures, colour/font tables |
| `assets/` | Page fonts (DejaVu Sans Mono, Press Start 2P, Victor Mono Italic) and their licences |

## How it works

### Transformations and camera (`App::updateCamera`)
Every object is drawn with a model matrix built by `transform(position, scale)` (translation
times scale). The camera orbits the origin on a sphere:

```
eye = ( sin(yaw)·d,  sin(pitch)·d,  cos(yaw)·cos(pitch)·d )
VP  = perspective(43°, aspect, 0.1, 40) · lookAt(eye, origin, up = +Y)
```

`aspect` is recomputed every frame from the framebuffer size, so resizing the window never
stretches the image. Right-drag changes `yaw`/`pitch` (clamped), the mouse wheel changes `d`
(clamped to 7.4–11).

### Scene composition (`App::scene`)
The frame is a list of `Draw` records (mesh, model matrix, texture, tint, emission, casts shadow).
Order: desk surface → desk props → book (`addDeskProps`, `addBook` from the page module) →
particles. The renderer then draws the list twice (shadow pass, colour pass).

### Particles
25 small emissive cubes on elliptical orbits:
`angle = 0.15·t + i·2.399` (golden-angle spacing),
`p = (cos(angle)·r, sin(angle)·2.6, 0.8 + 0.45·sin(0.7·t + i))`.
They use the **animation clock** `animNow()`, not the wall clock.

### Pause (F7) and the animation clock
`animNow() = (paused ? pausedAt : now) − animOffset`. Pausing freezes `pausedAt`; resuming adds
the paused duration to `animOffset`, so particles, the page turn and the caret continue exactly
where they stopped. Saving, status messages and FPS keep using real time.

### Movable light
`lightPos = (cos(a)·5, sin(a)·5, 7)`; `a` is changed by Alt+←/→ or the `<` `>` buttons. The
compass in the right panel draws the light's azimuth from the top.

### Mouse picking on the page (`App::hitPaper`)
1. Convert the mouse position to normalised device coordinates.
2. Un-project the near and far points with `inverse(VP)` and divide by `w` → a ray in world space.
3. Intersect the ray with the paper plane `z = 0.10`.
4. Convert the hit point to page UV, then to a character cell of the page grid, and pick the
   nearest character position from the word-wrapped layout.

### Selection and styling
The selection is `[min(anchor, cursor), max(anchor, cursor))`. Dragging updates `cursor`
while `anchor` stays; Shift+arrows do the same with the keyboard. Colour or font buttons call
`applyStyle`, which restyles the selected characters or, with no selection, sets the "pen" for
newly typed text.

### Text rendering and procedural textures (`font.cpp`)
Glyphs are rasterised with stb_truetype once per (character, size) and cached; painting a page
is then a fast alpha blend into a 640×900 RGBA canvas uploaded as an OpenGL texture. Only pages
whose content, caret or selection changed are repainted. Paper (grain, edge shading, ruled lines)
and the cover (leather grain, frame, pixel emblem, centred auto-sized title) are generated in code.

## Demo checklist
- Right-drag and scroll: orbit and zoom; resize the window.
- Alt+←/→: the light moves, shadows follow, the compass dot moves.
- F7: particles freeze, "PAUSED" appears; typing still works.
- Drag across text, change colour/font, Ctrl+C / Ctrl+V.

## Likely defence questions
- *Why is the aspect ratio recomputed every frame?* — The framebuffer can be resized at any time; a stale aspect stretches the image.
- *How does a mouse click become a character position?* — Ray from `inverse(VP)`, plane intersection, UV → grid cell → nearest laid-out character.
- *What does pause stop and what does it not?* — It stops the animation clock only; input, saving and rendering continue.
- *Why cache glyphs?* — Rasterising every glyph on every keystroke made typing lag; the cache makes repainting about 19× faster.
