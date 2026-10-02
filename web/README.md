# Browser build (planned)

This directory is a placeholder for a future Emscripten/WebAssembly frontend,
mirroring the `web/` frontends of the sibling emulators (1983-1986). The
Previous-derived core is largely platform-independent; a WASM port would
reuse the SDL3 API-compatible stub headers used by the siblings
(`web/compat/SDL3/SDL.h`) and a gamepad/on-screen-keyboard UI.

Not built by the default `make`. See `../ROADMAP.md` for the plan.