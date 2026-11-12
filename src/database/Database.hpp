#pragma once

#include <QSqlDatabase>
#include <QString>

namespace chainx {

/**
 * Owns the SQLite connection and applies schema migrations.
 *
 * All services share this single connection. It must only be used from the
 * thread that created it (the GUI thread).
 */
class Database
{
public:
    explicit Database(QString filePath);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool open();
    void close();

    bool isOpen() const;
    QSqlDatabase connection() const;
    QString filePath() const;
    QString lastError() const;

    /// Current schema version stored in SQLite's PRAGMA user_version.
    int schemaVersion() const;

private:
    bool migrate();
    bool execute(const QString& sql);

    QString m_filePath;
    QString m_connectionName;
    QSqlDatabase m_db;
    QString m_lastError;
};

} // namespace chainx
