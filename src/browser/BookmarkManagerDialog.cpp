#include "browser/BookmarkManagerDialog.hpp"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QVariant>

#include "services/BookmarkService.hpp"

namespace chainx {
namespace {

constexpr int kAllBookmarks = -1;
constexpr int kMaxResults = 2000;

QVariant folderIdVariant(qint64 id)
{
    return QVariant::fromValue(id);
}

} // namespace

BookmarkManagerDialog::BookmarkManagerDialog(BookmarkService& bookmarks, QWidget* parent)
    : QDialog(parent)
    , m_bookmarks(bookmarks)
{
    buildUi();

    connect(&m_bookmarks, &BookmarkService::bookmarksChanged, this, [this] {
        rebuildFolderTree();
        refreshBookmarks();
    });

    rebuildFolderTree();
    refreshBookmarks();
}

void BookmarkManagerDialog::buildUi()
{
    setWindowTitle(tr("Bookmarks — ChainX"));
    resize(920, 580);

    auto* layout = new QVBoxLayout(this);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    auto* folderPane = new QWidget(splitter);
    auto* folderLayout = new QVBoxLayout(folderPane);
    folderLayout->setContentsMargins(0, 0, 0, 0);

    folderLayout->addWidget(new QLabel(tr("Folders"), folderPane));

    m_folderTree = new QTreeWidget(folderPane);
    m_folderTree->setHeaderHidden(true);
    m_folderTree->setRootIsDecorated(true);
    m_folderTree->setSelectionMode(QAbstractItemView::SingleSelection);
    folderLayout->addWidget(m_folderTree, 1);

    auto* folderButtons = new QHBoxLayout();
    auto* newFolderButton = new QPushButton(tr("New…"), folderPane);
    m_renameFolderButton = new QPushButton(tr("Rename…"), folderPane);
    m_deleteFolderButton = new QPushButton(tr("Delete"), folderPane);
    folderButtons->addWidget(newFolderButton);
    folderButtons->addWidget(m_renameFolderButton);
    folderButtons->addWidget(m_deleteFolderButton);
    folderLayout->addLayout(folderButtons);

    auto* bookmarkPane = new QWidget(splitter);
    auto* bookmarkLayout = new QVBoxLayout(bookmarkPane);
    bookmarkLayout->setContentsMargins(0, 0, 0, 0);

    m_search = new QLineEdit(bookmarkPane);
    m_search->setPlaceholderText(tr("Search bookmarks by title or URL"));
    m_search->setClearButtonEnabled(true);
    bookmarkLayout->addWidget(m_search);

    m_table = new QTableWidget(bookmarkPane);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({tr("Title"), tr("URL")});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    bookmarkLayout->addWidget(m_table, 1);

    auto* bookmarkButtons = new QHBoxLayout();
    m_openButton = new QPushButton(tr("Open"), bookmarkPane);
    m_openButton->setDefault(true);
    m_renameButton = new QPushButton(tr("Rename…"), bookmarkPane);
    m_moveButton = new QPushButton(tr("Move To…"), bookmarkPane);
    m_deleteButton = new QPushButton(tr("Delete"), bookmarkPane);
    bookmarkButtons->addWidget(m_openButton);
    bookmarkButtons->addWidget(m_renameButton);
    bookmarkButtons->addWidget(m_moveButton);
    bookmarkButtons->addStretch(1);
    bookmarkButtons->addWidget(m_deleteButton);
    bookmarkLayout->addLayout(bookmarkButtons);

    splitter->addWidget(folderPane);
    splitter->addWidget(bookmarkPane);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({220, 700});
    layout->addWidget(splitter, 1);

    auto* bottomRow = new QHBoxLayout();
    bottomRow->addStretch(1);
    auto* closeButton = new QPushButton(tr("Close"), this);
    bottomRow->addWidget(closeButton);
    layout->addLayout(bottomRow);

    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_folderTree, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem* current, QTreeWidgetItem*) {
                m_currentFolderId = current
                                        ? current->data(0, Qt::UserRole).toLongLong()
                                        : kAllBookmarks;
                if (!m_search->text().isEmpty()) {
                    m_search->clear();
                } else {
                    refreshBookmarks();
                }
            });
    connect(m_search, &QLineEdit::textChanged, this, [this] { refreshBookmarks(); });
    connect(m_openButton, &QPushButton::clicked, this, &BookmarkManagerDialog::openSelected);
    connect(m_renameButton, &QPushButton::clicked,
            this, &BookmarkManagerDialog::renameSelected);
    connect(m_moveButton, &QPushButton::clicked, this, &BookmarkManagerDialog::moveSelected);
    connect(m_deleteButton, &QPushButton::clicked,
            this, &BookmarkManagerDialog::deleteSelected);
    connect(newFolderButton, &QPushButton::clicked, this,
            &BookmarkManagerDialog::createFolder);
    connect(m_renameFolderButton, &QPushButton::clicked, this,
            &BookmarkManagerDialog::renameFolder);
    connect(m_deleteFolderButton, &QPushButton::clicked, this,
            &BookmarkManagerDialog::deleteFolder);
    connect(m_table, &QTableWidget::cellDoubleClicked, this,
            [this](int, int) { openSelected(); });
    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &BookmarkManagerDialog::updateButtonState);

    auto* deleteShortcut = new QShortcut(QKeySequence::Delete, m_table);
    deleteShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(deleteShortcut, &QShortcut::activated,
            this, &BookmarkManagerDialog::deleteSelected);

    updateButtonState();
}

