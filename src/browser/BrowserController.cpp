#include "browser/BrowserController.hpp"

#include <QAction>
#include <QIcon>
#include <QKeySequence>
#include <QTabWidget>
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

#include "browser/BrowserTab.hpp"
#include "services/BookmarkService.hpp"
#include "services/DownloadService.hpp"
#include "services/HistoryService.hpp"
#include "settings/Settings.hpp"
#include "utils/UrlUtils.hpp"
#include "web3/Web3Provider.hpp"

namespace chainx {
namespace {

constexpr int kMaxClosedTabs = 10;

} // namespace

BrowserController::BrowserController(Settings& settings,
                                     HistoryService& history,
                                     BookmarkService& bookmarks,
                                     DownloadService& downloads,
                                     Web3Provider& web3,
                                     QWebEngineProfile* profile,
                                     bool privateMode,
                                     QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_history(history)
    , m_bookmarks(bookmarks)
    , m_downloads(downloads)
    , m_web3(web3)
    , m_profile(profile)
    , m_privateMode(privateMode)
{
    m_tabWidget = new QTabWidget();
    m_tabWidget->setObjectName(QStringLiteral("tabWidget"));
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setElideMode(Qt::ElideRight);
    m_tabWidget->setUsesScrollButtons(true);
    m_tabWidget->setTabBarAutoHide(false);

    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &BrowserController::onCurrentTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested,
            this, &BrowserController::closeTab);

    m_newTabAction = makeAction(tr("New Tab"),
                                {QKeySequence(QStringLiteral("Ctrl+T"))},
                                QStringLiteral(":/icons/newtab.svg"));
    m_closeTabAction = makeAction(tr("Close Tab"),
                                  {QKeySequence(QStringLiteral("Ctrl+W"))});
    m_duplicateTabAction = makeAction(tr("Duplicate Tab"),
                                      {QKeySequence(QStringLiteral("Ctrl+Shift+K"))});
    m_restoreTabAction = makeAction(tr("Reopen Closed Tab"),
                                    {QKeySequence(QStringLiteral("Ctrl+Shift+T"))});
    m_nextTabAction = makeAction(tr("Next Tab"),
                                 {QKeySequence(QStringLiteral("Ctrl+Tab")),
                                  QKeySequence(QStringLiteral("Ctrl+PgDown"))});
    m_previousTabAction = makeAction(tr("Previous Tab"),
                                     {QKeySequence(QStringLiteral("Ctrl+Shift+Tab")),
                                      QKeySequence(QStringLiteral("Ctrl+PgUp"))});
    m_backAction = makeAction(tr("Back"),
                              {QKeySequence(QStringLiteral("Alt+Left"))},
                              QStringLiteral(":/icons/back.svg"));
    m_forwardAction = makeAction(tr("Forward"),
                                 {QKeySequence(QStringLiteral("Alt+Right"))},
                                 QStringLiteral(":/icons/forward.svg"));
    m_reloadAction = makeAction(tr("Reload"),
                                {QKeySequence(QStringLiteral("Ctrl+R")),
                                 QKeySequence(QStringLiteral("F5"))},
                                QStringLiteral(":/icons/reload.svg"));
    m_stopAction = makeAction(tr("Stop"),
                              {QKeySequence(QStringLiteral("Ctrl+."))},
                              QStringLiteral(":/icons/stop.svg"));
    m_homeAction = makeAction(tr("Home"),
                              {QKeySequence(QStringLiteral("Alt+Home"))},
                              QStringLiteral(":/icons/home.svg"));
    m_focusAddressBarAction = makeAction(tr("Focus Address Bar"),
                                         {QKeySequence(QStringLiteral("Ctrl+L"))});
    m_bookmarkAction = makeAction(tr("Bookmark This Page"),
                                  {QKeySequence(QStringLiteral("Ctrl+D"))},
                                  QStringLiteral(":/icons/star.svg"));

    connect(m_newTabAction, &QAction::triggered, this, [this] { createTab(); });
    connect(m_closeTabAction, &QAction::triggered, this, &BrowserController::closeCurrentTab);
    connect(m_duplicateTabAction, &QAction::triggered,
            this, &BrowserController::duplicateCurrentTab);
    connect(m_restoreTabAction, &QAction::triggered,
            this, &BrowserController::restoreClosedTab);
    connect(m_nextTabAction, &QAction::triggered, this, [this] {
        if (m_tabWidget->count() > 1) {
            m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() + 1)
                                         % m_tabWidget->count());
        }
    });
    connect(m_previousTabAction, &QAction::triggered, this, [this] {
        if (m_tabWidget->count() > 1) {
            m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() - 1
                                          + m_tabWidget->count())
                                         % m_tabWidget->count());
        }
    });
    connect(m_backAction, &QAction::triggered, this, &BrowserController::goBack);
    connect(m_forwardAction, &QAction::triggered, this, &BrowserController::goForward);
    connect(m_reloadAction, &QAction::triggered, this, &BrowserController::reload);
    connect(m_stopAction, &QAction::triggered, this, &BrowserController::stop);
    connect(m_homeAction, &QAction::triggered, this, &BrowserController::goHome);
    connect(m_focusAddressBarAction, &QAction::triggered,
            this, &BrowserController::focusAddressBarRequested);
    connect(m_bookmarkAction, &QAction::triggered,
            this, &BrowserController::toggleBookmarkForCurrentTab);

    connect(&m_bookmarks, &BookmarkService::bookmarksChanged,
            this, &BrowserController::updateBookmarkState);

    connect(&m_web3, &Web3Provider::detectionChanged,
            this, [this](QWebEnginePage* page, bool detected) {
                if (BrowserTab* tab = currentTab(); tab && tab->page() == page) {
                    emit web3StatusChanged(detected);
                }
            });

    updateNavigationState();
}

