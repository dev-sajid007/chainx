#include "database/Database.hpp"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QUuid>
#include <QVariant>

#include "utils/Logger.hpp"

namespace chainx {
namespace {

constexpr int kSchemaVersion = 1;

struct Migration {
    int version;
    const char* statements;
};

const Migration kMigrations[] = {
    {1,
     R"SQL(
        CREATE TABLE IF NOT EXISTS bookmark_folders (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            name        TEXT    NOT NULL,
            parent_id   INTEGER REFERENCES bookmark_folders(id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS bookmarks (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            title       TEXT    NOT NULL,
            url         TEXT    NOT NULL,
            folder_id   INTEGER REFERENCES bookmark_folders(id) ON DELETE SET NULL,
            created_at  INTEGER NOT NULL
        );

        CREATE INDEX IF NOT EXISTS idx_bookmarks_url ON bookmarks(url);
        CREATE INDEX IF NOT EXISTS idx_bookmarks_folder ON bookmarks(folder_id);

        CREATE TABLE IF NOT EXISTS history (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            title       TEXT    NOT NULL,
            url         TEXT    NOT NULL,
            visited_at  INTEGER NOT NULL
        );

        CREATE INDEX IF NOT EXISTS idx_history_visited_at ON history(visited_at);
        CREATE INDEX IF NOT EXISTS idx_history_url ON history(url);

        CREATE TABLE IF NOT EXISTS downloads (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            filename        TEXT    NOT NULL,
            url             TEXT    NOT NULL,
            file_path       TEXT    NOT NULL,
            total_size      INTEGER NOT NULL DEFAULT 0,
            downloaded_size INTEGER NOT NULL DEFAULT 0,
            status          TEXT    NOT NULL,
            created_at      INTEGER NOT NULL
        );

        CREATE INDEX IF NOT EXISTS idx_downloads_created_at ON downloads(created_at);

        CREATE TABLE IF NOT EXISTS settings (
            id      INTEGER PRIMARY KEY AUTOINCREMENT,
            key     TEXT    NOT NULL UNIQUE,
            value   TEXT
        );

        INSERT INTO bookmark_folders (id, name, parent_id)
        SELECT 1, 'Bookmarks', NULL
        WHERE NOT EXISTS (SELECT 1 FROM bookmark_folders);
    )SQL"},
};

} // namespace

Database::Database(QString filePath)
    : m_filePath(std::move(filePath))
    , m_connectionName(QStringLiteral("chainx_%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

Database::~Database()
{
    close();
}

bool Database::open()
{
    if (m_db.isOpen()) {
        return true;
    }

    if (m_filePath != QLatin1String(":memory:")) {
        const QFileInfo info(m_filePath);
        if (!QDir().mkpath(info.absolutePath())) {
            m_lastError = QStringLiteral("Unable to create database directory: %1")
                              .arg(info.absolutePath());
            return false;
        }
    }

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(m_filePath);

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        qCritical() << "Failed to open database" << m_filePath << m_lastError;
        return false;
    }

    execute(QStringLiteral("PRAGMA foreign_keys = ON"));

    if (m_filePath != QLatin1String(":memory:")) {
        execute(QStringLiteral("PRAGMA journal_mode = WAL"));
        execute(QStringLiteral("PRAGMA synchronous = NORMAL"));
    }

    if (!migrate()) {
        m_db.close();
        return false;
    }

    qInfo() << "Database ready:" << m_filePath;
    return true;
}

void Database::close()
{
    if (!m_db.isValid()) {
        return;
    }

    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool Database::isOpen() const
{
    return m_db.isOpen();
}

QSqlDatabase Database::connection() const
{
    return m_db;
}

QString Database::filePath() const
{
    return m_filePath;
}

QString Database::lastError() const
{
    return m_lastError;
}

int Database::schemaVersion() const
{
    if (!m_db.isOpen()) {
        return 0;
    }

    QSqlQuery query(m_db);
    if (query.exec(QStringLiteral("PRAGMA user_version")) && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

bool Database::migrate()
{
    const int current = schemaVersion();

    if (!m_db.transaction()) {
        m_lastError = m_db.lastError().text();
        return false;
    }

    for (const Migration& migration : kMigrations) {
        if (migration.version <= current) {
            continue;
        }

        const QStringList statements =
            QString::fromLatin1(migration.statements)
                .split(QLatin1Char(';'), Qt::SkipEmptyParts);

        for (const QString& statement : statements) {
            const QString sql = statement.trimmed();
            if (sql.isEmpty()) {
                continue;
            }

            QSqlQuery query(m_db);
            if (!query.exec(sql)) {
                m_lastError = query.lastError().text();
                qCritical() << "Migration failed:" << m_lastError << "\nSQL:" << sql;
                m_db.rollback();
                return false;
            }
        }

        QSqlQuery versionQuery(m_db);
        if (!versionQuery.exec(QStringLiteral("PRAGMA user_version = %1")
                                   .arg(migration.version))) {
            m_lastError = versionQuery.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) {
        m_lastError = m_db.lastError().text();
        return false;
    }

    return true;
}

bool Database::execute(const QString& sql)
{
    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        qWarning() << "Database statement failed:" << m_lastError << "\nSQL:" << sql;
        return false;
    }
    return true;
}

} // namespace chainx
