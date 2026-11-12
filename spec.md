# ChainX Browser — Software Specification

**Project Name:** ChainX Browser  
**Version:** 1.0.0  
**Status:** Planning  
**Language:** C++20  
**Application Type:** Desktop Web Browser  
**License:** MIT (proposed)

---

# 1. Project Overview

ChainX is a modern, lightweight, privacy-focused desktop web browser built with C++ and Qt.

The primary goal is to build a Chromium-powered browser with a modern user interface, Web3 compatibility, privacy features, and extensible architecture.

ChainX will provide a familiar browsing experience while introducing Web3-oriented capabilities.

# 2. Project Goals

- Build a functional desktop web browser using C++.
- Provide a modern tab-based browsing interface.
- Support modern websites using Chromium.
- Implement bookmarks and browsing history.
- Provide private browsing mode.
- Implement download management.
- Add an integrated Web3 experience.
- Support Linux, Windows, and macOS.
- Provide a distributable Linux `.deb` package.

# 3. Technology Stack

| Component | Technology |
|---|---|
| Programming Language | C++20 |
| GUI Framework | Qt 6 |
| Browser Engine | Qt WebEngine |
| Build System | CMake |
| Database | SQLite |
| JSON | nlohmann/json |
| HTTP Client | Qt Network |
| Icons | SVG |
| Testing | GoogleTest |
| Packaging | CPack / Debian |
| Version Control | Git |

# 4. Core Features

## 4.1 Browser Engine

- Chromium-based rendering.
- HTML5 support.
- CSS3 support.
- JavaScript support.
- WebAssembly support.
- WebGL support.
- Modern website compatibility.

## 4.2 Tab Management

- Create new tabs.
- Close tabs.
- Switch between tabs.
- Duplicate tabs.
- Restore recently closed tabs.
- Tab keyboard shortcuts.
- Tab loading indicators.

## 4.3 Navigation

- Address bar.
- URL navigation.
- Search engine integration.
- Back and forward.
- Reload and stop.
- Home page.
- URL autocomplete.

## 4.4 Bookmarks

- Add bookmarks.
- Remove bookmarks.
- Organize bookmarks into folders.
- Bookmark toolbar.
- Search bookmarks.

## 4.5 History

- Store browsing history.
- Search browsing history.
- Delete individual history records.
- Clear all browsing history.
- Configurable history retention.

## 4.6 Download Manager

- Download files.
- Download progress.
- Pause and resume where supported.
- Cancel downloads.
- Download history.
- Configure download directory.

## 4.7 Privacy & Security

- Private browsing.
- Cookie management.
- Clear cache.
- Clear browsing data.
- HTTPS support.
- Certificate error handling.
- Permission management.
- JavaScript permission controls.

## 4.8 Web3 Features

- Detect Web3-enabled websites.
- Ethereum provider compatibility.
- EIP-1193 provider integration (future milestone).
- Wallet connection interface.
- Network information.
- dApp compatibility.
- Wallet security permissions.

Wallet private keys must never be exposed to websites.

# 5. User Interface

## Main Browser Window

Components:

1. Application menu.
2. Tab bar.
3. Navigation toolbar.
4. Address and search bar.
5. Bookmark toolbar.
6. Web content area.
7. Download indicator.
8. Status information.

## Browser Pages

- New Tab Page.
- Settings.
- History.
- Downloads.
- Bookmarks.
- Privacy Dashboard.
- Web3 Dashboard.

# 6. Architecture

Use a modular, layered architecture.

```text
┌─────────────────────────────────────┐
│           ChainX Browser UI         │
│       Qt Widgets / Qt WebEngine     │
├─────────────────────────────────────┤
│           Application Layer         │
│  BrowserController / TabController  │
├─────────────────────────────────────┤
│             Core Layer              │
│ Navigation / Permissions / Events   │
├─────────────────────────────────────┤
│            Services Layer           │
│ History / Bookmarks / Downloads     │
├─────────────────────────────────────┤
│          Web3 Integration           │
│ Wallet Interface / Provider Bridge  │
├─────────────────────────────────────┤
│          Infrastructure             │
│       SQLite / Qt Network           │
└─────────────────────────────────────┘
```

# 7. Project Structure