QTabWidget* BrowserController::tabWidget() const
{
    return m_tabWidget;
}

bool BrowserController::isPrivate() const
{
    return m_privateMode;
}

void BrowserController::applySettings()
{
    const bool javascript = m_settings.javascriptEnabled();
    const bool blockInsecure = m_settings.blockInsecureContent();

    for (BrowserTab* tab : m_tabs) {
        if (!tab || !tab->page()) {
            continue;
        }
        tab->setJavascriptEnabled(javascript);
        tab->page()->settings()->setAttribute(
            QWebEngineSettings::AllowRunningInsecureContent, !blockInsecure);
    }
}

BrowserTab* BrowserController::createTab(const QUrl& url, bool foreground)
{
    BrowserTab* tab = makeTab();

    const QUrl target = (!url.isValid() || url.isEmpty()) ? UrlUtils::newTabUrl() : url;
    tab->setUrl(target);

    const int index = m_tabWidget->addTab(tab, tab->displayTitle());
    m_tabWidget->setTabToolTip(index, target.toString());

    if (foreground) {
        m_tabWidget->setCurrentIndex(index);
    }

    emit tabCountChanged(m_tabs.size());
    updateNavigationState();
    return tab;
}

BrowserTab* BrowserController::currentTab() const
{
    return qobject_cast<BrowserTab*>(m_tabWidget->currentWidget());
}

int BrowserController::tabCount() const
{
    return m_tabs.size();
}

bool BrowserController::canRestoreClosedTab() const
{
    return !m_closedTabs.isEmpty();
}

QAction* BrowserController::newTabAction() const
{
    return m_newTabAction;
}

QAction* BrowserController::closeTabAction() const
{
    return m_closeTabAction;
}

QAction* BrowserController::duplicateTabAction() const
{
    return m_duplicateTabAction;
}

QAction* BrowserController::restoreTabAction() const
{
    return m_restoreTabAction;
}

QAction* BrowserController::nextTabAction() const
{
    return m_nextTabAction;
}

QAction* BrowserController::previousTabAction() const
{
    return m_previousTabAction;
}

QAction* BrowserController::backAction() const
{
    return m_backAction;
}

QAction* BrowserController::forwardAction() const
{
    return m_forwardAction;
}

QAction* BrowserController::reloadAction() const
{
    return m_reloadAction;
}

