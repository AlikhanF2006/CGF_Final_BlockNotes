# Module: Renderer, shaders and shadow mapping (headline technique)

**Owner:** Alexander Tarulin

## Files

| File | Contents |
|---|---|
| `shaders/scene.vert`, `shaders/scene.frag` | Main lighting pass: Blinn-Phong, shadow lookup with PCF, two-sided paper |
| `shaders/depth.vert`, `shaders/depth.frag` | Shadow pass: depth only, from the light |
| `shaders/ui.vert`, `shaders/ui.frag` | 2D interface: coloured / textured rectangles and text |
| `src/graphics.hpp`, `src/graphics.cpp` | Shader loading with error log, meshes (unit cube, plane), shadow framebuffer, render passes, back-face culling, wood and leather textures, UI drawing, screenshots |
| `src/dialogs.cpp` | Dialogs: sign / edit signature, cover editor with live preview, load list, delete book, delete page, new notebook |

## Rendering pipeline (one frame)

```
 scene() → list of Draw records
      │
      ├─ Pass 1: SHADOW MAP  (FBO, 2048×2048 depth texture, depth.vert/frag)
      │     lightMatrix = ortho(−7,7,−7,7, 1,22) · lookAt(lightPos, origin, +Y)
      │     draw every object with casts = true → depth from the light
      │
      ├─ Pass 2: COLOUR  (default framebuffer, scene.vert/frag)
      │     per object: model, tint, textures; culling ON for cubes, OFF for paper sheets
      │     vertex: world position, normal = transpose(inverse(model))·n, shadowPosition = lightMatrix·world
      │     fragment: Blinn-Phong + shadow factor (PCF 3×3)
      │
      └─ Pass 3: UI  (ui.vert/frag, orthographic 1440×900, depth test off)
```

## Lighting (scene.frag)

```
l = normalize(lightPos − p),  v = normalize(eye − p),  h = normalize(l + v)
diffuse  = max(n·l, 0)
specular = max(n·h, 0)^32 · 0.10
color    = base · (0.54 + (1 − shadow) · 0.52 · diffuse) + (1 − shadow) · specular
```

0.54 is the ambient term. With F2 (lighting off) the base colour is used directly.

## Shadow mapping with PCF (headline)

1. In pass 1 the scene is rendered from the light into a 2048×2048 depth texture.
2. In pass 2 each fragment is transformed into the light's clip space (`shadowPosition`),
   then to texture space: `p = shadowPosition.xyz / w · 0.5 + 0.5`.
3. A fragment is in shadow when its depth is greater than the stored depth:
   `p.z − bias > shadowMap(p.xy)`.
4. **Bias** against shadow acne depends on the surface slope:
   `bias = max(0.002 · (1 − n·l), 0.0005)`.
5. **PCF**: the test is done for the 3×3 neighbouring texels and averaged
   (`shadow = Σ / 9`), which softens the jagged edge.
6. Fragments outside the light's frustum are treated as lit.

**F3** toggles shadows (the headline toggle); F2 toggles lighting.

## Back-face culling
`glCullFace(GL_BACK)`, `glFrontFace(GL_CCW)`; enabled per draw for the unit cube only. All six
cube faces have counter-clockwise winding seen from outside, and no transform has a negative
scale. Paper planes and the curling sheet are drawn without culling because both sides are
visible; the fragment shader flips the normal on back faces (`!gl_FrontFacing`).

## Robustness
Shader compile/link errors are printed with the info log and stop the program with a message;
framebuffer completeness is checked after creating the shadow FBO.

## Demo checklist
- F3 on/off: shadows of the book, trees and pencil appear and disappear (headline toggle).
- Alt+←/→ with shadows on: the shadows rotate with the light.
- F2: flat colours without lighting.

## Likely defence questions
- *Why two passes?* — The depth from the light must exist before the colour pass can compare against it.
- *What is shadow acne and how do you fix it?* — Self-shadowing from limited depth precision; a slope-scaled bias.
- *What does PCF do?* — Averages several shadow tests around the sample to soften edges.
- *Why `transpose(inverse(model))` for normals?* — It keeps normals perpendicular under non-uniform scaling.
- *Why orthographic projection for the light?* — The light is far and acts almost like a directional light; ortho gives uniform shadow resolution over the desk.
