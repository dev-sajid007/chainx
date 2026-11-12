#pragma once

#include <QIcon>
#include <QMainWindow>
#include <QStyle>
#include <QUrl>

class QAction;
class QLabel;
class QMenu;
class QProgressBar;
class QToolBar;
class QToolButton;
class QWebEngineProfile;

namespace chainx {

class AddressBar;
class BookmarkService;
class BrowserController;
class BrowserTab;
class DownloadService;
class HistoryService;
class Settings;
class Web3Provider;

/**
 * Main browser window: chrome (menus, toolbars, address bar, status bar) plus
 * the tab widget owned by BrowserController.
 */
class BrowserWindow : public QMainWindow
{
    Q_OBJECT

public:
    BrowserWindow(Settings& settings,
                  HistoryService& history,
                  BookmarkService& bookmarks,
                  DownloadService& downloads,
                  Web3Provider& web3,
                  QWebEngineProfile* profile,
                  bool privateMode = false,
                  QWidget* parent = nullptr);

    BrowserController* controller() const;

signals:
    void newWindowRequested(bool privateMode);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildUi();
    void buildMenus();
    void buildToolBars();
    void buildStatusBar();

    void updateForCurrentTab(BrowserTab* tab);
    void updateLoadingState(bool loading);
    void updateBookmarkState(bool bookmarked);
    void updateWeb3Badge(bool detected);
    void showHistoryDialog();
    void showBookmarkManager();
    void showPrivacyDialog();
    void showSettings();
    void rebuildBookmarkMenu();
    void rebuildBookmarkToolBar();
    void rebuildDownloadsMenu();
    void setBookmarkToolBarVisible(bool visible);
    void clearHistory();
    void showAbout();

    void zoomIn();
    void zoomOut();
    void zoomReset();

    QIcon iconFor(const QString& resourcePath, QStyle::StandardPixmap fallback) const;

    Settings& m_settings;
    HistoryService& m_history;
    BookmarkService& m_bookmarks;
    DownloadService& m_downloads;
    Web3Provider& m_web3;
    QWebEngineProfile* m_profile = nullptr;
    bool m_privateMode = false;

    BrowserController* m_controller = nullptr;

    AddressBar* m_addressBar = nullptr;
    QToolBar* m_mainToolBar = nullptr;
    QToolBar* m_bookmarkToolBar = nullptr;
    QMenu* m_bookmarksMenu = nullptr;
    QMenu* m_allBookmarksMenu = nullptr;
    QMenu* m_downloadsMenu = nullptr;
    QToolButton* m_downloadsButton = nullptr;

    QLabel* m_statusLabel = nullptr;
    QLabel* m_web3Label = nullptr;
    QLabel* m_privateBadge = nullptr;
    QProgressBar* m_progressBar = nullptr;

    QAction* m_bookmarkToolBarAction = nullptr;
    QAction* m_historyAction = nullptr;
    QAction* m_zoomInAction = nullptr;
    QAction* m_zoomOutAction = nullptr;
    QAction* m_zoomResetAction = nullptr;
    QAction* m_fullScreenAction = nullptr;
};

} // namespace chainx