QAction* BrowserController::stopAction() const
{
    return m_stopAction;
}

QAction* BrowserController::homeAction() const
{
    return m_homeAction;
}

QAction* BrowserController::focusAddressBarAction() const
{
    return m_focusAddressBarAction;
}

QAction* BrowserController::bookmarkAction() const
{
    return m_bookmarkAction;
}

void BrowserController::closeTab(int index)
{
    auto* tab = qobject_cast<BrowserTab*>(m_tabWidget->widget(index));
    if (!tab) {
        return;
    }

    const QUrl url = tab->url();
    if (UrlUtils::isRecordable(url) && !UrlUtils::isNewTabPage(url)) {
        m_closedTabs.prepend(ClosedTab{url, tab->displayTitle()});
        while (m_closedTabs.size() > kMaxClosedTabs) {
            m_closedTabs.removeLast();
        }
    }

    m_tabs.removeAll(tab);
    m_tabWidget->removeTab(index);
    tab->deleteLater();

    emit tabCountChanged(m_tabs.size());

    if (m_tabs.isEmpty()) {
        createTab();
    }

    updateNavigationState();
}

void BrowserController::closeCurrentTab()
{
    if (currentTab()) {
        closeTab(m_tabWidget->currentIndex());
    }
}

void BrowserController::duplicateCurrentTab()
{
    if (BrowserTab* tab = currentTab()) {
        createTab(tab->url(), true);
    }
}

void BrowserController::restoreClosedTab()
{
    if (m_closedTabs.isEmpty()) {
        return;
    }

    const ClosedTab entry = m_closedTabs.takeFirst();
    createTab(entry.url, true);
}

void BrowserController::navigateCurrentTab(const QUrl& url)
{
    if (!url.isValid() || url.isEmpty()) {
        return;
    }

    if (BrowserTab* tab = currentTab()) {
        tab->setUrl(url);
    } else {
        createTab(url);
    }
}

void BrowserController::goBack()
{
    if (BrowserTab* tab = currentTab(); tab && tab->history()->canGoBack()) {
        tab->back();
    }
}

void BrowserController::goForward()
{
    if (BrowserTab* tab = currentTab(); tab && tab->history()->canGoForward()) {
        tab->forward();
    }
}

void BrowserController::reload()
{
    if (BrowserTab* tab = currentTab()) {
        tab->reload();
    }
}

void BrowserController::stop()
{
    if (BrowserTab* tab = currentTab()) {
        tab->stop();
    }
}

void BrowserController::goHome()
{
    navigateCurrentTab(QUrl(m_settings.homepage()));
}

void BrowserController::toggleBookmarkForCurrentTab()
{
    BrowserTab* tab = currentTab();
    if (!tab) {
        return;
    }

    const QUrl url = tab->url();
    if (!UrlUtils::isRecordable(url)) {
        emit statusMessage(tr("This page cannot be bookmarked"));
        return;
    }

    const QString urlString = url.toString();
    if (m_bookmarks.isBookmarked(urlString)) {
        m_bookmarks.removeByUrl(urlString);
        emit statusMessage(tr("Bookmark removed"));
    } else {
        const QList<BookmarkFolder> folders = m_bookmarks.folders();
        const qint64 folderId = folders.isEmpty() ? -1 : folders.first().id;
        m_bookmarks.addBookmark(tab->displayTitle(), urlString, folderId);
        emit statusMessage(tr("Bookmark added"));
    }

    updateBookmarkState();
}

BrowserTab* BrowserController::makeTab()
{
    auto* tab = new BrowserTab(m_profile, m_tabWidget);
    tab->setWindowFactory([this] { return createTab(QUrl(), true); });
    tab->setJavascriptEnabled(m_settings.javascriptEnabled());
    tab->page()->settings()->setAttribute(
        QWebEngineSettings::AllowRunningInsecureContent,
        !m_settings.blockInsecureContent());
    wireTab(tab);

    m_tabs.append(tab);
    return tab;
}

