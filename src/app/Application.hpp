#pragma once

#include <QApplication>
#include <QList>
#include <QString>

#include <memory>

class QWebEngineProfile;

namespace chainx {

class BookmarkService;
class BrowserWindow;
class Database;
class DownloadService;
class HistoryService;
class Settings;
class Web3Provider;

/**
 * Application entry point: owns persistent services, the WebEngine profile and
 * the browser windows.
 */
class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int& argc, char** argv);
    ~Application() override;

    static int run(int argc, char** argv);

    BrowserWindow* createWindow(bool privateMode = false);
    QWebEngineProfile* profile() const;

private:
    bool initialize();
    void openInitialWindow(bool privateMode);
    static QString dataDirectory();

    std::unique_ptr<Database> m_database;
    std::unique_ptr<Settings> m_settings;
    std::unique_ptr<HistoryService> m_history;
    std::unique_ptr<BookmarkService> m_bookmarks;
    std::unique_ptr<DownloadService> m_downloads;
    std::unique_ptr<Web3Provider> m_web3;

    QWebEngineProfile* m_profile = nullptr;
    QList<QWebEngineProfile*> m_privateProfiles;
    QList<BrowserWindow*> m_windows;
};

} // namespace chainx
