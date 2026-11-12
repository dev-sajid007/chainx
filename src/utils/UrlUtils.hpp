#pragma once

#include <QString>
#include <QUrl>

namespace chainx {

/**
 * Helpers for turning raw address-bar input into a navigable URL and for
 * presenting URLs back to the user.
 */
class UrlUtils
{
public:
    UrlUtils() = delete;

    static constexpr const char* DefaultSearchTemplate =
        "https://duckduckgo.com/?q=%1";

    /// Resolves user input to a URL, falling back to a search query.
    /// Returns an invalid QUrl when the input is empty.
    static QUrl resolveInput(const QString& input,
                             const QString& searchTemplate = QString::fromLatin1(DefaultSearchTemplate));

    /// True when the input looks like a host/URL rather than a search phrase.
    static bool looksLikeUrl(const QString& input);

    /// True when the URL is safe to show/record in history (http, https, file).
    static bool isRecordable(const QUrl& url);

    /// Human-readable representation for the address bar.
    static QString displayString(const QUrl& url);

    /// True when the URL points at the built-in new tab page.
    static bool isNewTabPage(const QUrl& url);

    /// URL of the built-in new tab page (Qt resource).
    static QUrl newTabUrl();

    /// Escapes a search phrase for use in a template containing %1.
    static QString searchUrl(const QString& query,
                             const QString& searchTemplate = QString::fromLatin1(DefaultSearchTemplate));
};

} // namespace chainx
