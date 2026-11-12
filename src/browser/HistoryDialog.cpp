#include "browser/HistoryDialog.hpp"

#include <QAbstractItemView>
#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "services/HistoryService.hpp"

namespace chainx {
namespace {

constexpr int kMaxRows = 1000;

} // namespace

HistoryDialog::HistoryDialog(HistoryService& history, QWidget* parent)
    : QDialog(parent)
    , m_history(history)
{
    setWindowTitle(tr("History — ChainX"));
    resize(860, 540);

    auto* layout = new QVBoxLayout(this);

    auto* searchRow = new QHBoxLayout();
    auto* searchLabel = new QLabel(tr("Search:"), this);
    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Filter by title or URL"));
    m_search->setClearButtonEnabled(true);
    searchRow->addWidget(searchLabel);
    searchRow->addWidget(m_search, 1);
    layout->addLayout(searchRow);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({tr("Title"), tr("URL"), tr("Visited")});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    layout->addWidget(m_table, 1);

    auto* buttonRow = new QHBoxLayout();
    auto* deleteButton = new QPushButton(tr("Delete Selected"), this);
    auto* clearButton = new QPushButton(tr("Clear All…"), this);
    auto* closeButton = new QPushButton(tr("Close"), this);
    closeButton->setDefault(true);
    buttonRow->addWidget(deleteButton);
    buttonRow->addWidget(clearButton);
    buttonRow->addStretch(1);
    buttonRow->addWidget(closeButton);
    layout->addLayout(buttonRow);

    connect(m_search, &QLineEdit::textChanged, this, &HistoryDialog::refresh);
    connect(deleteButton, &QPushButton::clicked, this, &HistoryDialog::deleteSelected);
    connect(clearButton, &QPushButton::clicked, this, &HistoryDialog::clearAll);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        const QTableWidgetItem* item = m_table->item(row, 2);
        if (!item) {
            return;
        }
        const QUrl url(item->data(Qt::UserRole + 1).toString());
        if (url.isValid()) {
            emit openRequested(url);
            accept();
        }
    });

    refresh(QString());
}

void HistoryDialog::refresh(const QString& filter)
{
    const QList<HistoryEntry> entries =
        filter.trimmed().isEmpty() ? m_history.recent(kMaxRows)
                                   : m_history.search(filter, kMaxRows);

    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(entries.size()));

    for (int row = 0; row < entries.size(); ++row) {
        const HistoryEntry& entry = entries.at(row);

        auto* titleItem = new QTableWidgetItem(entry.title);
        titleItem->setData(Qt::UserRole, entry.id);

        auto* urlItem = new QTableWidgetItem(entry.url);
        urlItem->setData(Qt::UserRole, entry.id);

        auto* visitedItem = new QTableWidgetItem(
            entry.visitedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm")));
        visitedItem->setData(Qt::UserRole, entry.id);
        visitedItem->setData(Qt::UserRole + 1, entry.url);

        m_table->setItem(row, 0, titleItem);
        m_table->setItem(row, 1, urlItem);
        m_table->setItem(row, 2, visitedItem);
    }
}

void HistoryDialog::deleteSelected()
{
    QList<qint64> ids;
    const QList<QTableWidgetItem*> selected = m_table->selectedItems();
    for (const QTableWidgetItem* item : selected) {
        if (!item || item->column() != 0) {
            continue;
        }
        const qint64 id = item->data(Qt::UserRole).toLongLong();
        if (!ids.contains(id)) {
            ids.append(id);
        }
    }

    for (qint64 id : ids) {
        m_history.remove(id);
    }

    refresh(m_search->text());
}

void HistoryDialog::clearAll()
{
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        tr("Clear Browsing History"),
        tr("Delete all browsing history? This cannot be undone."),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (answer == QMessageBox::Yes) {
        m_history.clear();
        refresh(m_search->text());
    }
}

} // namespace chainx
