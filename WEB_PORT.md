# AngryMipts — Web Port Plan (SFML → Raylib + Emscripten)

**Goal:** Build AngryMipts as a WebAssembly game playable in any browser.
**Strategy:** Replace SFML with Raylib. Keep all physics, data, shared, and core layers untouched.
**Native build:** Must continue to work at all times via `cmake --build build`.

---

## Architecture Overview

```
src/
  core/          ← NO CHANGES
  shared/        ← NO CHANGES
  physics/       ← NO CHANGES (except Phase 4: physics_thread)
  data/          ← NO CHANGES (except Phase 5: HTTP client)
  platform/      ← NEW: abstraction layer
  render/        ← REWRITE using platform layer
  ui/            ← REWRITE using platform layer
  main.cpp       ← REWRITE using platform layer
```

---

## Phases

### Phase 1 — Platform types (NO functional changes)

Create `src/platform/platform.hpp` — a single header that:
- On native (`#ifndef __EMSCRIPTEN__`): re-exports SFML types as-is
- On web (`#ifdef __EMSCRIPTEN__`): defines equivalent Raylib-based types

**Types to abstract:**

| SFML type | Raylib equivalent | Used in |
|-----------|------------------|---------|
| `sf::Vector2f` | `Vector2` (raylib) | everywhere |
| `sf::Vector2u` | custom `Vec2u` struct | main, view_utils |
| `sf::Color` | `Color` (raylib) | everywhere |
| `sf::FloatRect` | `Rectangle` (raylib) | ui hit-testing |
| `sf::Clock` / `sf::Time` | `GetTime()` wrapper | all scenes |
| `sf::Font` | `Font` (raylib) | all scenes |
| `sf::Text` | custom `PlatText` struct | all scenes |
| `sf::RenderWindow` | Raylib window wrapper | main, scenes |
| `sf::Event` | custom `PlatEvent` union | all scenes |

**Deliverable:** `src/platform/platform.hpp` compiles on both native and web.

---

### Phase 2 — Raylib CMake integration

**2a.** Add Raylib as FetchContent in `CMakeLists.txt` (only fetched when `EMSCRIPTEN`):

```cmake
if(EMSCRIPTEN)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()
```

**2b.** Create `build_web.sh`:

```bash
#!/bin/bash
source ~/Programs/emsdk/emsdk_env.sh
mkdir -p build_web
cd build_web
emcmake cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-emscripten.cmake
emmake make -j4
```

**2c.** Emscripten link flags (in CMakeLists.txt under `if(EMSCRIPTEN)`):

```cmake
target_link_options(${PROJECT_NAME} PRIVATE
    -sUSE_GLFW=3
    -sALLOW_MEMORY_GROWTH=1
    -sMAX_WEBGL_VERSION=2
    -sEXPORTED_RUNTIME_METHODS=cwrap
    --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets
    --preload-file ${CMAKE_SOURCE_DIR}/levels@/levels
    -sASSERTIONS=1
)
```

**Deliverable:** `build_web.sh` runs without errors (even if nothing renders yet).

---

### Phase 3 — Rewrite render/ layer

Files: `renderer.cpp/hpp`, `texture_manager.cpp/hpp`, `particles.cpp/hpp`, `sfx_player.cpp/hpp`, `view_utils.cpp/hpp`

**3a. TextureManager**
- SFML: generates textures via `sf::RenderTexture` + `sf::Image::setPixel` + saves to PNG
- Raylib: generate via `Image` + `ImageDrawPixel` + `LoadTextureFromImage`
- Strategy: create `texture_manager_sfml.cpp` and `texture_manager_raylib.cpp`, select in CMake

**3b. Renderer**
- SFML: `sf::RectangleShape`, `sf::CircleShape`, `sf::ConvexShape`, `sf::Sprite`
- Raylib: `DrawRectanglePro`, `DrawCircle`, `DrawPoly`, `DrawTextureEx`
- Shaders (bloom): Raylib has `LoadShader` + `BeginShaderMode` — direct equivalent
- Strategy: rewrite `renderer.cpp` with `#ifdef __EMSCRIPTEN__` blocks per draw call

