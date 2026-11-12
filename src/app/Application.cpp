#include "app/Application.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QStandardPaths>
#include <QWebEngineProfile>
#include <QtAlgorithms>

#include "browser/BrowserWindow.hpp"
#include "database/Database.hpp"
#include "services/BookmarkService.hpp"
#include "services/DownloadService.hpp"
#include "services/HistoryService.hpp"
#include "settings/Settings.hpp"
#include "utils/Logger.hpp"
#include "web3/Web3Provider.hpp"

#ifndef CHAINX_VERSION
#define CHAINX_VERSION "1.0.0"
#endif

namespace chainx {

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv)
{
}

Application::~Application()
{
    const QList<BrowserWindow*> windows = m_windows;
    m_windows.clear();
    qDeleteAll(windows);

    Logger::shutdown();
}

int Application::run(int argc, char** argv)
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    Application app(argc, argv);
    if (!app.initialize()) {
        return 1;
    }

    const bool privateWindow =
        QCoreApplication::arguments().contains(QStringLiteral("--private"));
    app.openInitialWindow(privateWindow);
    return app.exec();
}

BrowserWindow* Application::createWindow(bool privateMode)
{
    QWebEngineProfile* profile = m_profile;
    if (privateMode) {
        // No storage name means an off-the-record profile: cookies, cache and
        // history live in memory and disappear when the last tab closes.
        profile = new QWebEngineProfile(this);
        profile->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
        profile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
        profile->setDownloadPath(m_settings->downloadDirectory());
        m_privateProfiles.append(profile);
        m_downloads->attachProfile(profile, false);
    }

    auto* window = new BrowserWindow(*m_settings,
                                     *m_history,
                                     *m_bookmarks,
                                     *m_downloads,
                                     *m_web3,
                                     profile,
                                     privateMode);
    m_windows.append(window);

    connect(window, &BrowserWindow::newWindowRequested,
            this, [this](bool wantsPrivate) { createWindow(wantsPrivate); });

    connect(window, &QObject::destroyed, this,
            [this, window, profile, privateMode] {
                m_windows.removeAll(window);
                if (privateMode && profile) {
                    m_privateProfiles.removeAll(profile);
                    profile->deleteLater();
                }
            });

    window->show();
    return window;
}

QWebEngineProfile* Application::profile() const
{
    return m_profile;
}

bool Application::initialize()
{
    setApplicationName(QStringLiteral("ChainX"));
    setApplicationDisplayName(QStringLiteral("ChainX Browser"));
    setOrganizationName(QStringLiteral("ChainX"));
    setOrganizationDomain(QStringLiteral("chainx.local"));
    setApplicationVersion(QStringLiteral(CHAINX_VERSION));
    setDesktopFileName(QStringLiteral("chainx-browser"));

    const QString dataDir = dataDirectory();
    Logger::install(dataDir + QStringLiteral("/chainx.log"));
    qInfo() << "Starting ChainX Browser" << CHAINX_VERSION;

    m_database = std::make_unique<Database>(dataDir + QStringLiteral("/chainx.db"));
    if (!m_database->open()) {
        QMessageBox::critical(
            nullptr,
            tr("ChainX"),
            tr("Unable to open the browser database.\n\n%1").arg(m_database->lastError()));
        return false;
    }

    m_settings = std::make_unique<Settings>();
    m_history = std::make_unique<HistoryService>(*m_database);
    m_bookmarks = std::make_unique<BookmarkService>(*m_database);
    m_downloads = std::make_unique<DownloadService>(*m_database, *m_settings);
    m_web3 = std::make_unique<Web3Provider>();

    const int reconciled = m_downloads->reconcileInterrupted();
    if (reconciled > 0) {
        qInfo() << "Marked" << reconciled << "stale downloads as interrupted";
    }

    const int retentionDays = m_settings->historyRetentionDays();
    if (retentionDays > 0) {
        m_history->pruneOlderThan(retentionDays);
    }

    const QString profileDir = dataDir + QStringLiteral("/profile");
    QDir().mkpath(profileDir);

    m_profile = new QWebEngineProfile(QStringLiteral("chainx"), this);
    m_profile->setPersistentStoragePath(profileDir);
    m_profile->setCachePath(profileDir + QStringLiteral("/cache"));
    m_profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
    m_profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);

    m_downloads->attachProfile(m_profile);

    QFile styleFile(QStringLiteral(":/styles/chainx.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    return true;
}

void Application::openInitialWindow(bool privateMode)
{
    createWindow(privateMode);
}

QString Application::dataDirectory()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (path.isEmpty()) {
        path = QDir::homePath() + QStringLiteral("/.chainx");
    }
    QDir().mkpath(path);
    return path;
}

} // namespace chainx
