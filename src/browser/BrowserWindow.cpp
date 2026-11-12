#include "browser/BrowserWindow.hpp"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QSignalBlocker>
#include <QSize>
#include <QSizePolicy>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <QWebEngineProfile>

#include "browser/BrowserController.hpp"
#include "browser/BrowserTab.hpp"
#include "browser/BookmarkManagerDialog.hpp"
#include "browser/HistoryDialog.hpp"
#include "browser/PrivacyDialog.hpp"
#include "browser/SettingsDialog.hpp"
#include "navigation/AddressBar.hpp"
#include "services/BookmarkService.hpp"
#include "services/DownloadService.hpp"
#include "services/HistoryService.hpp"
#include "settings/Settings.hpp"
#include "utils/UrlUtils.hpp"
#include "web3/Web3Provider.hpp"

#ifndef CHAINX_VERSION
#define CHAINX_VERSION "1.0.0"
#endif

namespace chainx {
namespace {

constexpr auto kGeometryKey = "window/geometry";
constexpr auto kStateKey = "window/state";
constexpr auto kBookmarkBarKey = "ui/bookmarkToolbarVisible";

QIcon brandIcon()
{
    QIcon icon;
    for (int size : {16, 24, 32, 48, 64, 128, 256}) {
        icon.addFile(QStringLiteral(":/icons/branding/chainx-%1.png").arg(size),
                     QSize(size, size));
    }
    return icon;
}

} // namespace

BrowserWindow::BrowserWindow(Settings& settings,
                             HistoryService& history,
                             BookmarkService& bookmarks,
                             DownloadService& downloads,
                             Web3Provider& web3,
                             QWebEngineProfile* profile,
                             bool privateMode,
                             QWidget* parent)
    : QMainWindow(parent)
    , m_settings(settings)
    , m_history(history)
    , m_bookmarks(bookmarks)
    , m_downloads(downloads)
    , m_web3(web3)
    , m_profile(profile)
    , m_privateMode(privateMode)
{
    setWindowTitle(m_privateMode ? tr("ChainX (Private)") : QStringLiteral("ChainX"));
    setWindowIcon(brandIcon());
    resize(1280, 820);
    setMinimumSize(640, 480);

    m_controller = new BrowserController(settings, history, bookmarks, downloads,
                                         web3, profile, privateMode, this);

    buildStatusBar();
    buildUi();
    buildToolBars();
    buildMenus();

    const QByteArray geometry = m_settings.value(QString::fromLatin1(kGeometryKey)).toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }

    const QByteArray state = m_settings.value(QString::fromLatin1(kStateKey)).toByteArray();
    if (!state.isEmpty()) {
        restoreState(state);
    }

    setBookmarkToolBarVisible(
        m_settings.value(QString::fromLatin1(kBookmarkBarKey), true).toBool());

    m_controller->createTab();
}

BrowserController* BrowserWindow::controller() const
{
    return m_controller;
}

void BrowserWindow::closeEvent(QCloseEvent* event)
{
    m_settings.setValue(QString::fromLatin1(kGeometryKey), saveGeometry());
    m_settings.setValue(QString::fromLatin1(kStateKey), saveState());
    m_settings.sync();
    QMainWindow::closeEvent(event);
}

void BrowserWindow::buildUi()
{
    setCentralWidget(m_controller->tabWidget());

    m_addressBar = new AddressBar(this);
    m_addressBar->setSearchTemplate(m_settings.searchEngineTemplate());
    m_addressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_addressBar->setMinimumWidth(280);
    m_addressBar->setSuggestionProvider([this](const QString& text) {
        QStringList results = m_history.suggestions(text, 6);
        for (const QString& url : m_bookmarks.suggestions(text, 4)) {
            if (!results.contains(url)) {
                results.append(url);
            }
        }
        while (results.size() > 8) {
            results.removeLast();
        }
        return results;
    });

    connect(m_addressBar, &AddressBar::navigateRequested,
            m_controller, &BrowserController::navigateCurrentTab);
    connect(m_controller, &BrowserController::focusAddressBarRequested,
            m_addressBar, &AddressBar::focusAndSelectAll);
    connect(m_controller, &BrowserController::currentTabChanged,
            this, &BrowserWindow::updateForCurrentTab);
    connect(m_controller, &BrowserController::loadingChanged,
            this, &BrowserWindow::updateLoadingState);
    connect(m_controller, &BrowserController::loadProgressChanged,
            this, [this](int progress) { m_progressBar->setValue(progress); });
    connect(m_controller, &BrowserController::statusMessage,
            this, [this](const QString& message) { m_statusLabel->setText(message); });
    connect(m_controller, &BrowserController::bookmarkStateChanged,
            this, &BrowserWindow::updateBookmarkState);
    connect(m_controller, &BrowserController::web3StatusChanged,
            this, &BrowserWindow::updateWeb3Badge);

    connect(&m_bookmarks, &BookmarkService::bookmarksChanged,
            this, &BrowserWindow::rebuildBookmarkToolBar);
    connect(&m_bookmarks, &BookmarkService::bookmarksChanged,
            this, &BrowserWindow::rebuildBookmarkMenu);
    connect(&m_downloads, &DownloadService::downloadsChanged,
            this, &BrowserWindow::rebuildDownloadsMenu);
    connect(&m_downloads, &DownloadService::downloadStarted,
            this, [this](qint64, const QString& name) {
                m_statusLabel->setText(tr("Downloading %1…").arg(name));
            });
    connect(&m_downloads, &DownloadService::downloadFinished,
            this, [this](qint64) { m_statusLabel->setText(tr("Download finished")); });
}

