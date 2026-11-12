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

    /// Used by createWindow() to open target="_blank" links in a new tab.
    void setWindowFactory(std::function<BrowserTab*()> factory);

protected:
    QWebEngineView* createWindow(QWebEnginePage::WebWindowType type) override;

private:
    std::function<BrowserTab*()> m_windowFactory;
};

} // namespace chainx
