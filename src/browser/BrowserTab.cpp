#include "browser/BrowserTab.hpp"

#include <QColor>
#include <QUrl>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

#include "utils/UrlUtils.hpp"

namespace chainx {

BrowserTab::BrowserTab(QWebEngineProfile* profile, QWidget* parent)
    : QWebEngineView(parent)
{
    auto* webPage = new QWebEnginePage(profile, this);
    webPage->setBackgroundColor(QColor(0xFF, 0xFF, 0xFF));
    setPage(webPage);

    QWebEngineSettings* webSettings = webPage->settings();
    webSettings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    webSettings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
    webSettings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    webSettings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    webSettings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    webSettings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    webSettings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);
    webSettings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
}

QString BrowserTab::displayTitle() const
{
    const QString pageTitle = title().trimmed();
    if (!pageTitle.isEmpty()) {
        return pageTitle;
    }

    if (UrlUtils::isNewTabPage(url())) {
        return tr("New Tab");
    }

    const QString host = url().host();
    return host.isEmpty() ? tr("Untitled") : host;
}

void BrowserTab::setWindowFactory(std::function<BrowserTab*()> factory)
{
    m_windowFactory = std::move(factory);
}

QWebEngineView* BrowserTab::createWindow(QWebEnginePage::WebWindowType type)
{
    if (!m_windowFactory) {
        return nullptr;
    }

    // Popups and target="_blank" links open as regular ChainX tabs.
    Q_UNUSED(type);
    return m_windowFactory();
}

} // namespace chainx