void BrowserWindow::buildToolBars()
{
    m_mainToolBar = addToolBar(tr("Navigation"));
    m_mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
    m_mainToolBar->setMovable(false);
    m_mainToolBar->setFloatable(false);
    m_mainToolBar->setIconSize(QSize(18, 18));
    m_mainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    m_mainToolBar->addAction(m_controller->backAction());
    m_mainToolBar->addAction(m_controller->forwardAction());
    m_mainToolBar->addAction(m_controller->reloadAction());
    m_mainToolBar->addAction(m_controller->stopAction());
    m_mainToolBar->addAction(m_controller->homeAction());
    m_mainToolBar->addSeparator();

    m_mainToolBar->addWidget(m_addressBar);

    m_mainToolBar->addAction(m_controller->bookmarkAction());

    m_downloadsButton = new QToolButton(this);
    m_downloadsButton->setIcon(iconFor(QStringLiteral(":/icons/download.svg"),
                                       QStyle::SP_ArrowDown));
    m_downloadsButton->setToolTip(tr("Downloads"));
    m_downloadsButton->setAutoRaise(true);
    m_downloadsButton->setPopupMode(QToolButton::InstantPopup);
    m_downloadsMenu = new QMenu(m_downloadsButton);
    m_downloadsButton->setMenu(m_downloadsMenu);
    connect(m_downloadsMenu, &QMenu::aboutToShow,
            this, &BrowserWindow::rebuildDownloadsMenu);
    m_mainToolBar->addWidget(m_downloadsButton);

    m_historyAction = new QAction(iconFor(QStringLiteral(":/icons/history.svg"),
                                          QStyle::SP_FileDialogDetailedView),
                                  tr("History"), this);
    m_historyAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+H")));
    addAction(m_historyAction);
    connect(m_historyAction, &QAction::triggered,
            this, &BrowserWindow::showHistoryDialog);
    m_mainToolBar->addAction(m_historyAction);

    m_bookmarkToolBar = new QToolBar(tr("Bookmarks"), this);
    m_bookmarkToolBar->setObjectName(QStringLiteral("bookmarkToolBar"));
    m_bookmarkToolBar->setMovable(false);
    m_bookmarkToolBar->setFloatable(false);
    m_bookmarkToolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_bookmarkToolBar->setIconSize(QSize(16, 16));

    addToolBarBreak(Qt::TopToolBarArea);
    addToolBar(Qt::TopToolBarArea, m_bookmarkToolBar);

    rebuildBookmarkToolBar();
}

