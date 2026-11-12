# ChainX Architecture

ChainX follows the layered architecture described in `spec.md`:

```text
┌─────────────────────────────────────┐
│           ChainX Browser UI         │  BrowserWindow, AddressBar, HistoryDialog
│       Qt Widgets / Qt WebEngine     │
├─────────────────────────────────────┤
│           Application Layer         │  Application, BrowserController
├─────────────────────────────────────┤
│             Core Layer              │  Navigation, URL resolution, events
├─────────────────────────────────────┤
│            Services Layer           │  History, Bookmarks, Downloads
├─────────────────────────────────────┤
│          Web3 Integration           │  Web3Provider (detection only)
├─────────────────────────────────────┤
│          Infrastructure             │  SQLite (Qt SQL), Qt Network
└─────────────────────────────────────┘
```

## Ownership

`Application` is the composition root. It creates and owns:

- `Database` — a single SQLite connection on the GUI thread
- `Settings` — `QSettings`-backed preferences
- `HistoryService`, `BookmarkService`, `DownloadService`
- `Web3Provider`
- `QWebEngineProfile` — persistent Chromium profile (`~/.local/share/ChainX/ChainX/profile`)
- one or more `BrowserWindow` instances

`BrowserWindow` builds the chrome and owns a `BrowserController`. The controller
owns the `QTabWidget` and the tab lifecycle, and is independent of the window so
that private-browsing windows can reuse it with a different profile later.
Modal dialogs for history, the bookmark manager and settings are created on
demand from the window and talk to the services directly.

## Tabs

Each tab is a `BrowserTab`, a `QWebEngineView` subclass that:

- creates its own `QWebEnginePage` against the shared profile
- applies ChainX web settings
- routes `target="_blank"` / `window.open()` through a factory that asks the
  controller to open a real tab

`BrowserController` wires per-tab signals into the window: title, URL, favicon,
load progress, link hover, and window-close requests. It also records history on
successful top-level navigations and probes pages for `window.ethereum`.

## Persistence

SQLite schema (see `Database::migrate`):

| Table | Purpose |
|---|---|
| `bookmarks` | Bookmark entries with optional folder |
| `bookmark_folders` | Hierarchical folders |
| `history` | One row per visit (URL, title, timestamp) |
| `downloads` | Download state, size, destination |
| `settings` | Reserved for key/value storage |

Schema changes are versioned with `PRAGMA user_version`. Add a new `Migration`
entry in `Database.cpp` and bump `kSchemaVersion`; existing installs migrate
automatically inside a transaction.

## Downloads

`DownloadService` connects to `QWebEngineProfile::downloadRequested`, chooses a
unique destination in the configured download directory, records the download,
and mirrors state changes into SQLite. Active requests are tracked in memory so
the UI can cancel, pause or resume them. Downloads left running when the process
exits are reconciled to `interrupted` on the next start.

## Web3

`Web3Provider::isEnabled()` always returns `false` in v1.0.0. ChainX only probes
whether a page already exposes `window.ethereum` and shows a status badge. No
native APIs or secrets are exposed to web content. Enabling EIP-1193 requires the
security work in `spec.md` §9: origin-scoped permissions, explicit user consent,
and secure storage for credentials.

## Threading

All database and UI work happens on the GUI thread. Qt WebEngine performs network
and rendering work on its own processes and threads. Any future long-running work
must be moved to `QThread`/`QtConcurrent` rather than blocking the event loop.