**3c. Particles**
- SFML: `sf::ConvexShape` per particle
- Raylib: `DrawPoly` — direct equivalent
- Strategy: add `#ifdef` in `particles.cpp`

**3d. SfxPlayer**
- SFML: `sf::SoundBuffer::loadFromSamples` + `sf::Sound`
- Raylib: `LoadSoundFromWave` + `PlaySound` — equivalent API
- Strategy: rewrite `sfx_player.cpp` with `#ifdef`

**3e. ViewUtils**
- SFML: `sf::View::setViewport` for letterboxing
- Raylib: `BeginMode2D` + custom camera with offset — slightly different concept
- Strategy: rewrite `view_utils.cpp` fully with `#ifdef`

---

### Phase 4 — Rewrite ui/ layer + main.cpp

This is the largest phase. Every scene uses `sf::Font`, `sf::Text`, `sf::Event`.

**4a. Event system**
- SFML 3: `event.getIf<sf::Event::KeyPressed>()` (type-safe variant)
- Raylib: `IsKeyPressed(KEY_*)`, `IsMouseButtonPressed(MOUSE_LEFT_BUTTON)`, `GetMousePosition()`
- Strategy: create `src/platform/event.hpp` with a `PlatEvent` struct that wraps both

```cpp
// src/platform/event.hpp
#ifdef __EMSCRIPTEN__
struct PlatKeyEvent   { int key; bool alt; };
struct PlatMouseBtn   { float x, y; int button; };
struct PlatMouseMove  { float x, y; };
struct PlatMouseWheel { float delta; };
struct PlatTextInput  { uint32_t unicode; };
struct PlatResized    { unsigned w, h; };
using PlatEvent = std::variant<
    PlatKeyEvent, PlatMouseBtn, PlatMouseMove,
    PlatMouseWheel, PlatTextInput, PlatResized,
    std::monostate /*Closed*/, std::monostate /*FocusGained*/
>;
std::vector<PlatEvent> poll_events();  // polls Raylib input
#else
using PlatEvent = sf::Event;           // native: passthrough
#endif
```

**4b. Scene base class**
Change `scene.hpp`:
```cpp
// Before:
virtual SceneId handle_input(const sf::Event& event) = 0;
virtual void render(sf::RenderWindow& window) = 0;

// After:
virtual SceneId handle_input(const PlatEvent& event) = 0;
virtual void render(PlatWindow& window) = 0;
```

**4c. Text rendering in scenes**
- SFML: `sf::Text t(font, "str", size); t.setFillColor(...); window.draw(t);`
- Raylib: `DrawTextEx(font, "str", pos, size, spacing, color);`
- Strategy: helper `draw_text(window, font, str, x, y, size, color)` that wraps both

**4d. Scenes to rewrite (in order of complexity):**
1. `login_scene` — text input, buttons (moderate)
2. `menu_scene` — simple, few elements (easy)
3. `result_scene` — two panels, leaderboard (moderate)
4. `level_select_scene` — two panels, async preview (moderate)
5. `game_scene` — most complex: shaders, slingshot, HUD (hard)

**4e. main.cpp**
- Replace `sf::RenderWindow` with Raylib `InitWindow` / `CloseWindow`
- Replace event loop with `emscripten_set_main_loop` on web, regular `while` on native
- Fullscreen: `ToggleFullscreen()` in Raylib

---

### Phase 5 — Replace HTTP client (cpr → emscripten_fetch)

**Affected files:** `auth_client.cpp`, `OnlineScoreClient.cpp`

**Strategy:** Create `src/platform/http.hpp` interface:

```cpp
struct HttpResponse { int status; std::string body; };
using HttpCallback = std::function<void(HttpResponse)>;

// Async on web, sync-wrapped on native
void http_post(const std::string& url,
               const std::string& json_body,
               const std::string& token,
               HttpCallback cb);

void http_get(const std::string& url,
              HttpCallback cb);
```

