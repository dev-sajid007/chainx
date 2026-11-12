#pragma once

#include <QWebEnginePage>
#include <QWebEngineView>

#include <functional>

class QWebEngineProfile;

namespace chainx {

/**
 * A single browser tab: a WebEngine view configured with ChainX preferences.
 */
class BrowserTab : public QWebEngineView
{
    Q_OBJECT

public:
    explicit BrowserTab(QWebEngineProfile* profile, QWidget* parent = nullptr);

    /// Title suitable for the tab bar and window title.
    QString displayTitle() const;

    /// Applies the JavaScript preference to this tab's page.
    void setJavascriptEnabled(bool enabled);

    /// Used by createWindow() to open target="_blank" links in a new tab.
    void setWindowFactory(std::function<BrowserTab*()> factory);

protected:
    QWebEngineView* createWindow(QWebEnginePage::WebWindowType type) override;

private:
    void handlePermissionRequest(const QUrl& securityOrigin,
                                 QWebEnginePage::Feature feature);

    std::function<BrowserTab*()> m_windowFactory;
};

} // namespace chainx
