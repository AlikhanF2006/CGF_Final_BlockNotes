# Module: Pages, page turn and book model

**Owner:** Alikhan Kenzhebek

## Files

| File | Contents |
|---|---|
| `src/page_curl.cpp` | Page-turn state machine (`flip`, `updateFlip`), curved page mesh (`Graphics::updateCurl`), hierarchical book model (`addBook`) and desk props (`addDeskProps`) |
| `src/document.hpp`, `src/document.cpp` | Notebook data model: text editing, word-wrap layout and S/M/L grids, per-character styles, signing, Read/Edit mode, cover style, JSON save/load with backup, deleting books |
| `tests/document_tests.cpp` | Unit tests for the document model |
| `src/smoke_test.cpp` | Automated end-to-end test (`--smoke DIR`): screenshots of every feature, simulated mouse drag, restyle, delete and undo |

## How it works

### Page-turn state machine
`flip(dir)` starts a turn: it stores the target spread, the direction and the start time on the
**animation clock**. Each frame `updateFlip` checks whether 0.85 s have passed and then shows
the new spread. While turning, the renderer shows the outgoing page on the curling sheet and the
incoming page on its back. If the animation is paused, the spread changes immediately.

### Animation curve
Linear progress `t = (time − flipStart) / 0.85` is eased with **smoothstep**:

```
t' = t² · (3 − 2t)
```

It starts and ends with zero velocity, so the sheet lifts and lands softly.

### Curved page mesh (`Graphics::updateCurl`)
A 64 × 12 grid of vertices over the page (u across, v along the spine). Each column is rotated
around the spine by an angle that grows with `t` and bends the free edge further than the spine:

```
θ(u) = π·t + sin(π·t) · sin(0.7·π·u) · 0.48
p(u, v) = ( cos θ · u·W,  (v − 0.5)·H,  0.10 + sin θ · u·W )
```

Normals come from the tangent `∂p/∂u` (finite difference) crossed with the spine axis, so
lighting and shadows work on the bent sheet. For a backward turn x is mirrored and the
triangle winding flipped. The sheet is two-sided: front and back textures, no culling.

### Hierarchical book model
The open book is built from transformed unit cubes and planes: two leather boards, a spine,
a five-layer page stack per side, two page planes, rings on the spine, and the curling sheet.
The closed book is a body, a page block and a cover with the generated cover texture. The
pencil uses a parent transform (translate) multiplied by a rotation and a scale.

### Text layout (`layout`, `gridFor`)
Pages are character grids: S 41×27, M 35×23, L 27×18. Layout is word-wrapped: a word that
does not fit moves to the next line; only words longer than a line are split. A space at the
end of a full line hangs in the margin, so a full line followed by Enter does not create an
empty line.

### Styles and JSON format
Each character has `{ink, font}`. In JSON they are stored as runs `[count, ink, font]`.
Saving writes a temporary file, keeps the previous version as `.bak`, then renames. Loading
validates every field and refuses to replace the open book with a corrupt file.

## Demo checklist
- Next / Previous: the page curls forward and backward; F7 during a turn freezes the sheet mid-air.
- Type a long word at the end of a line: it moves to the next line.
- Run `ctest` (unit tests) and `BlockNotes --smoke out` (end-to-end test).

## Likely defence questions
- *Why smoothstep and not linear time?* — Linear motion starts and stops abruptly; smoothstep has zero velocity at both ends.
- *How are normals of the curved sheet computed?* — Finite-difference tangent along u, crossed with the spine direction.
- *Why is the page sheet drawn without back-face culling?* — Both sides of the sheet are visible during the turn.
- *What happens if the JSON file is damaged?* — Validation fails, the open book is kept, the error is shown; the `.bak` file still holds the previous save.
