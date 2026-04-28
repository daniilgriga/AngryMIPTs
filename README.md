# AngryMipts

<div align="center">

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue?logo=cplusplus)
![SFML 3](https://img.shields.io/badge/SFML-3.0-green?logo=sfml)
![Box2D 3.0](https://img.shields.io/badge/Box2D-3.0.0-orange)
![CMake](https://img.shields.io/badge/CMake-3.20+-blue?logo=cmake)
![Multithreaded](https://img.shields.io/badge/threading-multithreaded-purple)
![Cross-Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Web-lightgrey)
[![License: BUSL-1.1](https://img.shields.io/badge/License-BUSL--1.1-blue.svg)](LICENSE)

</div>

![battlefield](img/img1.png)

---

## Overview

**AngryMipts** is a physics-based projectile game inspired by the Angry Birds genre, built from scratch in C++17 as a university course project. Players use a slingshot to launch a variety of projectiles at structures made from four distinct materials, aiming to defeat all targets within a limited number of shots to earn up to three stars.

The project is technically non-trivial: on native builds the physics simulation runs on a dedicated worker thread at a fixed 60 Hz timestep, decoupled from rendering via a double-buffered **WorldSnapshot** with atomic index swapping. Communication between threads is handled through two mutex-protected `ThreadSafeQueue<T>` channels — one for **Commands** (UI -> Physics) and one for **Events** (Physics -> UI) — keeping shared state narrow and explicitly synchronized.

The codebase targets two completely different runtimes — native desktop via **SFML 3** and browser via **Emscripten + Raylib 5.5** — through a unified platform abstraction layer (`platform/`) that provides a common API for windowing, rendering primitives, input, and HTTP. Online features (registration, login, score submission, per-level leaderboards) work on both platforms with JWT-based authentication; the web build uses non-blocking `emscripten_fetch`, while native UI code runs leaderboard synchronization on background `std::thread` workers.

---

## Features

### Gameplay

- **9 projectile types** with distinct physical behaviour; 7 of them expose mid-flight active abilities
- **4 destructible block materials**: Wood, Stone, Glass, Ice — each with distinct durability
- **3 block shapes**: Rect, Circle, Triangle (triangle geometry driven by exact vertex data from the snapshot)
- **10 hand-crafted levels** in JSON format; all 9 projectile types are implemented and used across the current level set
- **3-star scoring** per level based on score thresholds defined in level metadata
- **Slingshot drag mechanics** with pull-vector physics
- **Online leaderboard** per level with JWT-authenticated score submission

### Technical

- Native multithreaded physics at fixed 60 Hz; web uses the same runtime facade in single-thread mode
- Double-buffered `WorldSnapshot` — renderer reads a copied front buffer while physics publishes the next one
- `ThreadSafeQueue<Command>` (non-blocking `try_pop`) and `ThreadSafeQueue<Event>` draining for explicit inter-thread communication
- **Command pattern** for all game actions (launch, restart, load level, pause, activate ability)
- **Observer/Event variant** for physics outcomes (9 event types covering collisions, destruction, scoring, ammo bonuses, and level state)
- **Strategy pattern** for physics execution mode: `PhysicsMode::SingleThread` or `Threaded`
- **Snapshot pattern** with atomic double-buffering for safe cross-thread state transfer
- **Facade pattern** over `PhysicsRuntime` and `AccountService` to hide subsystem complexity
- Scene polymorphism via `Scene` abstract base class governing the shared scene lifecycle
- Cross-platform rendering abstraction covering SFML 3 (native) and Raylib 5.5 (web/Emscripten)
- Unified HTTP abstraction: CPR on native, `emscripten_fetch` on web
- `AuthClient` + `SessionManager` with persistent `session.json` token storage on native and browser `localStorage` on web
- Leaderboard fetching with four distinct status codes
- 6 GTest modules covering physics, auth, account, scores, sessions, and UI/render
- CI/CD via GitHub Actions: Linux/macOS builds, ccache, CTest, GitHub Pages deployment

---

## Screenshots

**Main / Login**

![Main/Login screen](img/login_screen.png)

**Level Select**

![Level select screen](img/level_choosing_screen.png)

**Mid-Gameplay**

![Mid-gameplay with in-flight projectile](img/flying_bird_screen-.png)

**Destruction Moment**

![Destruction moment with debris and score popup](img/boom_moment_screen.png)

**Result Screen**

![Result screen with final score and stars](img/level_end_screen.png)

---

## Architecture

### Module Map

```
┌─────────────────────────────────────────────────────────────────────┐
│                         src/                                        │
│                                                                     │
│  ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────────────┐  │
│  │ shared/  │   │ physics/ │   │  core/   │   │     data/        │  │
│  │          │   │          │   │          │   │                  │  │
│  │ types    │◄──│ engine   │   │ score    │   │ LevelLoader      │  │
│  │ events   │   │ thread   │   │ system   │   │ AuthClient       │  │
│  │ commands │   │ runtime  │   │          │   │ SessionManager   │  │
│  │ snapshot │   │ units    │   │          │   │ OnlineScore      │  │
│  │ config   │   │          │   │          │   │   Client         │  │
│  └────┬─────┘   └────┬─────┘   └──────────┘   │ AccountService   │  │
│       │              │                        │ ScoreSaver       │  │
│       │         ┌────▼─────┐                  └──────────────────┘  │
│       │         │platform/ │                                        │
│       │         │          │                                        │
│       │         │ SFML     │                                        │
│       │         │ Raylib   │                                        │
│       │         │ HTTP     │                                        │
│       │         └────┬─────┘                                        │
│       │              │                                              │
│  ┌────▼──────────────▼──────────────────────────────────────────┐   │
│  │                         ui/                                  │   │
│  │                                                              │   │
│  │  SceneManager -> [LoginScene, MenuScene, LevelSelectScene,   │   │
│  │                   GameScene, ResultScene]                    │   │
│  │  Slingshot                                                   │   │
│  └──────────────────────────────────────────────────────────────┘   │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                       render/                               │    │
│  │  Renderer  TextureManager  ParticleSystem  SfxPlayer        │    │
│  └─────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
```

### Inter-Thread Data Flow

```
╔══════════════════════════╗          ╔══════════════════════════╗
║       MAIN THREAD        ║          ║     PHYSICS THREAD       ║
║  (UI · Events · Render)  ║          ║  worker_loop()  60 Hz    ║
╠══════════════════════════╣          ╠══════════════════════════╣
║                          ║          ║                          ║
║  SceneManager            ║          ║  PhysicsEngine           ║
║  Slingshot input         ║   push   ║  Box2D world step        ║
║  ──────────────────►  ThreadSafeQueue<Command>  ──────────►    ║
║                          ║ try_pop  ║  LaunchCmd               ║
║                          ║          ║  RestartCmd              ║
║                          ║          ║  LoadLevelCmd            ║
║                          ║          ║  PauseCmd                ║
║                          ║          ║  ActivateAbilityCmd      ║
║                          ║          ║                          ║
║  drain_events()  ◄──  ThreadSafeQueue<Event>  ◄──────────────  ║
║  process_events()        ║          ║  CollisionEvent          ║
║  ScoreSystem             ║          ║  DestroyedEvent          ║
║  UI state update         ║          ║  TargetHitEvent          ║
║                          ║          ║  ScoreChangedEvent       ║
║                          ║          ║  LevelCompletedEvent     ║
║                          ║          ║  ProjectileReadyEvent    ║
║                          ║          ║  AbilityActivatedEvent   ║
║                          ║          ║  ImpactResolvedEvent     ║
║                          ║          ║  AmmoBonusEvent          ║
║                          ║          ║                          ║
║  Renderer::draw()  ◄──────────── WorldSnapshot (double buffer) ║
║  reads front buffer      ║  atomic  ║  writes back buffer,     ║
║  under snapshot mutex    ║  swap    ║  then swaps index        ║
╚══════════════════════════╝          ╚══════════════════════════╝
```

### Design Patterns

|   Pattern   |                         Implementation                                      |                               Purpose                                    |
|-------------|-----------------------------------------------------------------------------|--------------------------------------------------------------------------|
| **Command** | `LaunchCmd`, `RestartCmd`, `LoadLevelCmd`, `PauseCmd`, `ActivateAbilityCmd` | Encapsulate game actions as data; safely cross thread boundary via queue |
| **Observer / Event Variant** | `CollisionEvent`, `DestroyedEvent`, `TargetHitEvent`, `ScoreChangedEvent`, `LevelCompletedEvent`, `ProjectileReadyEvent`, `AbilityActivatedEvent`, `ImpactResolvedEvent`, `AmmoBonusEvent` | Physics thread publishes events; main thread drains and reacts to them |
| **Snapshot** | `WorldSnapshot`, double-buffered with `std::atomic<int> front_snapshot_index_` | Provide renderer with a consistent, immutable world state without stalling the physics thread |
| **Strategy** | `PhysicsMode::SingleThread` vs `PhysicsMode::Threaded` | Allow physics to run on the main thread (debug/Web) or a dedicated worker (native) without changing call sites |
| **Facade** | `PhysicsRuntime`, `AccountService` | Simplify access to multi-class subsystems (physics engine + thread + queues; auth + session + online score) |
| **Scene Polymorphism** | `Scene` abstract base class -> `GameScene`, `MenuScene`, `LoginScene`, `LevelSelectScene`, `ResultScene` | Define common `handle_input`, `update`, and `render` lifecycle hooks enforced by the abstract base |

---

## Class Hierarchy

### Inheritance Graph

![inhgraph](img/class_diagram.png)

### Key Class Responsibilities

| Class | Module | Responsibility | Key Dependencies |
|---|---|---|---|
| `Scene` | `ui/` | Abstract base; defines scene lifecycle hooks | `SceneManager`, platform event types |
| `GameScene` | `ui/` | Active gameplay: slingshot input, HUD, physics dispatch | `Slingshot`, `PhysicsRuntime`, `ScoreSystem`, `Renderer` |
| `MenuScene` | `ui/` | Main menu navigation | `SceneManager` |
| `LoginScene` | `ui/` | Login / register form and validation | `AccountService` |
| `LevelSelectScene` | `ui/` | Level grid, star display, leaderboard panel | `LevelLoader`, `OnlineScoreClient`, `ScoreSystem` |
| `ResultScene` | `ui/` | End-of-level result, star animation | `ScoreSystem`, `SceneManager` |
| `SceneManager` | `ui/` | Owns and transitions between `Scene` instances by `SceneId` | All `Scene` subclasses |
| `PhysicsEngine` | `physics/` | Box2D world, body creation, step, event generation | Box2D 3.0.0 |
| `PhysicsThread` | `physics/` | Spawns worker thread, runs `worker_loop()` at 60 Hz | `PhysicsEngine`, `std::condition_variable stop_cv_` |
| `PhysicsRuntime` | `physics/` | Facade over engine + thread + queues + snapshot | `PhysicsEngine`, `PhysicsThread`, `ThreadSafeQueue<Command>`, `ThreadSafeQueue<Event>`, `WorldSnapshot` |
| `WorldSnapshot` | `shared/` | Immutable copy of world state for renderer; double-buffered | `std::array<WorldSnapshot, 2>`, `std::atomic<int>`, `std::mutex` |
| `ThreadSafeQueue<T>` | `shared/` | Thread-safe FIFO using mutex-protected `push()` and non-blocking `try_pop()` | `std::mutex`, `std::queue<T>` |
| `Renderer` | `render/` | Draws world from `WorldSnapshot`; manages views | `TextureManager`, `WorldSnapshot` |
| `TextureManager` | `render/` | Loads and caches platform textures | Platform texture type |
| `ParticleSystem` | `render/` | Spawns and simulates visual particle effects | Platform render target |
| `SfxPlayer` | `render/` | Plays sound effects keyed to game events | Platform audio |
| `LevelLoader` | `data/` | Parses level JSON files into level data structures | `nlohmann_json` |
| `AuthClient` | `data/` | HTTP calls to `/register` and `/login`; parses `AuthResult` | `platform/http.hpp` |
| `SessionManager` | `data/` | Saves/loads `session.json` with token and username | Filesystem |
| `OnlineScoreClient` | `data/` | Submits scores (POST `/scores`) and fetches leaderboards (GET `/leaderboard?levelId=...`) | `platform/http.hpp`, JWT token |
| `AccountService` | `data/` | Facade: session loading, login, logout, coordinating auth + session + online score | `AuthClient`, `SessionManager`, `OnlineScoreClient` |
| `ScoreSaver` | `data/` | Persists local best scores | Filesystem |
| `ScoreSystem` | `core/` | Tracks current run score, computes star rating | `LevelMeta` thresholds |
| `Slingshot` | `ui/` | Captures drag input, computes pull vector, emits `LaunchCmd` | Platform input events |
| `Logger` | `data/` | Structured logging with lightweight format substitution | — |

---

## Multithreading

### Thread Architecture

```
 ┌─────────────────────────────────────────────────────────────────┐
 │ OS Process                                                      │
 │                                                                 │
 │  Thread 0: MAIN                    Thread 1: PHYSICS WORKER     │
 │  ─────────────────                 ───────────────────────────  │
 │  Window event loop                 worker_loop()                │
 │  Scene::update(dt)                 Fixed timestep: 1/60 s       │
 │  Renderer::draw()                  PhysicsEngine::step()        │
 │  Input handling                    Box2D world integration      │
 │  HTTP callbacks                    Collision detection          │
 │  drain_events()  ◄───────────────► push_event()                 │
 │  push_command()  ───────────────►  try_pop()                    │
 │                   ThreadSafeQueue  snapshot swap (atomic)       │
 │                                                                 │
 │  Synchronisation primitives used:                               │
 │  • std::mutex (snapshot_mutex_, queue internal)                 │
 │  • std::atomic<int> front_snapshot_index_                       │
 │  • std::condition_variable stop_cv_  (graceful shutdown)        │
 └─────────────────────────────────────────────────────────────────┘
```

### Synchronisation Details

**Command queue (Main -> Physics)**

`ThreadSafeQueue<Command>` uses an internal `std::mutex` and `std::queue<T>`. The main thread calls `push()` (blocking only for the duration of the lock), and the physics thread calls `try_pop()` at the start of each tick — a non-blocking poll that returns immediately if the queue is empty. This means the physics loop never waits on the UI thread.

**Event queue (Physics -> Main)**

The physics thread calls `push()` for each event generated during a step. After its own frame processing, the main thread drains events with repeated non-blocking `try_pop()` calls and dispatches them outside the queue implementation.

**WorldSnapshot double-buffer**

```
 back buffer (index 1-front)          front buffer (index front)
 ───────────────────────────          ──────────────────────────
 Physics thread writes here           Renderer copies from here
 during each step                     under snapshot_mutex_

 After writing:
   snapshot_mutex_.lock()
   front_snapshot_index_.store(back)   <- atomic publish
   snapshot_mutex_.unlock()

 Renderer:
   snapshot_mutex_.lock()
   idx = front_snapshot_index_.load(std::memory_order_acquire)
   WorldSnapshot snap = snapshots_[idx]
   snapshot_mutex_.unlock()
```

Snapshot publishing and reading use a short `snapshot_mutex_` critical section plus an atomic front-buffer index. The renderer receives a complete `WorldSnapshot` copy and does not access mutable Box2D state directly.

**Shutdown**

`PhysicsThread` uses `std::condition_variable stop_cv_` to sleep between ticks without busy-waiting. Setting a stop flag and calling `stop_cv_.notify_all()` causes the worker to exit its loop and the owning thread to `join()` cleanly.

---

## Gameplay Mechanics

### Projectile Types

| # | Type | Special Ability (ActivateAbilityCmd) |
|---|---|---|
| 1 | **Standard** | No ability; baseline damage and trajectory |
| 2 | **Heavy** | No ability; high mass, high impulse on impact |
| 3 | **Splitter** | Splits into multiple smaller projectiles on activation |
| 4 | **Dasher** | Accelerates forward with a burst of speed on activation |
| 5 | **Bomber** | Detonates an explosion on activation, applying radial impulse |
| 6 | **Dropper** | Drops a secondary projectile downward on activation |
| 7 | **Boomerang** | Curves back toward the slingshot on activation |
| 8 | **Bubbler** | Expands a bubble on activation, pushing surrounding objects |
| 9 | **Inflater** | Inflates in size on activation, increasing area and mass |

### Block Materials

| Material | Relative Durability | Notes |
|---|---|---|
| **Wood** | Medium | Balanced general-purpose structure material |
| **Stone** | High | Resistant to most projectiles; requires heavy hits |
| **Glass** | Low | Shatters easily; useful for creating cascading collapses |
| **Ice** | Low-Medium | Brittle under impact; slippery surface behavior |

### Scoring & Stars

Each level defines star thresholds in its metadata:

```json
"meta": {
  "id": 1,
  "name": "level_01",
  "totalShots": 3,
  "starThresholds": [2000, 2500, 2990]
}
```

`ScoreSystem` accumulates positive score increments from destroyed targets, destroyed blocks, and unused-ammo bonuses. `LevelCompletedEvent` carries `{ win, finalScore, stars }`. Stars are determined by comparing `finalScore` against the configured thresholds, with a completed level granting at least one star.

Unused shots grant bonus points at level completion via `AmmoBonusEvent`. `ImpactResolvedEvent` provides per-collision telemetry (`outcome`, `speedBeforeMps`, `speedAfterMps`) for UI/debug feedback about projectile carry-through and blocked impacts.

### Slingshot Mechanics

`Slingshot` captures pointer/mouse press, drag, and release events. It computes a pull vector in screen pixels (`pullVectorPx`) relative to the slingshot pivot. On release it emits `LaunchCmd { pullVectorPx }`. The physics engine converts screen pixels to Box2D metres using `kPixelsPerMeter = 50` and applies the corresponding impulse to the loaded projectile body.

---

## Level Format

### JSON Example

```json
{
  "meta": {
    "id": 1,
    "name": "level_01",
    "totalShots": 3,
    "starThresholds": [2000, 2500, 2990]
  },
  "slingshot": {
    "position": [140, 600],
    "maxPull": 195
  },
  "projectiles": [
    { "type": "Standard" },
    { "type": "Splitter" },
    { "type": "Heavy" }
  ],
  "blocks": [
    {
      "shape": "rect",
      "material": "Wood",
      "position": [950, 505],
      "size": [200, 20],
      "angle": 0,
      "hp": 55
    },
    {
      "shape": "triangle",
      "material": "Stone",
      "position": [950, 465],
      "size": [56, 56],
      "vertices": [[-28, 28], [28, 28], [0, -28]],
      "angle": 0,
      "hp": 50
    }
  ],
  "targets": [
    {
      "position": [905, 475],
      "radius": 16,
      "hp": 22,
      "score": 1400
    }
  ]
}
```

### Level Fields


| Field | Type | Description |
|---|---|---|
| `meta.id` | `int` | Positive unique level identifier |
| `meta.name` | `string` | Non-empty level name (current files use `level_01`, ..., `level_010`) |
| `meta.totalShots` | `int` | Number of projectiles; must equal `projectiles.size()` |
| `meta.starThresholds` | `int[3]` | Strictly increasing thresholds for 1, 2, and 3 stars |
| `slingshot.position` | `[x,y]` | Slingshot base position in pixels |
| `slingshot.maxPull` | `number` | Maximum pull distance in pixels |
| `projectiles[].type` | `string` | Ordered projectile queue |
| `blocks[].shape` | `string` | `"rect"`, `"circle"`, or `"triangle"` |
| `blocks[].material` | `string` | `"Wood"`, `"Stone"`, `"Glass"`, or `"Ice"` |
| `blocks[].position` | `[x,y]` | Block center in pixels |
| `blocks[].size` | `[w,h]` | Required for `rect`; optional/derived for `triangle` when vertices exist |
| `blocks[].radius` | `number` | Required for `circle` |
| `blocks[].vertices` | `[x,y][]` | Exactly 3 local vertices for `triangle` blocks |
| `blocks[].static` / `indestructible` | `bool` | Optional; static and indestructible blocks are treated as static obstacles |
| `targets[].position` | `[x,y]` | Target center in pixels |
| `targets[].radius` | `number` | Target radius |
| `targets[].hp` | `number` | Target hit points |
| `targets[].score` | `int` | Points granted when the target is destroyed |

Levels are stored in `levels/level_01.json` through `levels/level_010.json` (10 levels total). Across the current level set there are 418 blocks and 38 targets; all 9 projectile types are implemented in the engine, and the current levels use them across different scenarios.

For the full current JSON contract, scoring rules, material HP guidance, and block/target
examples, see [docs/levels_guide.md](docs/levels_guide.md).
### Level Authoring Tool

`tools/tiled_to_level.py` converts a **Tiled** map export (`.tmj` format) to the gameplay JSON format above. This allows level designers to place blocks and targets visually in Tiled and export directly to the game's format.

---

## Online System

### Authentication Flow

```
  Client                              Backend
  ──────                              ───────
  POST /register ──────────────────►
    { username, password }
                       ◄─────────────
                         AuthResult { success, username, errorMessage }



  POST /login ─────────────────────►
    { username, password }
                       ◄─────────────
                         AuthResult { token, username, errorMessage }



  SessionManager.save_session() ──► session.json { token, username }



  POST /scores ────────────────────►
    Authorization: Bearer <token>
    { levelId, score, stars }
                       ◄─────────────
                         HTTP 200 OK



  GET /leaderboard?levelId=<id> ───►
                       ◄─────────────
                         LeaderboardFetchResult {
                           status: Ok | Empty | Unavailable | InvalidResponse
                           entries: [ { playerName, score, stars } ]
                         }
```

### Session Persistence

`SessionManager` writes `session.json` to disk after a successful native login. On web builds it stores the same token and username in browser `localStorage`. On the next application launch, `AccountService::load_session()` restores the session without requiring the user to log in again. `AccountService::logout()` clears the stored session.

### Leaderboard Status Codes

| `LeaderboardFetchStatus` | Meaning |
|---|---|
| `Ok` | Entries retrieved successfully |
| `Empty` | Level has no recorded scores yet |
| `Unavailable` | Server unreachable or HTTP error |
| `InvalidResponse` | Response body failed JSON parsing |

### Database Schema

![db_schema](img/db_schema.png)

| Table | Purpose |
|---|---|
| `users` | Registered players. Password stored as a hash. `username` is unique. |
| `user_scores` | Full attempt history — every score submission creates a new row. Index on `level_id` speeds up per-level history queries. |
| `user_best_scores` | Best result per player per level, updated via upsert on every submission. Composite index on `(level_id, best_score DESC, best_stars DESC)` drives leaderboard queries. All rows are cascade-deleted when the parent user is removed (`ON DELETE CASCADE`). |

### Async Operations

Score submission and leaderboard fetching expose callback-based variants:

- `submit_score_with_token_async` — fire-and-forget; result handled via callback
- `fetch_leaderboard_with_status_async` — callback delivers `LeaderboardFetchResult`

On web, these use non-blocking `emscripten_fetch`. On native, the low-level HTTP wrapper is synchronous, and UI code performs submit/fetch workflows on detached background `std::thread` workers so the render loop does not freeze. Both paths use the unified `platform/http.hpp` `Response { status_code, body, network_error, error_message }` type.

---

## Platform Support

### Target Platforms

| Feature | Native (Linux / macOS) | Web (Emscripten) |
|---|---|---|
| Rendering library | SFML 3 | Raylib 5.5 |
| HTTP client | CPR 1.10.4 | `emscripten_fetch` |
| Physics mode default | `Threaded` | `SingleThread` |
| Window management | `platform_sfml.hpp` | `platform_raylib.hpp` |
| Fullscreen toggle | F11 / Alt+Enter | Browser fullscreen API |
| Entry point | `main.cpp` (SFML loop) | `web/shell.html` (Emscripten loop) |
| Deployment | Binary executable | GitHub Pages (`web-pages.yml`) |

### Platform Abstraction Layer

`src/platform/` provides a unified API used throughout the engine and UI:

| Abstracted Type | SFML mapping | Raylib mapping |
|---|---|---|
| `Vec2f` | `sf::Vector2f` | `Vector2` |
| `Vec2u` | `sf::Vector2u` | unsigned vector pair |
| `Color` | `sf::Color` | `Color` |
| `Texture` | `sf::Texture` | `Texture2D` |
| `Window` | `sf::RenderWindow` | Raylib window state |
| `RenderTarget` | `sf::RenderTarget` | Raylib framebuffer |
| `Shader` | `sf::Shader` | Raylib `Shader` |
| `View` | `sf::View` | Raylib `Camera2D` |
| `Font` | `sf::Font` | Raylib `Font` |
| `Text` | `sf::Text` | Raylib `DrawText` params |
| `Sprite` | `sf::Sprite` | Raylib `DrawTexture` params |
| `Clock` | `sf::Clock` | `GetTime()` |
| `RenderTexture` | `sf::RenderTexture` | Raylib `RenderTexture2D` |
| `Event` | `sf::Event` variant | Raylib input poll result |

`platform/http.hpp` exposes `Response { status_code, body, network_error, error_message }` regardless of the underlying HTTP mechanism.

### World Constants

| Constant | Value | Meaning |
|---|---|---|
| `kWidthPx` | 1280 | Logical world width in pixels |
| `kHeightPx` | 720 | Logical world height in pixels |
| `kGroundTopYPx` | 600 | Y coordinate of the ground plane |
| `kPixelsPerMeter` | 50 | Scale factor: pixels per Box2D metre |

---

## Testing

Six GTest modules cover all major subsystems:

| Module | File | What is tested |
|---|---|---|
| `physics_engine_tests` | `tests/` | Win/lose status transitions, star threshold computation, collision event emission, carry-through impact resolution, triangle body snapshot correctness |
| `auth_client_tests` | `tests/` | `/register` and `/login` HTTP calls, `AuthResult` JSON parsing, network failure handling |
| `account_service_tests` | `tests/` | Session loading from `session.json`, logout flow, token persistence across `AccountService` instances |
| `online_score_client_tests` | `tests/` | Score submission POST, leaderboard JSON parsing, all four `LeaderboardFetchStatus` mappings |
| `session_manager_tests` | `tests/` | Save/load roundtrip, missing file path handling, malformed JSON graceful failure |
| `ui_render_tests` | `tests/` | Letterbox viewport calculation, `ParticleSystem` frame advance and lifetime expiry |

Run all tests:

```bash
cd build
ctest --output-on-failure
```

---

## Compliance Table

This table maps common university course requirements to their concrete implementations in AngryMipts.

| Requirement | Implementation | Location |
|---|---|---|
| **Object-Oriented Design** | Inheritance hierarchy: `Scene` -> 5 concrete scenes; abstract methods enforce lifecycle contract | `src/ui/` |
| **Encapsulation** | Private members with `snake_case_` suffix; public API only exposes necessary operations | Throughout |
| **Polymorphism** | Virtual `handle_input()`, `update()`, and `render()` dispatched through `Scene*` pointer in `SceneManager` | `src/ui/` |
| **Templates / Generics** | `ThreadSafeQueue<T>` — type-safe queue for both `Command` and `Event` types | `src/shared/` |
| **Standard Collections** | `std::queue`, `std::array`, `std::vector`, `std::variant` for events and commands | Throughout |
| **Multithreading** | Physics worker thread running at fixed 60 Hz; main thread handles UI, render, and events | `src/physics/` |
| **Thread Synchronisation** | `std::mutex`, `std::atomic<int>`, `std::condition_variable stop_cv_` | `src/physics/`, `src/shared/` |
| **Explicit synchronized communication** | `ThreadSafeQueue<Command>` and `ThreadSafeQueue<Event>` use a small mutex-protected FIFO with non-blocking `try_pop()` | `src/shared/` |
| **Design Patterns** | Command, Observer/Event, Snapshot, Strategy, Facade, Scene polymorphism — all documented above | Various |
| **File I/O** | `LevelLoader` reads 10 JSON level files; `SessionManager` reads/writes `session.json` on native; `ScoreSaver` persists local scores | `src/data/` |
| **Error Handling** | Network errors reported via `Response.network_error`; JSON parse failures -> `LeaderboardFetchStatus::InvalidResponse`; missing `session.json` handled gracefully | `src/data/`, `src/platform/` |
| **GUI** | Full game UI with menu, login, level select, HUD, result screen rendered via platform abstraction | `src/ui/`, `src/render/` |
| **Event-Driven Input** | Platform events polled per frame, dispatched to `Scene::handle_input()` | `src/ui/`, `main.cpp` |
| **Networking** | JWT-authenticated REST API: register, login, submit score, fetch leaderboard | `src/data/` |
| **Cross-Platform Build** | CMake with SFML (native) and Emscripten (web) build targets; CI on Linux and macOS | `CMakeLists.txt`, `.github/workflows/` |
| **Automated Tests** | 6 GTest modules, run via CTest in CI | `tests/` |
| **Level Data Format** | JSON with full schema: metadata, projectile list, blocks (3 shapes, 4 materials), targets | `levels/` |
| **Tool / Pipeline** | `tools/tiled_to_level.py` — level authoring pipeline from Tiled editor to game format | `tools/` |
| **CI/CD** | GitHub Actions: build, test (ccache), GitHub Pages deployment | `.github/workflows/` |

---

## Build & Run

### Prerequisites

| Tool | Version | Notes |
|---|---|---|
| CMake | ≥ 3.20 | Build system |
| C++ compiler | GCC 11+ / Clang 14+ / MSVC 2022 | C++17 required |
| SFML | 3.x | Native builds only |
| Emscripten | 3.x | Web builds only |
| GTest | Any | Tests only; required for native test builds |

Box2D 3.0.0 is used from `external/box2d` when present, otherwise fetched via CMake `FetchContent`. nlohmann_json 3.11.3 is fetched for web builds, taken from `external/nlohmann_json` when present, or resolved with `find_package` on native. CPR 1.10.4 is fetched via CMake `FetchContent` for native HTTP.

### Native Build (Linux / macOS)

```bash
git clone <repo-url> AngryMipts
cd AngryMipts

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

./build/AngryMipts
```

### Running Tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target all -j$(nproc)
cd build && ctest --output-on-failure
```

### Keyboard Shortcuts (Native)

| Key | Action |
|---|---|
| `F11` / `Alt+Enter` | Toggle fullscreen |
| `F10` | Cycle FPS cap: 60 -> 120 -> Unlimited |
| `Esc` | Exit fullscreen / return to previous scene |

---

## Project Structure

```
AngryMipts/
├── src/
│   ├── shared/           # Types, events (9 variants), commands (5), WorldSnapshot,
│   │                     # ThreadSafeQueue<T>, config constants
│   ├── physics/          # PhysicsEngine (Box2D), PhysicsThread (worker_loop),
│   │                     # PhysicsRuntime (facade), unit conversion utilities
│   ├── core/             # ScoreSystem
│   ├── data/             # LevelLoader, AuthClient, SessionManager,
│   │                     # OnlineScoreClient, AccountService, ScoreSaver
│   ├── render/           # Renderer, TextureManager, ParticleSystem, SfxPlayer
│   ├── ui/               # SceneManager, Scene base, GameScene, MenuScene,
│   │                     # LoginScene, LevelSelectScene, ResultScene, Slingshot,
│   │                     # view utilities
│   ├── platform/         # platform_sfml.hpp, platform_raylib.hpp, http.hpp
│   │                     # (unified Vec2f, Color, Texture, Window, Event, …)
│   └── main.cpp          # Entry point
├── tests/
│   ├── physics_engine_tests.*
│   ├── auth_client_tests.*
│   ├── account_service_tests.*
│   ├── online_score_client_tests.*
│   ├── session_manager_tests.*
│   └── ui_render_tests.*
├── levels/
│   ├── level_01.json      # Tutorial
│   ├── level_05.json
│   └── level_010.json     # 10 total levels
├── tools/
│   └── tiled_to_level.py  # Tiled .tmj -> gameplay JSON converter
├── web/
│   └── shell.html         # Emscripten HTML shell
├── assets/
│   └── fonts/             # Game fonts
├── .github/
│   └── workflows/
│       ├── ci.yml         # Build + CTest on GitHub-hosted Linux/macOS runners
│       └── web-pages.yml  # GitHub Pages deployment
├── cmake/                 # CMake helper modules
├── CMakeLists.txt
└── build_web.sh           # Web build script
```

---

## Team

| Role | Responsibilities | Source Ownership |
|---|---|---|
| **Шафран Юрий Б01-403** — Physics & Core Logic | Box2D integration, physics thread, collision events, projectile behaviour, score computation | `src/physics/`, `src/core/` |
| **Григорьев Даниил Б01-407** — UI & Render | All scenes, slingshot input, renderer, textures, particles, SFX, platform abstraction UI side | `src/render/`, `src/ui/`, `src/platform/` |
| **Остафейчук Роман Б01-401** — Levels & Data Pipeline | Level JSON files, level balance, level authoring guide, Tiled conversion pipeline, level loading contract | `levels/`, `docs/`, `tools/`, `src/data/level_loader.*` |

`src/shared/` is a **jointly owned** module; changes require agreement from all three participants.

---
