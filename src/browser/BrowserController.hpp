#pragma once

#include <QKeySequence>
#include <QList>
#include <QObject>
#include <QUrl>

class QAction;
class QTabWidget;
class QWebEngineProfile;

namespace chainx {

class BookmarkService;
class BrowserTab;
class DownloadService;
class HistoryService;
class Settings;
class Web3Provider;

/**
 * Owns the tab model and browser actions.
 *
 * The controller is deliberately independent of BrowserWindow so that the tab
 * lifecycle and navigation logic can be unit-tested and reused by future
 * windows (for example private browsing windows).
 */
class BrowserController : public QObject
{
    Q_OBJECT

public:
    BrowserController(Settings& settings,
                      HistoryService& history,
                      BookmarkService& bookmarks,
                      DownloadService& downloads,
                      Web3Provider& web3,
                      QWebEngineProfile* profile,
                      bool privateMode = false,
                      QObject* parent = nullptr);

    QTabWidget* tabWidget() const;

    bool isPrivate() const;

    /// Re-applies preference-backed settings to every open tab.
    void applySettings();

    BrowserTab* createTab(const QUrl& url = QUrl(), bool foreground = true);
    BrowserTab* currentTab() const;
    int tabCount() const;

    bool canRestoreClosedTab() const;

    QAction* newTabAction() const;
    QAction* closeTabAction() const;
    QAction* duplicateTabAction() const;
    QAction* restoreTabAction() const;
    QAction* nextTabAction() const;
    QAction* previousTabAction() const;
    QAction* backAction() const;
    QAction* forwardAction() const;
    QAction* reloadAction() const;
    QAction* stopAction() const;
    QAction* homeAction() const;
    QAction* focusAddressBarAction() const;
    QAction* bookmarkAction() const;

public slots:
    void closeTab(int index);
    void closeCurrentTab();
    void duplicateCurrentTab();
    void restoreClosedTab();
    void navigateCurrentTab(const QUrl& url);
    void goBack();
    void goForward();
    void reload();
    void stop();
    void goHome();
    void toggleBookmarkForCurrentTab();

signals:
    void currentTabChanged(chainx::BrowserTab* tab);
    void tabCountChanged(int count);
    void statusMessage(const QString& message);
    void bookmarkStateChanged(bool bookmarked);
    void web3StatusChanged(bool detected);
    void loadingChanged(bool loading);
    void loadProgressChanged(int progress);
    void focusAddressBarRequested();

private:
    BrowserTab* makeTab();
    void wireTab(BrowserTab* tab);
    void updateNavigationState();
    void updateBookmarkState();
    void onCurrentTabChanged(int index);
    void recordHistory(BrowserTab* tab);

    QAction* makeAction(const QString& text,
                        const QList<QKeySequence>& shortcuts,
                        const QString& iconPath = {});

    Settings& m_settings;
    HistoryService& m_history;
    BookmarkService& m_bookmarks;
    DownloadService& m_downloads;
    Web3Provider& m_web3;
    QWebEngineProfile* m_profile = nullptr;
    bool m_privateMode = false;

    QTabWidget* m_tabWidget = nullptr;
    QList<BrowserTab*> m_tabs;

    struct ClosedTab
    {
        QUrl url;
        QString title;
    };
    QList<ClosedTab> m_closedTabs;

    QAction* m_newTabAction = nullptr;
    QAction* m_closeTabAction = nullptr;
    QAction* m_duplicateTabAction = nullptr;
    QAction* m_restoreTabAction = nullptr;
    QAction* m_nextTabAction = nullptr;
    QAction* m_previousTabAction = nullptr;
    QAction* m_backAction = nullptr;
    QAction* m_forwardAction = nullptr;
    QAction* m_reloadAction = nullptr;
    QAction* m_stopAction = nullptr;
    QAction* m_homeAction = nullptr;
    QAction* m_focusAddressBarAction = nullptr;
    QAction* m_bookmarkAction = nullptr;
};

} // namespace chainx
