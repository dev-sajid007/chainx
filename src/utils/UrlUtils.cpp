#include "utils/UrlUtils.hpp"

#include <QRegularExpression>

namespace chainx {

QUrl UrlUtils::resolveInput(const QString& input, const QString& searchTemplate)
{
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    static const QRegularExpression schemePattern(
        QStringLiteral("^(https?|ftp|file|qrc|about|data|mailto|view-source):"),
        QRegularExpression::CaseInsensitiveOption);

    if (schemePattern.match(trimmed).hasMatch()) {
        const QUrl url(trimmed);
        if (url.isValid()) {
            return url;
        }
    }

    if (looksLikeUrl(trimmed)) {
        return QUrl(QStringLiteral("https://") + trimmed);
    }

    return QUrl(searchUrl(trimmed, searchTemplate));
}

bool UrlUtils::looksLikeUrl(const QString& input)
{
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty() || trimmed.contains(QLatin1Char(' '))) {
        return false;
    }

    if (trimmed == QLatin1String("localhost")
        || trimmed.startsWith(QLatin1String("localhost:"))
        || trimmed.startsWith(QLatin1String("localhost/"))) {
        return true;
    }

    // IPv4 literal, optionally with a port.
    static const QRegularExpression ipv4(
        QStringLiteral("^\\d{1,3}(\\.\\d{1,3}){3}(:\\d+)?([/?#].*)?$"));
    if (ipv4.match(trimmed).hasMatch()) {
        return true;
    }

    // A dotted decimal number (for example "3.14") is not a domain.
    static const QRegularExpression numericOnly(
        QStringLiteral("^\\d+(\\.\\d+)+$"));
    if (numericOnly.match(trimmed).hasMatch()) {
        return false;
    }

    // Domain name with a plausible TLD, optionally with a path.
    static const QRegularExpression domain(
        QStringLiteral("^[A-Za-z0-9]([A-Za-z0-9-]*[A-Za-z0-9])?"
                       "(\\.[A-Za-z0-9]([A-Za-z0-9-]*[A-Za-z0-9])?)+"
                       "(:\\d+)?([/?#].*)?$"));
    return domain.match(trimmed).hasMatch();
}

bool UrlUtils::isRecordable(const QUrl& url)
{
    return url.isValid() && (url.scheme() == QLatin1String("http")
                             || url.scheme() == QLatin1String("https")
                             || url.scheme() == QLatin1String("file"));
}

QString UrlUtils::displayString(const QUrl& url)
{
    if (!url.isValid() || url.isEmpty()) {
        return {};
    }
    if (isNewTabPage(url)) {
        return {};
    }
    return url.toDisplayString(QUrl::RemovePassword);
}

QString UrlUtils::searchUrl(const QString& query, const QString& searchTemplate)
{
    return QString(searchTemplate).replace(QStringLiteral("%1"),
                                           QString::fromLatin1(QUrl::toPercentEncoding(query)));
}

bool UrlUtils::isNewTabPage(const QUrl& url)
{
    return url.scheme() == QLatin1String("qrc")
           && url.path() == QLatin1String("/pages/newtab.html");
}

QUrl UrlUtils::newTabUrl()
{
    return QUrl(QStringLiteral("qrc:/pages/newtab.html"));
}

} // namespace chainx
