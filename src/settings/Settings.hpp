#pragma once

#include <QSettings>
#include <QString>
#include <QVariant>

namespace chainx {

/**
 * Typed access to persistent application preferences.
 *
 * Backed by QSettings (INI format) so the values survive across releases and
 * are easy to inspect. Web preferences that belong to the Chromium profile are
 * configured separately in Application.
 */
class Settings
{
public:
    Settings();

    QString homepage() const;
    void setHomepage(const QString& url);

    QString searchEngineName() const;
    QString searchEngineTemplate() const;
    void setSearchEngine(const QString& name, const QString& templateUrl);

    QString downloadDirectory() const;
    void setDownloadDirectory(const QString& path);

    /// Days to keep history for. Zero keeps entries forever.
    int historyRetentionDays() const;
    void setHistoryRetentionDays(int days);

    bool restoreSession() const;
    void setRestoreSession(bool enabled);

    bool javascriptEnabled() const;
    void setJavascriptEnabled(bool enabled);

    QVariant value(const QString& key, const QVariant& fallback = {}) const;
    void setValue(const QString& key, const QVariant& value);

    void sync();

    static QString defaultDownloadDirectory();

private:
    QSettings m_settings;
};

} // namespace chainx
