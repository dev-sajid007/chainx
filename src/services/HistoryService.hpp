#pragma once

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

namespace chainx {

class Database;

struct HistoryEntry
{
    qint64 id = -1;
    QString title;
    QString url;
    QDateTime visitedAt;
};

/**
 * Persists and queries browsing history.
 */
class HistoryService : public QObject
{
    Q_OBJECT

public:
    explicit HistoryService(Database& database, QObject* parent = nullptr);

    bool addVisit(const QString& url, const QString& title);

    QList<HistoryEntry> recent(int limit = 100) const;
    QList<HistoryEntry> search(const QString& query, int limit = 100) const;
    QList<HistoryEntry> all(int limit = 1000) const;

    bool remove(qint64 id);
    bool clear();

    /// Deletes entries older than @p days. Returns the number of rows removed.
    /// A non-positive value keeps history forever.
    int pruneOlderThan(int days);

    /// URL suggestions for the address bar, most recent first.
    QStringList suggestions(const QString& prefix, int limit = 8) const;

    int count() const;

signals:
    void historyChanged();

private:
    Database& m_database;
};

} // namespace chainx