void BrowserWindow::buildMenus()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    QAction* newWindowAction = fileMenu->addAction(tr("New &Window"));
    newWindowAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+N")));
    connect(newWindowAction, &QAction::triggered, this,
            [this] { emit newWindowRequested(false); });

    QAction* newPrivateWindowAction = fileMenu->addAction(tr("New &Private Window"));
    newPrivateWindowAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+N")));
    connect(newPrivateWindowAction, &QAction::triggered, this,
            [this] { emit newWindowRequested(true); });

    fileMenu->addSeparator();
    fileMenu->addAction(m_controller->newTabAction());
    fileMenu->addAction(m_controller->closeTabAction());
    fileMenu->addAction(m_controller->duplicateTabAction());
    fileMenu->addAction(m_controller->restoreTabAction());
    fileMenu->addSeparator();

    QAction* quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Q")));
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    QAction* settingsAction = fileMenu->addAction(tr("&Settings…"));
    settingsAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+,")));
    connect(settingsAction, &QAction::triggered, this, &BrowserWindow::showSettings);
    fileMenu->insertAction(quitAction, settingsAction);
    fileMenu->insertSeparator(quitAction);

    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(m_controller->backAction());
    viewMenu->addAction(m_controller->forwardAction());
    viewMenu->addAction(m_controller->reloadAction());
    viewMenu->addAction(m_controller->stopAction());
    viewMenu->addAction(m_controller->homeAction());
    viewMenu->addSeparator();

    m_zoomInAction = viewMenu->addAction(tr("Zoom &In"));
    m_zoomInAction->setShortcuts({QKeySequence(QStringLiteral("Ctrl+=")),
                                  QKeySequence(QStringLiteral("Ctrl++"))});
    connect(m_zoomInAction, &QAction::triggered, this, &BrowserWindow::zoomIn);

    m_zoomOutAction = viewMenu->addAction(tr("Zoom &Out"));
    m_zoomOutAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+-")));
    connect(m_zoomOutAction, &QAction::triggered, this, &BrowserWindow::zoomOut);

    m_zoomResetAction = viewMenu->addAction(tr("&Reset Zoom"));
    m_zoomResetAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));
    connect(m_zoomResetAction, &QAction::triggered, this, &BrowserWindow::zoomReset);

    viewMenu->addSeparator();

    m_bookmarkToolBarAction = viewMenu->addAction(tr("Show &Bookmarks Bar"));
    m_bookmarkToolBarAction->setCheckable(true);
    m_bookmarkToolBarAction->setChecked(true);
    connect(m_bookmarkToolBarAction, &QAction::toggled,
            this, &BrowserWindow::setBookmarkToolBarVisible);

    m_fullScreenAction = viewMenu->addAction(tr("&Full Screen"));
    m_fullScreenAction->setShortcut(QKeySequence(QStringLiteral("F11")));
    m_fullScreenAction->setCheckable(true);
    connect(m_fullScreenAction, &QAction::toggled,
            this, [this](bool enabled) {
                if (enabled) {
                    showFullScreen();
                } else {
                    showNormal();
                }
            });

    m_bookmarksMenu = menuBar()->addMenu(tr("&Bookmarks"));
    m_bookmarksMenu->addAction(m_controller->bookmarkAction());
    QAction* manageBookmarksAction = m_bookmarksMenu->addAction(tr("&Manage Bookmarks…"));
    manageBookmarksAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+O")));
    connect(manageBookmarksAction, &QAction::triggered,
            this, &BrowserWindow::showBookmarkManager);
    m_bookmarksMenu->addAction(m_bookmarkToolBarAction);
    m_bookmarksMenu->addSeparator();

    m_allBookmarksMenu = m_bookmarksMenu->addMenu(tr("&All Bookmarks"));
    connect(m_allBookmarksMenu, &QMenu::aboutToShow,
            this, &BrowserWindow::rebuildBookmarkMenu);

    QMenu* historyMenu = menuBar()->addMenu(tr("&History"));
    historyMenu->addAction(m_historyAction);
    QAction* clearHistoryAction = historyMenu->addAction(tr("&Clear Browsing History…"));
    connect(clearHistoryAction, &QAction::triggered, this, &BrowserWindow::clearHistory);

    QAction* clearDataAction = historyMenu->addAction(tr("Clear Browsing &Data…"));
    clearDataAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+Delete")));
    connect(clearDataAction, &QAction::triggered,
            this, &BrowserWindow::showPrivacyDialog);

    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction* aboutAction = helpMenu->addAction(tr("&About ChainX"));
    connect(aboutAction, &QAction::triggered, this, &BrowserWindow::showAbout);

    rebuildBookmarkMenu();
}

void BrowserWindow::buildStatusBar()
{
    m_statusLabel = new QLabel(this);
    m_statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statusBar()->addWidget(m_statusLabel, 1);

    if (m_privateMode) {
        m_privateBadge = new QLabel(tr("Private"), this);
        m_privateBadge->setStyleSheet(QStringLiteral(
            "QLabel { background: #eef0f6; color: #4a5162; border-radius: 8px;"
            " padding: 2px 8px; font-weight: 600; font-size: 11px; }"));
        m_privateBadge->setToolTip(
            tr("This window does not save history, cookies or cache"));
        statusBar()->addPermanentWidget(m_privateBadge);
    }

    m_web3Label = new QLabel(tr("Web3"), this);
    m_web3Label->setStyleSheet(QStringLiteral(
        "QLabel { background: #e7f7ee; color: #1c7a4a; border-radius: 8px;"
        " padding: 2px 8px; font-weight: 600; font-size: 11px; }"));
    m_web3Label->setToolTip(tr("This page exposes a Web3 provider (window.ethereum)"));
    m_web3Label->hide();
    statusBar()->addPermanentWidget(m_web3Label);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedWidth(160);
    m_progressBar->hide();
    statusBar()->addPermanentWidget(m_progressBar);

    statusBar()->setSizeGripEnabled(false);
}

