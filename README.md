# ChainX Browser

A lightweight, privacy-focused desktop web browser built with **C++20** and **Qt 6**,
powered by Chromium through **Qt WebEngine**.

ChainX aims to deliver a familiar, fast browsing experience with a solid foundation
for Web3 capabilities. Web3 wallet integration is intentionally disabled until the
security model described in [`spec.md`](spec.md) has been validated.

## Status

Milestone 01 (browser foundation), Milestone 02 (tab management),
Milestone 03 (browser services) and Milestone 04 (privacy & security) are
implemented:

- Chromium rendering via Qt WebEngine
- Multi-tab browsing with duplicate / reopen-closed-tab support
- Address bar with URL resolution, search fallback and autocompletion
- Back / forward / reload / stop / home navigation
- Bookmarks (SQLite-backed) with a bookmarks bar
- Bookmark manager: folders, rename, move, search and delete
- Settings page: homepage, search engine, download folder, history retention
- Browsing history (SQLite-backed) with search, per-entry delete and retention
- Download manager backed by Qt WebEngine, with progress in the status bar
- Private browsing windows backed by an off-the-record profile
- Clear browsing data: history, downloads, cookies and cache
- Permission prompts for camera, microphone, location and notifications
- JavaScript and insecure-content controls
- Built-in new tab page
- Web3 provider *detection* only (`window.ethereum`), disabled by design
- Unit tests (GoogleTest) for URL handling and persistence
- Debian packaging through CPack

## Requirements

- CMake 3.21+
- A C++20 compiler (GCC 11+ or Clang 14+)
- Qt 6.4+ with the WebEngine, Widgets, Network and SQL modules
- GoogleTest (optional, for unit tests)

### Ubuntu / Linux Mint / Debian

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  ninja-build \
  git \
  qt6-base-dev \
  qt6-webengine-dev \
  qt6-svg-dev \
  libgtest-dev
```

`qt6-svg-dev` is needed for SVG toolbar icons; without it ChainX falls back to
standard Qt icons.

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

Run:

```bash
./build/chainx-browser
```

Start with a private window:

```bash
./build/chainx-browser --private
```

Run the tests:

```bash
ctest --test-dir build --output-on-failure
```

## Packaging

```bash
cmake --build build
cd build
cpack -G DEB
sudo apt install ./chainx-browser_1.0.0_amd64.deb
```

## Keyboard shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl+N` | New window |
| `Ctrl+Shift+N` | New private window |
| `Ctrl+T` | New tab |
| `Ctrl+W` | Close tab |
| `Ctrl+Shift+T` | Reopen closed tab |
| `Ctrl+Shift+K` | Duplicate tab |
| `Ctrl+Tab` / `Ctrl+Shift+Tab` | Next / previous tab |
| `Ctrl+L` | Focus address bar |
| `Ctrl+D` | Bookmark current page |
| `Ctrl+Shift+O` | Manage bookmarks |
| `Ctrl+H` | History |
| `Ctrl+,` | Settings |
| `Ctrl+Shift+Delete` | Clear browsing data |
| `Alt+Left` / `Alt+Right` | Back / forward |
| `Ctrl+R` / `F5` | Reload |
| `Ctrl+.` | Stop loading |
| `Alt+Home` | Home page |
| `Ctrl+=` / `Ctrl+-` / `Ctrl+0` | Zoom in / out / reset |
| `F11` | Full screen |

## Data locations

ChainX follows the XDG base directory specification:

- Database and profile: `~/.local/share/ChainX/ChainX/`
- Settings: `~/.config/ChainX/ChainX.ini`
- Logs: `~/.local/share/ChainX/ChainX/chainx.log`

Private windows use an in-memory profile: cookies, cache, history and their
download list are discarded when the window closes.

## Project layout

```text
src/
├── app/          Application bootstrap and service ownership
├── browser/      Window, tabs, controller, dialogs
├── database/     SQLite connection and schema migrations
├── navigation/   Address bar
├── services/     History, bookmarks, downloads
├── settings/     Persistent preferences
├── utils/        Logging and URL helpers
└── web3/         Provider detection (integration disabled)
```

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for the layered design and
[`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md) for contributor notes.

## License

MIT. See [`LICENSE`](LICENSE).
