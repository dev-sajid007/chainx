#include "services/DownloadService.hpp"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <QVariant>
#include <QWebEngineProfile>

#include "database/Database.hpp"
#include "settings/Settings.hpp"
#include "utils/Logger.hpp"

namespace chainx {
namespace {

QString uniquePath(const QString& directory, const QString& fileName)
{
    const QFileInfo info(fileName);
    const QString base = info.completeBaseName().isEmpty() ? QStringLiteral("download")
                                                           : info.completeBaseName();
    const QString suffix = info.suffix();

    QString candidate = QDir(directory).filePath(fileName);
    int counter = 1;
    while (QFileInfo::exists(candidate)) {
        const QString name =
            suffix.isEmpty()
                ? QStringLiteral("%1 (%2)").arg(base).arg(counter)
                : QStringLiteral("%1 (%2).%3").arg(base).arg(counter).arg(suffix);
        candidate = QDir(directory).filePath(name);
        ++counter;
    }
    return candidate;
}

bool isFinalState(QWebEngineDownloadRequest::DownloadState state)
{
    return state == QWebEngineDownloadRequest::DownloadCompleted
           || state == QWebEngineDownloadRequest::DownloadCancelled
           || state == QWebEngineDownloadRequest::DownloadInterrupted;
}

} // namespace

DownloadService::DownloadService(Database& database, Settings& settings, QObject* parent)
    : QObject(parent)
    , m_database(database)
    , m_settings(settings)
{
}

void DownloadService::attachProfile(QWebEngineProfile* profile)
{
    if (!profile) {
        return;
    }

    profile->setDownloadPath(downloadDirectory());
    connect(profile, &QWebEngineProfile::downloadRequested,
            this, &DownloadService::handleDownload);
}

QString DownloadService::downloadDirectory() const
{
    return m_settings.downloadDirectory();
}

void DownloadService::setDownloadDirectory(const QString& directory)
{
    m_settings.setDownloadDirectory(directory);
    m_settings.sync();
}

QList<DownloadRecord> DownloadService::downloads(int limit) const
{
    QList<DownloadRecord> records;

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "SELECT id, filename, url, file_path, total_size, downloaded_size, status, created_at "
        "FROM downloads ORDER BY created_at DESC, id DESC LIMIT ?"));
    query.addBindValue(qMax(1, limit));

    if (!query.exec()) {
        qWarning() << "Failed to read downloads:" << query.lastError().text();
        return records;
    }

    while (query.next()) {
        DownloadRecord record;
        record.id = query.value(0).toLongLong();
        record.filename = query.value(1).toString();
        record.url = query.value(2).toString();
        record.filePath = query.value(3).toString();
        record.totalSize = query.value(4).toLongLong();
        record.downloadedSize = query.value(5).toLongLong();
        record.status = query.value(6).toString();
        record.createdAt = QDateTime::fromSecsSinceEpoch(query.value(7).toLongLong());
        records.append(record);
    }

    return records;
}

bool DownloadService::openFile(qint64 id) const
{
    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral("SELECT file_path FROM downloads WHERE id = ?"));
    query.addBindValue(id);

    if (!query.exec() || !query.next()) {
        return false;
    }

    const QString path = query.value(0).toString();
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return false;
    }
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

bool DownloadService::showInFolder(qint64 id) const
{
    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral("SELECT file_path FROM downloads WHERE id = ?"));
    query.addBindValue(id);

    if (!query.exec() || !query.next()) {
        return false;
    }

    const QString path = query.value(0).toString();
    if (path.isEmpty()) {
        return false;
    }
    return QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
}

int DownloadService::reconcileInterrupted()
{
    QSqlQuery query(m_database.connection());
    if (!query.exec(QStringLiteral(
            "UPDATE downloads SET status = 'interrupted' "
            "WHERE status IN ('requested', 'in_progress')"))) {
        qWarning() << "Failed to reconcile downloads:" << query.lastError().text();
        return 0;
    }
    return query.numRowsAffected();
}

QString DownloadService::statusToString(QWebEngineDownloadRequest::DownloadState state)
{
    switch (state) {
    case QWebEngineDownloadRequest::DownloadRequested:
        return QStringLiteral("requested");
    case QWebEngineDownloadRequest::DownloadInProgress:
        return QStringLiteral("in_progress");
    case QWebEngineDownloadRequest::DownloadCompleted:
        return QStringLiteral("completed");
    case QWebEngineDownloadRequest::DownloadCancelled:
        return QStringLiteral("cancelled");
    case QWebEngineDownloadRequest::DownloadInterrupted:
        return QStringLiteral("interrupted");
    }
    return QStringLiteral("unknown");
}

