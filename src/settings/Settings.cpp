#include "settings/Settings.hpp"

#include <QStandardPaths>

#include "utils/UrlUtils.hpp"

namespace chainx {
namespace {

constexpr auto kHomepage = "browser/homepage";
constexpr auto kSearchEngineName = "browser/searchEngineName";
constexpr auto kSearchEngineTemplate = "browser/searchEngineTemplate";
constexpr auto kDownloadDirectory = "downloads/directory";
constexpr auto kHistoryRetention = "history/retentionDays";
constexpr auto kRestoreSession = "browser/restoreSession";
constexpr auto kJavascript = "privacy/javascriptEnabled";
constexpr auto kBlockInsecureContent = "privacy/blockInsecureContent";

} // namespace

Settings::Settings()
    : m_settings(QSettings::IniFormat, QSettings::UserScope,
                 QStringLiteral("ChainX"), QStringLiteral("ChainX"))
{
}

QString Settings::homepage() const
{
    return m_settings.value(QString::fromLatin1(kHomepage),
                            UrlUtils::newTabUrl().toString())
        .toString();
}

void Settings::setHomepage(const QString& url)
{
    m_settings.setValue(QString::fromLatin1(kHomepage), url);
}

QString Settings::searchEngineName() const
{
    return m_settings.value(QString::fromLatin1(kSearchEngineName),
                            QStringLiteral("DuckDuckGo"))
        .toString();
}

QString Settings::searchEngineTemplate() const
{
    return m_settings.value(QString::fromLatin1(kSearchEngineTemplate),
                            QString::fromLatin1(UrlUtils::DefaultSearchTemplate))
        .toString();
}

void Settings::setSearchEngine(const QString& name, const QString& templateUrl)
{
    m_settings.setValue(QString::fromLatin1(kSearchEngineName), name);
    m_settings.setValue(QString::fromLatin1(kSearchEngineTemplate), templateUrl);
}

QString Settings::downloadDirectory() const
{
    return m_settings.value(QString::fromLatin1(kDownloadDirectory),
                            defaultDownloadDirectory())
        .toString();
}

void Settings::setDownloadDirectory(const QString& path)
{
    m_settings.setValue(QString::fromLatin1(kDownloadDirectory), path);
}

int Settings::historyRetentionDays() const
{
    return m_settings.value(QString::fromLatin1(kHistoryRetention), 90).toInt();
}

void Settings::setHistoryRetentionDays(int days)
{
    m_settings.setValue(QString::fromLatin1(kHistoryRetention), qMax(0, days));
}

bool Settings::restoreSession() const
{
    return m_settings.value(QString::fromLatin1(kRestoreSession), true).toBool();
}

void Settings::setRestoreSession(bool enabled)
{
    m_settings.setValue(QString::fromLatin1(kRestoreSession), enabled);
}

bool Settings::javascriptEnabled() const
{
    return m_settings.value(QString::fromLatin1(kJavascript), true).toBool();
}

void Settings::setJavascriptEnabled(bool enabled)
{
    m_settings.setValue(QString::fromLatin1(kJavascript), enabled);
}

bool Settings::blockInsecureContent() const
{
    return m_settings.value(QString::fromLatin1(kBlockInsecureContent), true).toBool();
}

void Settings::setBlockInsecureContent(bool enabled)
{
    m_settings.setValue(QString::fromLatin1(kBlockInsecureContent), enabled);
}

QVariant Settings::value(const QString& key, const QVariant& fallback) const
{
    return m_settings.value(key, fallback);
}

void Settings::setValue(const QString& key, const QVariant& value)
{
    m_settings.setValue(key, value);
}

void Settings::sync()
{
    m_settings.sync();
}

QString Settings::defaultDownloadDirectory()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (path.isEmpty()) {
        path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
    return path;
}

} // namespace chainx
