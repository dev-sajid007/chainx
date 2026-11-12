# Development

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

`CHAINX_BUILD_TESTS=ON` is the default. Tests are skipped automatically when
GoogleTest is not installed.

## Running

```bash
./build/chainx-browser
```

Enable verbose Chromium logging when debugging WebEngine:

```bash
QTWEBENGINE_CHROMIUM_FLAGS="--enable-logging --v=1" ./build/chainx-browser
```

Run under a temporary home directory to test first-run behaviour without touching
real data:

```bash
HOME=$(mktemp -d) ./build/chainx-browser
```

## Tests

```bash
ctest --test-dir build --output-on-failure
```

Unit tests cover URL resolution and the SQLite-backed services. They use an
in-memory database, so they never touch user data.

## Adding a service

1. Add `FooService.hpp/.cpp` under `src/services/`.
2. Add both files to `CHAINX_CORE_SOURCES` in the top-level `CMakeLists.txt`.
3. Construct it in `Application::initialize()` and pass a reference to
   `BrowserWindow` / `BrowserController`.
4. If it needs schema, add a migration in `src/database/Database.cpp`.

## Adding an icon

1. Drop the SVG into `assets/icons/`.
2. Register it in `assets/assets.qrc`.
3. Load it with `QIcon(":/icons/name.svg")`. `BrowserWindow::iconFor()` falls
   back to a standard Qt icon when SVG support is unavailable.

## Code style

- C++20, Qt naming conventions (`camelCase` methods, `m_` members).
- One class per file, `#pragma once` headers.
- Keep the GUI thread free of blocking I/O.
- Prefer signals/slots over callbacks for cross-layer communication.

## Roadmap

| Milestone | State |
|---|---|
| 01 — Browser foundation | Done |
| 02 — Tab management | Done |
| 03 — Browser services | Done |
| 04 — Privacy & security | Pending |
| 05 — Web3 integration | Detection only |
| 06 — Production release | Packaging scaffolded |

The next planned work is Milestone 04: private browsing windows backed by an
off-the-record profile, cookie and cache management, and permission handling for
camera, microphone, geolocation and notifications.