void BrowserController::wireTab(BrowserTab* tab)
{
    connect(tab, &QWebEngineView::titleChanged, this, [this, tab](const QString&) {
        const int index = m_tabWidget->indexOf(tab);
        if (index < 0) {
            return;
        }

        m_tabWidget->setTabText(index, tab->displayTitle());
        m_tabWidget->setTabToolTip(index, tab->url().toString());

        if (tab == currentTab()) {
            emit currentTabChanged(tab);
        }
    });

    connect(tab, &QWebEngineView::urlChanged, this, [this, tab](const QUrl&) {
        if (tab != currentTab()) {
            return;
        }
        updateNavigationState();
        updateBookmarkState();
        emit currentTabChanged(tab);
    });

    connect(tab, &QWebEngineView::iconChanged, this, [this, tab](const QIcon& icon) {
        const int index = m_tabWidget->indexOf(tab);
        if (index >= 0 && !icon.isNull()) {
            m_tabWidget->setTabIcon(index, icon);
        }
    });

    connect(tab, &QWebEngineView::loadStarted, this, [this, tab] {
        if (tab == currentTab()) {
            emit loadingChanged(true);
            emit loadProgressChanged(0);
        }
    });

    connect(tab, &QWebEngineView::loadProgress, this, [this, tab](int progress) {
        if (tab == currentTab()) {
            emit loadProgressChanged(progress);
        }
    });

    connect(tab, &QWebEngineView::loadFinished, this, [this, tab](bool ok) {
        if (tab == currentTab()) {
            emit loadingChanged(false);
            emit loadProgressChanged(100);
            updateNavigationState();
            updateBookmarkState();
        }

        if (ok) {
            recordHistory(tab);
            m_web3.probe(tab->page());
            if (tab == currentTab()) {
                emit web3StatusChanged(m_web3.isDetected(tab->page()));
            }
        }
    });

    connect(tab->page(), &QWebEnginePage::linkHovered, this,
            [this, tab](const QString& link) {
                if (tab == currentTab()) {
                    emit statusMessage(link);
                }
            });

    connect(tab->page(), &QWebEnginePage::windowCloseRequested, this, [this, tab] {
        closeTab(m_tabWidget->indexOf(tab));
    });
}

void BrowserController::updateNavigationState()
{
    BrowserTab* tab = currentTab();
    const bool hasTab = tab != nullptr;

    m_backAction->setEnabled(hasTab && tab->history()->canGoBack());
    m_forwardAction->setEnabled(hasTab && tab->history()->canGoForward());
    m_reloadAction->setEnabled(hasTab);
    m_stopAction->setEnabled(hasTab);
    m_duplicateTabAction->setEnabled(hasTab);
    m_closeTabAction->setEnabled(hasTab);
    m_bookmarkAction->setEnabled(hasTab && UrlUtils::isRecordable(tab->url()));
}

void BrowserController::updateBookmarkState()
{
    BrowserTab* tab = currentTab();
    emit bookmarkStateChanged(tab && m_bookmarks.isBookmarked(tab->url().toString()));
}

void BrowserController::onCurrentTabChanged(int index)
{
    Q_UNUSED(index);

    BrowserTab* tab = currentTab();

    updateNavigationState();
    updateBookmarkState();

    if (tab) {
        emit web3StatusChanged(m_web3.isDetected(tab->page()));
        emit loadingChanged(tab->page()->isLoading());
    }

    emit currentTabChanged(tab);
}

void BrowserController::recordHistory(BrowserTab* tab)
{
    if (!tab || m_privateMode) {
        return;
    }

    const QUrl url = tab->url();
    if (!UrlUtils::isRecordable(url) || UrlUtils::isNewTabPage(url)) {
        return;
    }

    m_history.addVisit(url.toString(), tab->displayTitle());
}

QAction* BrowserController::makeAction(const QString& text,
                                       const QList<QKeySequence>& shortcuts,
                                       const QString& iconPath)
{
    auto* action = new QAction(text, m_tabWidget);
    if (!iconPath.isEmpty()) {
        action->setIcon(QIcon(iconPath));
    }
    if (!shortcuts.isEmpty()) {
        action->setShortcuts(shortcuts);
    }
    m_tabWidget->addAction(action);
    return action;
}

} // namespace chainx