- Native implementation (`http_cpr.cpp`): wraps `cpr::Post` / `cpr::Get`, calls callback immediately
- Web implementation (`http_fetch.cpp`): uses `emscripten_fetch` with `EMSCRIPTEN_FETCH_LOAD_TO_MEMORY`

**Impact:** AuthClient and OnlineScoreClient become callback-based instead of synchronous.
This also means leaderboard threads go away — callbacks replace them.

---

### Phase 6 — Replace physics thread

**File:** `physics/physics_thread.cpp`

On web, `std::thread` requires SharedArrayBuffer + COOP/COEP headers — complex to set up.

**Strategy:** Add `PhysicsMode::Inline` to `PhysicsRuntime` (already has `Single` mode):
- On web: run physics inline in the main loop frame, no separate thread
- On native: keep threaded mode

```cpp
#ifdef __EMSCRIPTEN__
    runtime_.set_mode(PhysicsMode::Single);  // inline, no thread
#else
    runtime_.set_mode(PhysicsMode::Threaded);
#endif
```

`PhysicsRuntime` already supports `PhysicsMode::Single` — this may require zero changes to physics code.

---

### Phase 7 — Asset paths for web

On web, assets are embedded via `--preload-file`. Emscripten mounts them at the virtual FS path.

In `main.cpp`, add:
```cpp
#ifdef __EMSCRIPTEN__
    const std::string fontPath = "/assets/fonts/liberation_sans.ttf";
    const std::string levelsPath = "/levels";
    const std::string sessionPath = "/session.json";
#else
    // existing resolveProjectPath logic
#endif
```

`TextureManager::generate_all()` calls `image.saveToFile()` — on web, skip the save (textures are generated in memory only, no disk write needed).

---

### Phase 8 — Web shell + deployment

**8a.** Create `web/shell.html` — custom HTML page:
- Canvas centered on page
- Title, description
- Fullscreen button
- Loading spinner

**8b.** Deploy to GitHub Pages:
- Build produces `build_web/AngryMipts.js` + `AngryMipts.wasm` + `AngryMipts.data`
- Copy to `docs/` folder (GitHub Pages source)
- Push to GitHub

**8c.** GitHub Actions CI (optional):
- Auto-build on push to `main`
- Deploy to GitHub Pages automatically

---

## Recommended Work Order

| Week | Phase | Goal |
|------|-------|------|
| 1 | 1 + 2 | Platform types compile, build_web.sh runs |
| 2 | 3 | render/ layer works in both builds |
| 3 | 4a–4d | Scenes compile with Raylib |
| 4 | 4e + 5 | main loop + HTTP async |
| 5 | 6 + 7 | Physics inline + asset paths |
| 6 | 8 | Web shell, deploy, test in browser |

---

## Key Risks

| Risk | Mitigation |
|------|-----------|
| Raylib font rendering differs from SFML | Test early with `DrawTextEx`, adjust spacing |
| Shader (bloom) GLSL syntax differences | Raylib uses GLSL 100 on web vs 330 on desktop — may need two shader versions |
| emscripten_fetch CORS | Backend at `84.201.138.107:8080` must have CORS headers for browser requests |
| Texture generation on web | Remove `saveToFile()` calls, keep textures in-memory only |
| `std::filesystem` on web | Replace with hardcoded paths under `#ifdef __EMSCRIPTEN__` |

---

## CORS — Backend Must Be Fixed

**This is required for login/leaderboard to work in browser.**

The backend server must respond with:
```
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, POST, OPTIONS
Access-Control-Allow-Headers: Content-Type, Authorization
```

Without this, the browser will block all HTTP requests from the game.
Coordinate with the backend team (Participant A or C) before Phase 5.

---

## Testing Checkpoints

After each phase, verify:
- [ ] `cmake --build build` succeeds (native)
- [ ] `./build/AngryMipts` runs and looks correct
- [ ] After Phase 2+: `bash build_web.sh` compiles without errors
- [ ] After Phase 8: game loads in Chrome/Firefox, login works, leaderboard works
