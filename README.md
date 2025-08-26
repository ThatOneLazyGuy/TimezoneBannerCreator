# TimezoneBannerCreator

A simple tool to create an image banner with automatically converted times.
Adding time zones will automatically convert all of them properly according to the format specified.
Extra images and text can also be added to the banner.

# Libraries:
- SDL3: for managing the window, rendering, and miscellaneous uses.
- Dear ImGui: for UI.
- ImSearch: a better search add-on for Dear ImGui.
- Portable File Dialogs: for file dialogs that block the current thread (SDL3 only supports non-blocking dialogs).
- stb_image, stb_image_resize2, stb_image_write: self-explanatory.
- stb_truetype: for text rendering to textures.
