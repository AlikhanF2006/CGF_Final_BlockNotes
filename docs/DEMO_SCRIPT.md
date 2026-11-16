# Live demo guide — approximately 4 minutes

## 0:00–0:30 — Idea and scene

“Our project is BlockNotes, an interactive 3D notebook built with C++ and OpenGL.
It takes inspiration from the writable book in Minecraft. Users can write notes,
turn pages, save their work and sign the finished book.”

Show the sample book. Right-drag briefly, then reset the view.

## 0:30–1:20 — Editing

“Text is rendered directly onto the page texture. Clicking the paper places the
cursor. The application supports editing, undo, page creation and different ink colours.”

Add a page. Type “Final demonstration”. Add a second line, change the ink colour,
erase a word and undo. Explain that the ink colour applies to the entire page.

## 1:20–2:00 — Page animation

“The page is a subdivided mesh. During a turn, we update its vertex positions and
normals. The front and back use different textures so the text moves with the paper.”

Ensure at least three pages exist. Use Next and Previous. Orbit slightly to show
the sheet's depth, then restore the view.

## 2:00–2:45 — Graphics features

“Our scene uses a model-view-projection pipeline. Lighting combines ambient,
diffuse and specular components. Shadows use a separate depth framebuffer.
Paper, leather and wood textures are generated procedurally.”

Toggle Light, Shadows and Particles one at a time. Restore all three.

## 2:45–3:20 — Persistence

“Notes are stored locally in JSON. We have manual saving, autosaving, a backup
file and validation when loading a book.”

Save the demonstration book. Use Load book to reopen it and show that the text remains.

## 3:20–4:00 — Signing and conclusion

“Signing puts the title and author on the cover. Read mode, switched at the top
right or with F6, locks the pages; the cover can be customized at any time.”

Sign a disposable book, press Make read-only and try typing to show the lock.
Open Cover, change the leather colour and emblem, then click Done to show the cover.
Mention that the page curl is a geometric animation, not a paper-physics simulation.
