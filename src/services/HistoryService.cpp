#include "services/HistoryService.hpp"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "database/Database.hpp"
#include "utils/Logger.hpp"

namespace chainx {

HistoryService::HistoryService(Database& database, QObject* parent)
    : QObject(parent)
    , m_database(database)
{
}

bool HistoryService::addVisit(const QString& url, const QString& title)
{
    if (url.trimmed().isEmpty()) {
        return false;
    }

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "INSERT INTO history (title, url, visited_at) VALUES (?, ?, ?)"));
    query.addBindValue(title.trimmed().isEmpty() ? url : title.trimmed());
    query.addBindValue(url);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());

    if (!query.exec()) {
        qWarning() << "Failed to add history entry:" << query.lastError().text();
        return false;
    }

    emit historyChanged();
    return true;
}

QList<HistoryEntry> HistoryService::recent(int limit) const
{
    QList<HistoryEntry> entries;

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "SELECT id, title, url, visited_at FROM history "
        "ORDER BY visited_at DESC, id DESC LIMIT ?"));
    query.addBindValue(qMax(1, limit));

    if (!query.exec()) {
        qWarning() << "Failed to read history:" << query.lastError().text();
        return entries;
    }

    while (query.next()) {
        HistoryEntry entry;
        entry.id = query.value(0).toLongLong();
        entry.title = query.value(1).toString();
        entry.url = query.value(2).toString();
        entry.visitedAt = QDateTime::fromSecsSinceEpoch(query.value(3).toLongLong());
        entries.append(entry);
    }

    return entries;
}

QList<HistoryEntry> HistoryService::search(const QString& searchTerm, int limit) const
{
    QList<HistoryEntry> entries;

    if (searchTerm.trimmed().isEmpty()) {
        return recent(limit);
    }

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral(
        "SELECT id, title, url, visited_at FROM history "
        "WHERE title LIKE ? OR url LIKE ? "
        "ORDER BY visited_at DESC, id DESC LIMIT ?"));
    const QString pattern = QStringLiteral("%") + searchTerm.trimmed() + QStringLiteral("%");
    query.addBindValue(pattern);
    query.addBindValue(pattern);
    query.addBindValue(qMax(1, limit));

    if (!query.exec()) {
        qWarning() << "Failed to search history:" << query.lastError().text();
        return entries;
    }

    while (query.next()) {
        HistoryEntry entry;
        entry.id = query.value(0).toLongLong();
        entry.title = query.value(1).toString();
        entry.url = query.value(2).toString();
        entry.visitedAt = QDateTime::fromSecsSinceEpoch(query.value(3).toLongLong());
        entries.append(entry);
    }

    return entries;
}

QList<HistoryEntry> HistoryService::all(int limit) const
{
    return recent(limit);
}

bool HistoryService::remove(qint64 id)
{
    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral("DELETE FROM history WHERE id = ?"));
    query.addBindValue(id);

    if (!query.exec()) {
        qWarning() << "Failed to remove history entry:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() > 0) {
        emit historyChanged();
        return true;
    }
    return false;
}

bool HistoryService::clear()
{
    QSqlQuery query(m_database.connection());
    if (!query.exec(QStringLiteral("DELETE FROM history"))) {
        qWarning() << "Failed to clear history:" << query.lastError().text();
        return false;
    }

    emit historyChanged();
    return true;
}

int HistoryService::pruneOlderThan(int days)
{
    if (days <= 0) {
        return 0;
    }

    const qint64 cutoff =
        QDateTime::currentSecsSinceEpoch() - static_cast<qint64>(days) * 24 * 60 * 60;

    QSqlQuery query(m_database.connection());
    query.prepare(QStringLiteral("DELETE FROM history WHERE visited_at < ?"));
    query.addBindValue(cutoff);

    if (!query.exec()) {
        qWarning() << "Failed to prune history:" << query.lastError().text();
        return 0;
    }

    const int removed = query.numRowsAffected();
    if (removed > 0) {
        qInfo() << "Pruned" << removed << "history entries older than" << days << "days";
        emit historyChanged();
    }
    return removed;
}

QStringList HistoryService::suggestions(const QString& prefix, int limit) const
{
    QStringList results;

    QSqlQuery query(m_database.connection());
    if (prefix.trimmed().isEmpty()) {
        query.prepare(QStringLiteral(
            "SELECT url FROM history GROUP BY url "
            "ORDER BY MAX(visited_at) DESC LIMIT ?"));
        query.addBindValue(qMax(1, limit));
    } else {
        query.prepare(QStringLiteral(
            "SELECT url FROM history WHERE url LIKE ? GROUP BY url "
            "ORDER BY MAX(visited_at) DESC LIMIT ?"));
        query.addBindValue(QStringLiteral("%") + prefix.trimmed() + QStringLiteral("%"));
        query.addBindValue(qMax(1, limit));
    }

    if (!query.exec()) {
        qWarning() << "Failed to read history suggestions:" << query.lastError().text();
        return results;
    }

    while (query.next()) {
        results.append(query.value(0).toString());
    }
    return results;
}

int HistoryService::count() const
{
    QSqlQuery query(m_database.connection());
    if (query.exec(QStringLiteral("SELECT COUNT(*) FROM history")) && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

} // namespace chainx