void BrowserWindow::updateForCurrentTab(BrowserTab* tab)
{
    const QString suffix = m_privateMode ? tr("ChainX (Private)") : tr("ChainX");

    if (!tab) {
        m_addressBar->setUrl(QUrl());
        setWindowTitle(suffix);
        updateLoadingState(false);
        return;
    }

    m_addressBar->setUrl(tab->url());
    setWindowTitle(tr("%1 — %2").arg(tab->displayTitle(), suffix));
    updateLoadingState(tab->page()->isLoading());
}

void BrowserWindow::updateLoadingState(bool loading)
{
    if (!m_mainToolBar) {
        return;
    }

    if (QWidget* reloadButton =
            m_mainToolBar->widgetForAction(m_controller->reloadAction())) {
        reloadButton->setVisible(!loading);
    }
    if (QWidget* stopButton =
            m_mainToolBar->widgetForAction(m_controller->stopAction())) {
        stopButton->setVisible(loading);
    }

    m_progressBar->setVisible(loading);
    if (!loading) {
        m_progressBar->setValue(0);
    }
}

void BrowserWindow::updateBookmarkState(bool bookmarked)
{
    QAction* action = m_controller->bookmarkAction();
    action->setIcon(iconFor(bookmarked ? QStringLiteral(":/icons/star-filled.svg")
                                       : QStringLiteral(":/icons/star.svg"),
                            bookmarked ? QStyle::SP_DialogApplyButton
                                       : QStyle::SP_DialogResetButton));
    action->setText(bookmarked ? tr("Remove Bookmark") : tr("Bookmark This Page"));
}

void BrowserWindow::updateWeb3Badge(bool detected)
{
    m_web3Label->setVisible(detected);
}

void BrowserWindow::showHistoryDialog()
{
    HistoryDialog dialog(m_history, this);
    connect(&dialog, &HistoryDialog::openRequested,
            m_controller, &BrowserController::navigateCurrentTab);
    dialog.exec();
}

void BrowserWindow::showBookmarkManager()
{
    BookmarkManagerDialog dialog(m_bookmarks, this);
    connect(&dialog, &BookmarkManagerDialog::openRequested,
            m_controller, &BrowserController::navigateCurrentTab);
    dialog.exec();
}

void BrowserWindow::showPrivacyDialog()
{
    PrivacyDialog dialog(m_profile, m_history, m_downloads, this);
    connect(&dialog, &PrivacyDialog::dataCleared, this, [this] {
        m_statusLabel->setText(tr("Browsing data cleared"));
    });
    dialog.exec();
}

void BrowserWindow::showSettings()
{
    SettingsDialog dialog(m_settings, this);
    connect(&dialog, &SettingsDialog::settingsChanged, this, [this] {
        m_addressBar->setSearchTemplate(m_settings.searchEngineTemplate());
        setBookmarkToolBarVisible(
            m_settings.value(QStringLiteral("ui/bookmarkToolbarVisible"), true).toBool());
        m_controller->applySettings();
        m_statusLabel->setText(tr("Settings saved"));
    });
    dialog.exec();
}

void BrowserWindow::rebuildBookmarkMenu()
{
    if (!m_allBookmarksMenu) {
        return;
    }

    m_allBookmarksMenu->clear();

    const QList<Bookmark> bookmarks = m_bookmarks.bookmarks();
    if (bookmarks.isEmpty()) {
        QAction* empty = m_allBookmarksMenu->addAction(tr("No bookmarks yet"));
        empty->setEnabled(false);
        return;
    }

    for (const Bookmark& bookmark : bookmarks) {
        QAction* action = m_allBookmarksMenu->addAction(bookmark.title);
        action->setToolTip(bookmark.url);
        const QString url = bookmark.url;
        connect(action, &QAction::triggered, this, [this, url] {
            m_controller->navigateCurrentTab(QUrl(url));
        });
    }
}

