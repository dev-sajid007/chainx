#include "browser/BrowserTab.hpp"

#include <QColor>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

#include "utils/UrlUtils.hpp"

namespace chainx {
namespace {

QString featureName(QWebEnginePage::Feature feature)
{
    switch (feature) {
    case QWebEnginePage::Notifications:
        return QObject::tr("notifications");
    case QWebEnginePage::Geolocation:
        return QObject::tr("your location");
    case QWebEnginePage::MediaAudioCapture:
        return QObject::tr("your microphone");
    case QWebEnginePage::MediaVideoCapture:
        return QObject::tr("your camera");
    case QWebEnginePage::MediaAudioVideoCapture:
        return QObject::tr("your camera and microphone");
    case QWebEnginePage::MouseLock:
        return QObject::tr("your mouse pointer");
    case QWebEnginePage::DesktopVideoCapture:
        return QObject::tr("your screen");
    case QWebEnginePage::DesktopAudioVideoCapture:
        return QObject::tr("your screen and audio");
    }
    return QObject::tr("this permission");
}

} // namespace

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

    connect(webPage, &QWebEnginePage::featurePermissionRequested,
            this, &BrowserTab::handlePermissionRequest);
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

void BrowserTab::setJavascriptEnabled(bool enabled)
{
    page()->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, enabled);
}

void BrowserTab::handlePermissionRequest(const QUrl& securityOrigin,
                                         QWebEnginePage::Feature feature)
{
    const QString host = securityOrigin.host().isEmpty()
                             ? securityOrigin.toString()
                             : securityOrigin.host();

    QMessageBox box(this);
    box.setWindowTitle(tr("Permission Request"));
    box.setIcon(QMessageBox::Question);
    box.setText(tr("<b>%1</b> wants to use %2.")
                    .arg(host.toHtmlEscaped(), featureName(feature)));
    box.setInformativeText(tr("You can change this later by reloading the page."));

    QPushButton* allowButton = box.addButton(tr("Allow"), QMessageBox::AcceptRole);
    QPushButton* blockButton = box.addButton(tr("Block"), QMessageBox::RejectRole);
    box.setDefaultButton(blockButton);
    box.exec();

    const QWebEnginePage::PermissionPolicy policy =
        box.clickedButton() == allowButton ? QWebEnginePage::PermissionGrantedByUser
                                           : QWebEnginePage::PermissionDeniedByUser;

    page()->setFeaturePermission(securityOrigin, feature, policy);
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
