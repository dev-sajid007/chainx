#include "services/BookmarkService.hpp"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "database/Database.hpp"
#include "utils/Logger.hpp"

namespace chainx {
namespace {

QString normaliseUrl(const QString& url)
{
    return url.trimmed();
}

} // namespace

BookmarkService::BookmarkService(Database& database, QObject* parent)
    : QObject(parent)
    , m_database(database)
{
}

bool BookmarkService::addBookmark(const QString& title, const QString& url, qint64 folderId)
{
    const QString normalised = normaliseUrl(url);
    if (normalised.isEmpty()) {
        return false;
    }

    if (isBookmarked(normalised)) {
        return true;
    }

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "INSERT INTO bookmarks (title, url, folder_id, created_at) VALUES (?, ?, ?, ?)"));
    query.addBindValue(title.trimmed().isEmpty() ? normalised : title.trimmed());
    query.addBindValue(normalised);
    if (folderId > 0) {
        query.addBindValue(folderId);
    } else {
        query.addBindValue(QVariant());
    }
    query.addBindValue(QDateTime::currentSecsSinceEpoch());

    if (!query.exec()) {
        qWarning() << "Failed to add bookmark:" << query.lastError().text();
        return false;
    }

    emit bookmarksChanged();
    return true;
}

bool BookmarkService::removeBookmark(qint64 id)
{
    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral("DELETE FROM bookmarks WHERE id = ?"));
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to remove bookmark:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        emit bookmarksChanged();
        return true;
    }
    return false;
}

bool BookmarkService::removeByUrl(const QString& url)
{
    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral("DELETE FROM bookmarks WHERE url = ?"));
    query.addBindValue(normaliseUrl(url));

    if (!query.exec()) {
        qWarning() << "Failed to remove bookmark by URL:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        emit bookmarksChanged();
        return true;
    }
    return false;
}

bool BookmarkService::isBookmarked(const QString& url) const
{
    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral("SELECT 1 FROM bookmarks WHERE url = ? LIMIT 1"));
    query.addBindValue(normaliseUrl(url));
    return query.exec() && query.next();
}

QList<Bookmark> BookmarkService::bookmarks() const
{
    QList<Bookmark> results;

    QSqlQuery query(m_database.connection());
    if (!query.exec(QStringLiteral(
            "SELECT id, title, url, COALESCE(folder_id, -1), created_at "
            "FROM bookmarks ORDER BY created_at ASC, id ASC"))) {
        qWarning() << "Failed to read bookmarks:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        Bookmark bookmark;
        bookmark.id = query.value(0).toLongLong();
        bookmark.title = query.value(1).toString();
        bookmark.url = query.value(2).toString();
        bookmark.folderId = query.value(3).toLongLong();
        bookmark.createdAt = QDateTime::fromSecsSinceEpoch(query.value(4).toLongLong());
        results.append(bookmark);
    }

    return results;
}

QList<Bookmark> BookmarkService::bookmarksInFolder(qint64 folderId) const
{
    QList<Bookmark> results;

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "SELECT id, title, url, COALESCE(folder_id, -1), created_at "
        "FROM bookmarks WHERE folder_id = ? ORDER BY created_at ASC, id ASC"));
    query.addBindValue(folderId);

    if (!query.exec()) {
        qWarning() << "Failed to read bookmarks in folder:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        Bookmark bookmark;
        bookmark.id = query.value(0).toLongLong();
        bookmark.title = query.value(1).toString();
        bookmark.url = query.value(2).toString();
        bookmark.folderId = query.value(3).toLongLong();
        bookmark.createdAt = QDateTime::fromSecsSinceEpoch(query.value(4).toLongLong());
        results.append(bookmark);
    }

    return results;
}

QList<Bookmark> BookmarkService::search(const QString& searchTerm, int limit) const
{
    QList<Bookmark> results;

    if (searchTerm.trimmed().isEmpty()) {
        return bookmarks();
    }

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "SELECT id, title, url, COALESCE(folder_id, -1), created_at "
        "FROM bookmarks WHERE title LIKE ? OR url LIKE ? "
        "ORDER BY created_at ASC, id ASC LIMIT ?"));
    const QString pattern = QStringLiteral("%") + searchTerm.trimmed() + QStringLiteral("%");
    query.addBindValue(pattern);
    query.addBindValue(pattern);
    query.addBindValue(qMax(1, limit));

    if (!query.exec()) {
        qWarning() << "Failed to search bookmarks:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        Bookmark bookmark;
        bookmark.id = query.value(0).toLongLong();
        bookmark.title = query.value(1).toString();
        bookmark.url = query.value(2).toString();
        bookmark.folderId = query.value(3).toLongLong();
        bookmark.createdAt = QDateTime::fromSecsSinceEpoch(query.value(4).toLongLong());
        results.append(bookmark);
    }

    return results;
}

