#pragma once

#include <QDialog>
#include <QUrl>

class QLineEdit;
class QTableWidget;

namespace chainx {

class HistoryService;

/**
 * Searchable browsing history with per-entry deletion.
 */
class HistoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HistoryDialog(HistoryService& history, QWidget* parent = nullptr);

signals:
    void openRequested(const QUrl& url);

private:
    void refresh(const QString& filter);
    void deleteSelected();
    void clearAll();

    HistoryService& m_history;
    QLineEdit* m_search = nullptr;
    QTableWidget* m_table = nullptr;
};

} // namespace chainx
