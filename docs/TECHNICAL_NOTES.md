# Technical notes

## Rendering pipeline

1. GLFW collects window, character, key, mouse, scroll and file-drop events.
2. The document model applies valid mutations. Read-only books reject edits.
3. Changed notes are rasterized into page textures. Font glyphs come from a
   bundled TrueType font; the paper pattern is generated deterministically.
4. A light-space depth pass renders shadow-casting geometry into a framebuffer.
5. The colour pass draws textured meshes with the scene shader.
6. A separate orthographic pass draws the interface and font atlas with alpha blending.
7. GLFW swaps buffers. The loop also services autosave and the page-turn timer.

All 3D objects use model, view and perspective-projection matrices. Surface normals
use the inverse-transpose model matrix, so non-uniformly scaled cubes retain
correct lighting. The interface has its own projection in a 1440x900 logical coordinate space.

## Curved page

The animated sheet has 64 subdivisions across its width and 12 along its height.
For normalized horizontal coordinate u and eased animation progress t:

```text
theta = pi*t + sin(pi*t)*sin(0.7*pi*u)*0.48
x = cos(theta)*u*pageWidth
y = (v - 0.5)*pageHeight
z = pageHeightOffset + sin(theta)*u*pageWidth
```

The animation progress uses smoothstep, `t*t*(3-2*t)`, and lasts 0.85 seconds.
Normals are derived from a finite-difference horizontal tangent crossed with the
vertical tangent. A backward turn mirrors the geometry, reverses winding and
adjusts UV coordinates. Different textures on the front and back keep the
outgoing and incoming page text attached to the turning sheet.

At the end of a turn the active spread changes, and the sheet returns to the static
page representation. The result is a controlled visual approximation of paper,
not a physical elasticity model.

## Lighting and shadows

The fragment shader combines a constant ambient term, Lambertian diffuse light
and a Blinn-Phong specular term. A shadow framebuffer stores light-space depth.
The colour pass compares projected fragment depth to the depth map, using
a slope-dependent bias and nine samples for percentage-closer filtering.

Turning paper is rendered on both sides. Back-facing normals are corrected in
the fragment shader, and a different texture is sampled for the reverse side.
Small animated cube particles have an additional emissive contribution.

## Picking and editing

Mouse coordinates are converted to normalized device coordinates. The inverse
view-projection matrix produces a ray, which intersects the flat paper plane.
The intersection selects the left or right page and its text row/column.
Picking is disabled during page turns, when the paper is no longer flat.

The document stores UTF-32 internally so Backspace removes a code point rather
than a byte. JSON stores UTF-8. English is the language of the interface and sample
content. The bundled font also supports Cyrillic input, but complex shaping and
emoji rendering are outside this beta's scope.

## Persistence

The JSON schema contains format/version, title, author, signed and readOnly flags,
an optional cover style and an array of page objects with text, size and style runs
(`[count, ink, font]` per run of equally styled characters). Loading validates the whole candidate
document before replacing the current one. Oversized, malformed or unsupported
files are rejected. A save writes a temporary file before replacing the main file
and retains the previous save as a backup. This is single-user local persistence;
concurrent editing by several processes is not coordinated.

## Suggested team walkthrough

These areas follow the planned module responsibilities. Each team member should be ready to explain and demonstrate the listed parts of the code.

| Team member | Code to study and demonstrate |
|---|---|
| Alikhan Faizrakhman | Scene/depth shaders, shadow framebuffer, generated textures |
| Alikhan Kenzhebek | Page deformation, easing and front/back UV mapping |
| Alexandr Tarulin | Mesh construction, model/view/projection matrices and scene placement |
| Farkhad Imanbayev | GLFW callbacks, text layout, interface, document model and JSON |
| Everyone | Build, test, demonstration and attribution |
