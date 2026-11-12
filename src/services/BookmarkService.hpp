#pragma once

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

namespace chainx {

class Database;

struct BookmarkFolder
{
    qint64 id = -1;
    QString name;
    qint64 parentId = -1;
};

struct Bookmark
{
    qint64 id = -1;
    QString title;
    QString url;
    qint64 folderId = -1;
    QDateTime createdAt;
};

/**
 * Persists bookmarks and bookmark folders.
 */
class BookmarkService : public QObject
{
    Q_OBJECT

public:
    explicit BookmarkService(Database& database, QObject* parent = nullptr);

    bool addBookmark(const QString& title, const QString& url, qint64 folderId = -1);
    bool removeBookmark(qint64 id);
    bool removeByUrl(const QString& url);
    bool updateBookmark(qint64 id, const QString& title, const QString& url);
    bool moveBookmark(qint64 id, qint64 folderId);

    bool isBookmarked(const QString& url) const;

    QList<Bookmark> bookmarks() const;
    QList<Bookmark> bookmarksInFolder(qint64 folderId) const;
    QList<Bookmark> search(const QString& query, int limit = 100) const;

    QStringList suggestions(const QString& prefix, int limit = 8) const;

    qint64 addFolder(const QString& name, qint64 parentId = -1);
    bool renameFolder(qint64 id, const QString& name);
    bool removeFolder(qint64 id);
    QList<BookmarkFolder> folders() const;

    int count() const;

signals:
    void bookmarksChanged();

private:
    Database& m_database;
};

} // namespace chainx