void BookmarkManagerDialog::rebuildFolderTree()
{
    const qint64 previous = m_currentFolderId;

    const QSignalBlocker blocker(m_folderTree);
    m_folderTree->clear();

    auto* allItem = new QTreeWidgetItem(m_folderTree);
    allItem->setText(0, tr("All Bookmarks"));
    allItem->setData(0, Qt::UserRole, folderIdVariant(kAllBookmarks));

    const QList<BookmarkFolder> folders = m_bookmarks.folders();

    QHash<qint64, QTreeWidgetItem*> items;
    for (const BookmarkFolder& folder : folders) {
        auto* item = new QTreeWidgetItem();
        item->setText(0, folder.name);
        item->setData(0, Qt::UserRole, folderIdVariant(folder.id));
        items.insert(folder.id, item);
    }

    for (const BookmarkFolder& folder : folders) {
        QTreeWidgetItem* item = items.value(folder.id);
        if (!item) {
            continue;
        }
        QTreeWidgetItem* parent =
            folder.parentId > 0 ? items.value(folder.parentId, allItem) : allItem;
        parent->addChild(item);
    }

    m_folderTree->expandAll();

    QTreeWidgetItem* toSelect = allItem;
    if (previous > 0 && items.contains(previous)) {
        toSelect = items.value(previous);
    }
    m_currentFolderId = toSelect->data(0, Qt::UserRole).toLongLong();
    m_folderTree->setCurrentItem(toSelect);

    m_renameFolderButton->setEnabled(m_currentFolderId > 0);
    m_deleteFolderButton->setEnabled(m_currentFolderId > 0);
}

void BookmarkManagerDialog::refreshBookmarks()
{
    const QString filter = m_search->text().trimmed();

    QList<Bookmark> entries;
    if (!filter.isEmpty()) {
        entries = m_bookmarks.search(filter, kMaxResults);
    } else if (m_currentFolderId > 0) {
        entries = m_bookmarks.bookmarksInFolder(m_currentFolderId);
    } else {
        entries = m_bookmarks.bookmarks();
    }

    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(entries.size()));

    for (int row = 0; row < entries.size(); ++row) {
        const Bookmark& bookmark = entries.at(row);

        auto* titleItem = new QTableWidgetItem(bookmark.title);
        titleItem->setData(Qt::UserRole, bookmark.id);
        titleItem->setToolTip(bookmark.url);

        auto* urlItem = new QTableWidgetItem(bookmark.url);
        urlItem->setData(Qt::UserRole, bookmark.id);

        m_table->setItem(row, 0, titleItem);
        m_table->setItem(row, 1, urlItem);
    }

    updateButtonState();
}

qint64 BookmarkManagerDialog::selectedBookmarkId() const
{
    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return -1;
    }

    const QTableWidgetItem* item = m_table->item(rows.first().row(), 0);
    return item ? item->data(Qt::UserRole).toLongLong() : -1;
}