void BrowserWindow::rebuildBookmarkToolBar()
{
    if (!m_bookmarkToolBar) {
        return;
    }

    m_bookmarkToolBar->clear();

    const QList<Bookmark> bookmarks = m_bookmarks.bookmarks();
    if (bookmarks.isEmpty()) {
        QAction* hint =
            m_bookmarkToolBar->addAction(tr("No bookmarks yet — press Ctrl+D to add one"));
        hint->setEnabled(false);
        return;
    }

    for (const Bookmark& bookmark : bookmarks) {
        QAction* action = m_bookmarkToolBar->addAction(bookmark.title);
        action->setToolTip(bookmark.url);
        const QString url = bookmark.url;
        connect(action, &QAction::triggered, this, [this, url] {
            m_controller->navigateCurrentTab(QUrl(url));
        });
    }
}

void BrowserWindow::rebuildDownloadsMenu()
{
    if (!m_downloadsMenu) {
        return;
    }

    m_downloadsMenu->clear();

    const QList<DownloadRecord> records = m_downloads.downloads(8);
    if (records.isEmpty()) {
        QAction* empty = m_downloadsMenu->addAction(tr("No downloads yet"));
        empty->setEnabled(false);
    } else {
        for (const DownloadRecord& record : records) {
            QString label = record.filename;
            if (record.status == QLatin1String("in_progress")
                || record.status == QLatin1String("requested")) {
                const int percent =
                    record.totalSize > 0
                        ? static_cast<int>(record.downloadedSize * 100 / record.totalSize)
                        : 0;
                label += tr(" — %1%").arg(percent);
            } else {
                label += QStringLiteral(" — ") + record.status;
            }

            QAction* action = m_downloadsMenu->addAction(label);
            action->setToolTip(record.url);
            const qint64 id = record.id;
            connect(action, &QAction::triggered, this, [this, id] {
                if (!m_downloads.openFile(id)) {
                    m_downloads.showInFolder(id);
                }
            });
        }
    }

    m_downloadsMenu->addSeparator();
    QAction* openFolder = m_downloadsMenu->addAction(tr("Open Downloads Folder"));
    connect(openFolder, &QAction::triggered, this, [this] {
        const QString directory = m_downloads.downloadDirectory();
        if (!directory.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(directory));
        }
    });
}

void BrowserWindow::setBookmarkToolBarVisible(bool visible)
{
    if (m_bookmarkToolBar) {
        m_bookmarkToolBar->setVisible(visible);
    }

    if (m_bookmarkToolBarAction) {
        const QSignalBlocker blocker(m_bookmarkToolBarAction);
        m_bookmarkToolBarAction->setChecked(visible);
    }

    m_settings.setValue(QString::fromLatin1(kBookmarkBarKey), visible);
}

void BrowserWindow::clearHistory()
{
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        tr("Clear Browsing History"),
        tr("Delete all browsing history? This cannot be undone."),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (answer == QMessageBox::Yes) {
        m_history.clear();
        m_statusLabel->setText(tr("Browsing history cleared"));
    }
}

void BrowserWindow::showAbout()
{
    QMessageBox box(this);
    box.setWindowTitle(tr("About ChainX"));
    box.setIconPixmap(brandIcon().pixmap(96, 96));
    box.setTextFormat(Qt::RichText);
    box.setText(
        tr("<h3>ChainX Browser</h3>"
           "<p>Version %1</p>"
           "<p>A lightweight, privacy-focused browser built with C++20 and Qt 6, "
           "powered by Chromium through Qt WebEngine.</p>"
           "<p>Web3 provider integration is disabled until the security model "
           "has been validated.</p>")
            .arg(QStringLiteral(CHAINX_VERSION)));
    box.exec();
}

void BrowserWindow::zoomIn()
{
    if (BrowserTab* tab = m_controller->currentTab()) {
        tab->setZoomFactor(qMin(5.0, tab->zoomFactor() + 0.1));
    }
}

void BrowserWindow::zoomOut()
{
    if (BrowserTab* tab = m_controller->currentTab()) {
        tab->setZoomFactor(qMax(0.25, tab->zoomFactor() - 0.1));
    }
}

void BrowserWindow::zoomReset()
{
    if (BrowserTab* tab = m_controller->currentTab()) {
        tab->setZoomFactor(1.0);
    }
}

QIcon BrowserWindow::iconFor(const QString& resourcePath,
                             QStyle::StandardPixmap fallback) const
{
    const QIcon icon(resourcePath);
    if (!icon.isNull()) {
        return icon;
    }
    return style()->standardIcon(fallback);
}

} // namespace chainx
