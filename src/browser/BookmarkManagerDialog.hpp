#pragma once

#include <QDialog>
#include <QUrl>

class QLineEdit;
class QPushButton;
class QTableWidget;
class QTreeWidget;

namespace chainx {

class BookmarkService;

/**
 * Full bookmark manager: browse bookmarks by folder, search, open, rename,
 * move between folders and delete. Folders themselves can be created,
 * renamed and removed.
 */
class BookmarkManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BookmarkManagerDialog(BookmarkService& bookmarks,
                                   QWidget* parent = nullptr);

signals:
    void openRequested(const QUrl& url);

private:
    void buildUi();
    void rebuildFolderTree();
    void refreshBookmarks();

    qint64 selectedBookmarkId() const;
    void updateButtonState();

    void openSelected();
    void deleteSelected();
    void renameSelected();
    void moveSelected();

    void createFolder();
    void renameFolder();
    void deleteFolder();

    BookmarkService& m_bookmarks;

    QTreeWidget* m_folderTree = nullptr;
    QLineEdit* m_search = nullptr;
    QTableWidget* m_table = nullptr;

    QPushButton* m_openButton = nullptr;
    QPushButton* m_deleteButton = nullptr;
    QPushButton* m_renameButton = nullptr;
    QPushButton* m_moveButton = nullptr;
    QPushButton* m_renameFolderButton = nullptr;
    QPushButton* m_deleteFolderButton = nullptr;

    qint64 m_currentFolderId = -1;
};

} // namespace chainx