```text
chainx-browser/
│
├── CMakeLists.txt
├── README.md
├── LICENSE
├── SPEC.md
│
├── assets/
│   ├── icons/
│   ├── images/
│   └── styles/
│
├── src/
│   ├── main.cpp
│   │
│   ├── app/
│   │   ├── Application.hpp
│   │   └── Application.cpp
│   │
│   ├── browser/
│   │   ├── BrowserWindow.hpp
│   │   ├── BrowserWindow.cpp
│   │   ├── BrowserTab.hpp
│   │   ├── BrowserTab.cpp
│   │   ├── BrowserController.hpp
│   │   └── BrowserController.cpp
│   │
│   ├── navigation/
│   │   ├── AddressBar.hpp
│   │   └── AddressBar.cpp
│   │
│   ├── services/
│   │   ├── HistoryService.hpp
│   │   ├── BookmarkService.hpp
│   │   └── DownloadService.hpp
│   │
│   ├── database/
│   │   ├── Database.hpp
│   │   └── Database.cpp
│   │
│   ├── web3/
│   │   ├── Web3Provider.hpp
│   │   └── Web3Provider.cpp
│   │
│   ├── settings/
│   │   ├── Settings.hpp
│   │   └── Settings.cpp
│   │
│   └── utils/
│       ├── Logger.hpp
│       └── Logger.cpp
│
├── tests/
│   ├── CMakeLists.txt
│   └── unit/
│
├── packaging/
│   └── debian/
│
└── docs/
    ├── ARCHITECTURE.md
    └── DEVELOPMENT.md
```

# 8. Database Design

SQLite will store local browser data.

## bookmarks

- id
- title
- url
- folder_id
- created_at

## bookmark_folders

- id
- name
- parent_id

## history

- id
- title
- url
- visited_at

## downloads

- id
- filename
- url
- file_path
- total_size
- downloaded_size
- status
- created_at

## settings

- id
- key
- value

# 9. Security Requirements

- Never store passwords in plaintext.
- Never expose wallet secrets to websites.
- Validate external URLs.
- Isolate private browsing sessions.
- Use secure storage for future wallet credentials.
- Require explicit permission for wallet connections.
- Prevent unauthorized website access to native application APIs.
- Keep Web3 integration disabled until security validation is complete.

# 10. Performance Requirements

- Efficient tab management.
- Asynchronous network operations.
- Avoid blocking the GUI thread.
- Lazy initialization where practical.
- Minimize unnecessary memory allocations.
- Proper resource cleanup.
- Monitor memory usage per browser session.

Note: Chromium-based rendering has substantial memory requirements. ChainX will not attempt to implement its own rendering engine.

# 11. Platform Support

| Platform | Distribution |
|---|---|
| Linux Mint | `.deb` |
| Ubuntu | `.deb` |
| Debian | `.deb` |
| Windows | `.exe` installer |
| macOS | `.dmg` |

# 12. Development Milestones

## Milestone 01 — Browser Foundation

- Initialize C++20 project.
- Configure Qt 6 and CMake.
- Create main application window.
- Integrate Qt WebEngine.
- Implement address bar.
- Implement URL navigation.
- Implement back, forward, reload.
- Build first functional browser.

**Deliverable:** Working desktop browser.

## Milestone 02 — Tab Management

- Multiple tabs.
- Tab switching.
- Tab closing.
- New tab page.
- Keyboard shortcuts.

**Deliverable:** Multi-tab browser.

## Milestone 03 — Browser Services

- Bookmarks.
- Browsing history.
- SQLite integration.
- Download manager.

**Deliverable:** Persistent browser data.

## Milestone 04 — Privacy & Security

- Private browsing.
- Cookie management.
- Cache management.
- Permission controls.
- Security settings.

**Deliverable:** Privacy-focused browser.

## Milestone 05 — Web3 Integration

- Web3 detection.
- Ethereum provider interface.
- Wallet connection interface.
- dApp compatibility.
- Network selection.
- Permission-based wallet access.

**Deliverable:** Web3-ready browser.

## Milestone 06 — Production Release

- UI refinement.
- Automated tests.
- Performance optimization.
- Error handling.
- Debian packaging.
- Windows and macOS packaging.
- Release documentation.

**Deliverable:** ChainX Browser v1.0.0.

# 13. Build & Development

Required packages on Ubuntu/Linux Mint:

```bash
sudo apt update

sudo apt install -y \
  build-essential \
  cmake \
  ninja-build \
  git \
  qt6-base-dev \
  qt6-webengine-dev \
  libsqlite3-dev
```

Build:

```bash
cmake -S . -B build -G Ninja

cmake --build build
```

Run:

```bash
./build/chainx-browser
```

# 14. Initial Release Scope

The first release must prioritize:

1. Working Chromium browser engine.
2. Navigation.
3. Multi-tab browsing.
4. Bookmarks.
5. History.
6. Downloads.
7. Linux `.deb` packaging.

Web3 wallet functionality will be introduced only after the browser foundation and security architecture are stable.

# 15. Success Criteria

ChainX v1.0.0 will be considered ready when:

- Modern websites render correctly.
- Multiple tabs work reliably.
- Navigation works correctly.
- Bookmarks and history persist.
- Downloads work.
- Private browsing does not retain private session history.
- No critical security vulnerabilities remain.
- Linux `.deb` installation works.
- Application can launch and operate without development dependencies.

---

**Project Vision:** Build a modern, extensible, privacy-focused C++ browser with a strong foundation for the decentralized web.