void BookmarkManagerDialog::updateButtonState()
{
    const bool hasSelection = selectedBookmarkId() > 0;
    m_openButton->setEnabled(hasSelection);
    m_renameButton->setEnabled(hasSelection);
    m_moveButton->setEnabled(hasSelection);
    m_deleteButton->setEnabled(hasSelection);

    const bool hasFolder = m_currentFolderId > 0;
    m_renameFolderButton->setEnabled(hasFolder);
    m_deleteFolderButton->setEnabled(hasFolder);
}

void BookmarkManagerDialog::openSelected()
{
    const qint64 id = selectedBookmarkId();
    if (id <= 0) {
        return;
    }

    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    const QTableWidgetItem* urlItem = m_table->item(rows.first().row(), 1);
    if (!urlItem) {
        return;
    }

    const QUrl url(urlItem->text());
    if (url.isValid()) {
        emit openRequested(url);
        accept();
    }
}

void BookmarkManagerDialog::deleteSelected()
{
    const qint64 id = selectedBookmarkId();
    if (id <= 0) {
        return;
    }

    const QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        tr("Delete Bookmark"),
        tr("Delete the selected bookmark?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (answer == QMessageBox::Yes) {
        m_bookmarks.removeBookmark(id);
    }
}

void BookmarkManagerDialog::renameSelected()
{
    const qint64 id = selectedBookmarkId();
    if (id <= 0) {
        return;
    }

    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    const int row = rows.first().row();
    const QTableWidgetItem* titleItem = m_table->item(row, 0);
    const QTableWidgetItem* urlItem = m_table->item(row, 1);
    if (!titleItem || !urlItem) {
        return;
    }

    bool accepted = false;
    const QString title = QInputDialog::getText(this,
                                                tr("Rename Bookmark"),
                                                tr("Title:"),
                                                QLineEdit::Normal,
                                                titleItem->text(),
                                                &accepted);

    if (accepted && !title.trimmed().isEmpty()) {
        m_bookmarks.updateBookmark(id, title, urlItem->text());
    }
}

void BookmarkManagerDialog::moveSelected()
{
    const qint64 id = selectedBookmarkId();
    if (id <= 0) {
        return;
    }

    QStringList names{tr("Unfiled")};
    QList<qint64> ids{kAllBookmarks};

    for (const BookmarkFolder& folder : m_bookmarks.folders()) {
        names.append(folder.name);
        ids.append(folder.id);
    }

    bool accepted = false;
    const QString choice = QInputDialog::getItem(this,
                                                 tr("Move Bookmark"),
                                                 tr("Folder:"),
                                                 names,
                                                 0,
                                                 false,
                                                 &accepted);

    if (!accepted) {
        return;
    }

    const int index = names.indexOf(choice);
    if (index >= 0 && index < ids.size()) {
        m_bookmarks.moveBookmark(id, ids.at(index));
    }
}

void BookmarkManagerDialog::createFolder()
{
    bool accepted = false;
    const QString name = QInputDialog::getText(this,
                                               tr("New Folder"),
                                               tr("Folder name:"),
                                               QLineEdit::Normal,
                                               QString(),
                                               &accepted);

    if (accepted && !name.trimmed().isEmpty()) {
        m_bookmarks.addFolder(name);
    }
}

void BookmarkManagerDialog::renameFolder()
{
    if (m_currentFolderId <= 0) {
        return;
    }

    QTreeWidgetItem* item = m_folderTree->currentItem();
    if (!item) {
        return;
    }

    bool accepted = false;
    const QString name = QInputDialog::getText(this,
                                               tr("Rename Folder"),
                                               tr("Folder name:"),
                                               QLineEdit::Normal,
                                               item->text(0),
                                               &accepted);

    if (accepted && !name.trimmed().isEmpty()) {
        m_bookmarks.renameFolder(m_currentFolderId, name);
    }
}

void BookmarkManagerDialog::deleteFolder()
{
    if (m_currentFolderId <= 0) {
        return;
    }

    const QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        tr("Delete Folder"),
        tr("Delete this folder? Its bookmarks will become unfiled."),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (answer == QMessageBox::Yes) {
        m_bookmarks.removeFolder(m_currentFolderId);
    }
}

} // namespace chainx