void DownloadService::handleDownload(QWebEngineDownloadRequest* request)
{
    if (!request) {
        return;
    }

    QString directory = downloadDirectory();
    if (directory.isEmpty() || !QDir().mkpath(directory)) {
        directory = Settings::defaultDownloadDirectory();
        QDir().mkpath(directory);
    }

    QString fileName = request->downloadFileName();
    if (fileName.isEmpty()) {
        fileName = QFileInfo(request->url().path()).fileName();
    }
    if (fileName.isEmpty()) {
        fileName = QStringLiteral("download");
    }

    const QString target = uniquePath(directory, fileName);
    request->setDownloadDirectory(directory);
    request->setDownloadFileName(QFileInfo(target).fileName());

    const qint64 id = insertRecord(QFileInfo(target).fileName(),
                                   request->url().toString(),
                                   target,
                                   request->totalBytes(),
                                   0,
                                   QStringLiteral("requested"));
    if (id < 0) {
        qWarning() << "Refusing download: could not record it";
        request->cancel();
        return;
    }

    m_idsByRequest.insert(request, id);
    m_requestsById.insert(id, request);

    connect(request, &QWebEngineDownloadRequest::receivedBytesChanged,
            this, [this, request, id] {
                emit downloadProgress(id, request->receivedBytes(), request->totalBytes());
            });

    connect(request, &QWebEngineDownloadRequest::stateChanged,
            this, [this, request, id](QWebEngineDownloadRequest::DownloadState state) {
                updateRecord(id,
                             request->totalBytes(),
                             request->receivedBytes(),
                             statusToString(state),
                             QDir(request->downloadDirectory())
                                 .filePath(request->downloadFileName()));

                if (isFinalState(state)) {
                    finishDownload(id, request);
                } else {
                    emit downloadProgress(id, request->receivedBytes(),
                                          request->totalBytes());
                }
            });

    request->accept();

    emit downloadStarted(id, QFileInfo(target).fileName());
    emit downloadsChanged();
    qInfo() << "Download started:" << target;
}

void DownloadService::cancelDownload(qint64 id)
{
    if (QWebEngineDownloadRequest* request = m_requestsById.value(id, nullptr)) {
        request->cancel();
    }
}

void DownloadService::pauseDownload(qint64 id)
{
    if (QWebEngineDownloadRequest* request = m_requestsById.value(id, nullptr)) {
        request->pause();
    }
}

void DownloadService::resumeDownload(qint64 id)
{
    if (QWebEngineDownloadRequest* request = m_requestsById.value(id, nullptr)) {
        request->resume();
    }
}

qint64 DownloadService::insertRecord(const QString& filename,
                                     const QString& url,
                                     const QString& filePath,
                                     qint64 totalSize,
                                     qint64 downloadedSize,
                                     const QString& status)
{
    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "INSERT INTO downloads "
        "(filename, url, file_path, total_size, downloaded_size, status, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(filename);
    query.addBindValue(url);
    query.addBindValue(filePath);
    query.addBindValue(totalSize);
    query.addBindValue(downloadedSize);
    query.addBindValue(status);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());

    if (!query.exec()) {
        qWarning() << "Failed to record download:" << query.lastError().text();
        return -1;
    }
    return query.lastInsertId().toLongLong();
}

void DownloadService::updateRecord(qint64 id,
                                   qint64 totalSize,
                                   qint64 downloadedSize,
                                   const QString& status,
                                   const QString& filePath)
{
    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "UPDATE downloads SET total_size = ?, downloaded_size = ?, status = ?, file_path = ? "
        "WHERE id = ?"));
    query.addBindValue(totalSize);
    query.addBindValue(downloadedSize);
    query.addBindValue(status);
    query.addBindValue(filePath);
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to update download:" << query.lastError().text();
    }
}

void DownloadService::finishDownload(qint64 id, QWebEngineDownloadRequest* request)
{
    m_requestsById.remove(id);
    m_idsByRequest.remove(request);

    emit downloadFinished(id);
    emit downloadsChanged();

    qInfo() << "Download finished:" << request->downloadFileName()
            << statusToString(request->state());
}

} // namespace chainx
