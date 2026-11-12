#pragma once

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

#include <QWebEngineDownloadRequest>

class QWebEngineProfile;

namespace chainx {

class Database;
class Settings;

struct DownloadRecord
{
    qint64 id = -1;
    QString filename;
    QString url;
    QString filePath;
    qint64 totalSize = 0;
    qint64 downloadedSize = 0;
    QString status;
    QDateTime createdAt;
};

/**
 * Handles downloads requested by Qt WebEngine and records them in SQLite.
 */
class DownloadService : public QObject
{
    Q_OBJECT

public:
    DownloadService(Database& database, Settings& settings, QObject* parent = nullptr);

    /// Starts listening for download requests from the given profile.
    void attachProfile(QWebEngineProfile* profile);

    QString downloadDirectory() const;
    void setDownloadDirectory(const QString& directory);

    QList<DownloadRecord> downloads(int limit = 100) const;

    bool openFile(qint64 id) const;
    bool showInFolder(qint64 id) const;

    /// Marks downloads that were left running by a previous session as
    /// interrupted. Called once at startup.
    int reconcileInterrupted();

    static QString statusToString(QWebEngineDownloadRequest::DownloadState state);

public slots:
    void handleDownload(QWebEngineDownloadRequest* request);
    void cancelDownload(qint64 id);
    void pauseDownload(qint64 id);
    void resumeDownload(qint64 id);

signals:
    void downloadStarted(qint64 id, const QString& filename);
    void downloadProgress(qint64 id, qint64 received, qint64 total);
    void downloadFinished(qint64 id);
    void downloadsChanged();

private:
    qint64 insertRecord(const QString& filename,
                        const QString& url,
                        const QString& filePath,
                        qint64 totalSize,
                        qint64 downloadedSize,
                        const QString& status);
    void updateRecord(qint64 id,
                      qint64 totalSize,
                      qint64 downloadedSize,
                      const QString& status,
                      const QString& filePath);
    void finishDownload(qint64 id, QWebEngineDownloadRequest* request);

    Database& m_database;
    Settings& m_settings;
    QHash<QWebEngineDownloadRequest*, qint64> m_idsByRequest;
    QHash<qint64, QWebEngineDownloadRequest*> m_requestsById;
};

} // namespace chainx
