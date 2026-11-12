#include <gtest/gtest.h>

#include <QCheckBox>
#include <QLineEdit>
#include <QTableWidget>

#include "browser/BookmarkManagerDialog.hpp"
#include "browser/PrivacyDialog.hpp"
#include "browser/SettingsDialog.hpp"
#include "database/Database.hpp"
#include "services/BookmarkService.hpp"
#include "services/DownloadService.hpp"
#include "services/HistoryService.hpp"
#include "settings/Settings.hpp"

using chainx::BookmarkManagerDialog;
using chainx::BookmarkService;
using chainx::Database;
using chainx::DownloadService;
using chainx::HistoryService;
using chainx::PrivacyDialog;
using chainx::Settings;
using chainx::SettingsDialog;

TEST(DialogSmokeTest, BookmarkManagerListsBookmarks)
{
    Database database(QStringLiteral(":memory:"));
    ASSERT_TRUE(database.open());

    BookmarkService bookmarks(database);
    ASSERT_TRUE(bookmarks.addBookmark(QStringLiteral("Example"),
                                      QStringLiteral("https://example.com")));

    BookmarkManagerDialog dialog(bookmarks);

    auto* table = dialog.findChild<QTableWidget*>();
    ASSERT_NE(table, nullptr);
    ASSERT_EQ(table->rowCount(), 1);
    ASSERT_NE(table->item(0, 1), nullptr);
    EXPECT_EQ(table->item(0, 1)->text(), QStringLiteral("https://example.com"));
}

TEST(DialogSmokeTest, SettingsDialogLoadsValues)
{
    Settings settings;
    SettingsDialog dialog(settings);

    EXPECT_FALSE(dialog.windowTitle().isEmpty());
    EXPECT_FALSE(dialog.findChildren<QLineEdit*>().isEmpty());
}

TEST(DialogSmokeTest, PrivacyDialogBuilds)
{
    Database database(QStringLiteral(":memory:"));
    ASSERT_TRUE(database.open());

    Settings settings;
    HistoryService history(database);
    DownloadService downloads(database, settings);

    PrivacyDialog dialog(nullptr, history, downloads);
    EXPECT_FALSE(dialog.windowTitle().isEmpty());
    EXPECT_GE(dialog.findChildren<QCheckBox*>().size(), 4);
}