QStringList BookmarkService::suggestions(const QString& prefix, int limit) const
{
    QStringList results;

    QSqlQuery query(m_database.connection());
    if (prefix.trimmed().isEmpty()) {
        query.prepare(QStringLiteral(
            "SELECT url FROM bookmarks ORDER BY created_at DESC LIMIT ?"));
        query.addBindValue(qMax(1, limit));
    } else {
        query.prepare(QStringLiteral(
            "SELECT url FROM bookmarks WHERE url LIKE ? OR title LIKE ? "
            "ORDER BY created_at DESC LIMIT ?"));
        const QString pattern = QStringLiteral("%") + prefix.trimmed() + QStringLiteral("%");
        query.addBindValue(pattern);
        query.addBindValue(pattern);
        query.addBindValue(qMax(1, limit));
    }

    if (!query.exec()) {
        qWarning() << "Failed to read bookmark suggestions:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        results.append(query.value(0).toString());
    }
    return results;
}

qint64 BookmarkService::addFolder(const QString& name, qint64 parentId)
{
    if (name.trimmed().isEmpty()) {
        return -1;
    }

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "INSERT INTO bookmark_folders (name, parent_id) VALUES (?, ?)"));
    query.addBindValue(name.trimmed());
    if (parentId > 0) {
        query.addBindValue(parentId);
    } else {
        query.addBindValue(QVariant());
    }

    if (!query.exec()) {
        qWarning() << "Failed to add bookmark folder:" << query.lastError().text();
        return -1;
    }

    emit bookmarksChanged();
    return query.lastInsertId().toLongLong();
}

QList<BookmarkFolder> BookmarkService::folders() const
{
    QList<BookmarkFolder> results;

    QSqlQuery query(m_database.connection());
    if (!query.exec(QStringLiteral(
            "SELECT id, name, COALESCE(parent_id, -1) FROM bookmark_folders "
            "ORDER BY name COLLATE NOCASE ASC"))) {
        qWarning() << "Failed to read bookmark folders:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        BookmarkFolder folder;
        folder.id = query.value(0).toLongLong();
        folder.name = query.value(1).toString();
        folder.parentId = query.value(2).toLongLong();
        results.append(folder);
    }

    return results;
}

bool BookmarkService::updateBookmark(qint64 id, const QString& title, const QString& url)
{
    const QString normalised = normaliseUrl(url);
    if (id <= 0 || normalised.isEmpty()) {
        return false;
    }

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "UPDATE bookmarks SET title = ?, url = ? WHERE id = ?"));
    query.addBindValue(title.trimmed().isEmpty() ? normalised : title.trimmed());
    query.addBindValue(normalised);
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to update bookmark:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        emit bookmarksChanged();
        return true;
    }
    return false;
}

bool BookmarkService::moveBookmark(qint64 id, qint64 folderId)
{
    if (id <= 0) {
        return false;
    }

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral("UPDATE bookmarks SET folder_id = ? WHERE id = ?"));
    if (folderId > 0) {
        query.addBindValue(folderId);
    } else {
        query.addBindValue(QVariant());
    }
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to move bookmark:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        emit bookmarksChanged();
        return true;
    }
    return false;
}

bool BookmarkService::renameFolder(qint64 id, const QString& name)
{
    if (id <= 0 || name.trimmed().isEmpty()) {
        return false;
    }

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral("UPDATE bookmark_folders SET name = ? WHERE id = ?"));
    query.addBindValue(name.trimmed());
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to rename bookmark folder:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        emit bookmarksChanged();
        return true;
    }
    return false;
}

bool BookmarkService::removeFolder(qint64 id)
{
    if (id <= 0) {
        return false;
    }

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral("DELETE FROM bookmark_folders WHERE id = ?"));
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to remove bookmark folder:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        emit bookmarksChanged();
        return true;
    }
    return false;
}

int BookmarkService::count() const
{
    QSqlQuery query(m_database.connection());
    if (query.exec(QStringLiteral("SELECT COUNT(*) FROM bookmarks")) && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

} // namespace chainx
