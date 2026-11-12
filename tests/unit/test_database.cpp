#include <gtest/gtest.h>

#include <QSqlQuery>
#include <QStringList>

#include "database/Database.hpp"
#include "services/BookmarkService.hpp"
#include "services/HistoryService.hpp"

using chainx::BookmarkService;
using chainx::Database;
using chainx::HistoryService;

namespace {

QStringList tableNames(const Database& database)
{
    QStringList names;
    QSqlQuery query(database.connection());
    if (!query.exec(QStringLiteral(
            "SELECT name FROM sqlite_master WHERE type = 'table' ORDER BY name"))) {
        return names;
    }
    while (query.next()) {
        names.append(query.value(0).toString());
    }
    return names;
}

} // namespace

TEST(DatabaseTest, CreatesSchema)
{
    Database database(QStringLiteral(":memory:"));
    ASSERT_TRUE(database.open());
    EXPECT_TRUE(database.isOpen());
    EXPECT_EQ(database.schemaVersion(), 1);

    const QStringList tables = tableNames(database);
    EXPECT_TRUE(tables.contains(QStringLiteral("bookmarks")));
    EXPECT_TRUE(tables.contains(QStringLiteral("bookmark_folders")));
    EXPECT_TRUE(tables.contains(QStringLiteral("history")));
    EXPECT_TRUE(tables.contains(QStringLiteral("downloads")));
    EXPECT_TRUE(tables.contains(QStringLiteral("settings")));
}

TEST(HistoryServiceTest, AddsAndReadsVisits)
{
    Database database(QStringLiteral(":memory:"));
    ASSERT_TRUE(database.open());

    HistoryService history(database);
    EXPECT_EQ(history.count(), 0);

    ASSERT_TRUE(history.addVisit(QStringLiteral("https://example.com"),
                                 QStringLiteral("Example")));
    ASSERT_TRUE(history.addVisit(QStringLiteral("https://qt.io"),
                                 QStringLiteral("Qt")));

    EXPECT_EQ(history.count(), 2);

    const QList<chainx::HistoryEntry> recent = history.recent();
    ASSERT_EQ(recent.size(), 2);
    EXPECT_EQ(recent.first().url, QStringLiteral("https://qt.io"));

    EXPECT_EQ(history.search(QStringLiteral("example")).size(), 1);

    ASSERT_TRUE(history.remove(recent.first().id));
    EXPECT_EQ(history.count(), 1);

    EXPECT_TRUE(history.clear());
    EXPECT_EQ(history.count(), 0);
}

TEST(HistoryServiceTest, SuggestsUrls)
{
    Database database(QStringLiteral(":memory:"));
    ASSERT_TRUE(database.open());

    HistoryService history(database);
    ASSERT_TRUE(history.addVisit(QStringLiteral("https://example.com/a"), QString()));
    ASSERT_TRUE(history.addVisit(QStringLiteral("https://other.test/b"), QString()));

    const QStringList suggestions = history.suggestions(QStringLiteral("example"));
    EXPECT_EQ(suggestions.size(), 1);
    EXPECT_EQ(suggestions.first(), QStringLiteral("https://example.com/a"));
}

TEST(BookmarkServiceTest, AddsRemovesAndSearches)
{
    Database database(QStringLiteral(":memory:"));
    ASSERT_TRUE(database.open());

    BookmarkService bookmarks(database);
    EXPECT_EQ(bookmarks.count(), 0);

    const QList<chainx::BookmarkFolder> folders = bookmarks.folders();
    ASSERT_FALSE(folders.isEmpty());
    const qint64 folderId = folders.first().id;

    ASSERT_TRUE(bookmarks.addBookmark(QStringLiteral("Example"),
                                      QStringLiteral("https://example.com"),
                                      folderId));

    EXPECT_TRUE(bookmarks.isBookmarked(QStringLiteral("https://example.com")));
    EXPECT_EQ(bookmarks.count(), 1);

    const QList<chainx::Bookmark> all = bookmarks.bookmarks();
    ASSERT_EQ(all.size(), 1);
    EXPECT_EQ(all.first().folderId, folderId);

    EXPECT_EQ(bookmarks.search(QStringLiteral("exam")).size(), 1);
    EXPECT_EQ(bookmarks.bookmarksInFolder(folderId).size(), 1);

    ASSERT_TRUE(bookmarks.removeByUrl(QStringLiteral("https://example.com")));
    EXPECT_FALSE(bookmarks.isBookmarked(QStringLiteral("https://example.com")));
    EXPECT_EQ(bookmarks.count(), 0);
}

TEST(BookmarkServiceTest, CreatesFolders)
{
    Database database(QStringLiteral(":memory:"));
    ASSERT_TRUE(database.open());

    BookmarkService bookmarks(database);
    const qint64 folderId = bookmarks.addFolder(QStringLiteral("Work"));
    EXPECT_GT(folderId, 0);

    const QList<chainx::BookmarkFolder> folders = bookmarks.folders();
    bool found = false;
    for (const chainx::BookmarkFolder& folder : folders) {
        if (folder.id == folderId && folder.name == QStringLiteral("Work")) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST(BookmarkServiceTest, UpdatesMovesAndRemovesFolders)
{
    Database database(QStringLiteral(":memory:"));
    ASSERT_TRUE(database.open());

    BookmarkService bookmarks(database);

    const QList<chainx::BookmarkFolder> initial = bookmarks.folders();
    ASSERT_FALSE(initial.isEmpty());
    const qint64 defaultFolder = initial.first().id;

    const qint64 work = bookmarks.addFolder(QStringLiteral("Work"));
    ASSERT_GT(work, 0);

    ASSERT_TRUE(bookmarks.addBookmark(QStringLiteral("Example"),
                                      QStringLiteral("https://example.com"),
                                      defaultFolder));

    const QList<chainx::Bookmark> all = bookmarks.bookmarks();
    ASSERT_EQ(all.size(), 1);
    const qint64 bookmarkId = all.first().id;

    ASSERT_TRUE(bookmarks.moveBookmark(bookmarkId, work));
    EXPECT_EQ(bookmarks.bookmarksInFolder(work).size(), 1);

    ASSERT_TRUE(bookmarks.updateBookmark(bookmarkId,
                                         QStringLiteral("Renamed"),
                                         QStringLiteral("https://example.com")));
    EXPECT_EQ(bookmarks.bookmarks().first().title, QStringLiteral("Renamed"));

    ASSERT_TRUE(bookmarks.renameFolder(work, QStringLiteral("Job")));
    bool renamed = false;
    for (const chainx::BookmarkFolder& folder : bookmarks.folders()) {
        if (folder.id == work && folder.name == QStringLiteral("Job")) {
            renamed = true;
        }
    }
    EXPECT_TRUE(renamed);

    ASSERT_TRUE(bookmarks.removeFolder(work));
    const QList<chainx::Bookmark> afterRemoval = bookmarks.bookmarks();
    ASSERT_EQ(afterRemoval.size(), 1);
    EXPECT_EQ(afterRemoval.first().folderId, -1);
}
