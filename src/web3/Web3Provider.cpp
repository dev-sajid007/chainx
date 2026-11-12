#include "web3/Web3Provider.hpp"

#include <QPointer>
#include <QUrl>
#include <QVariant>
#include <QWebEnginePage>

namespace chainx {
namespace {

const char* kProbeScript = R"JS(
(function () {
    try {
        return typeof window.ethereum !== 'undefined' && window.ethereum !== null;
    } catch (error) {
        return false;
    }
})();
)JS";

} // namespace

Web3Provider::Web3Provider(QObject* parent)
    : QObject(parent)
{
}

bool Web3Provider::isEnabled() const
{
    return m_enabled;
}

void Web3Provider::probe(QWebEnginePage* page)
{
    if (!page) {
        return;
    }

    QPointer<QWebEnginePage> guard(page);
    if (!m_watched.contains(page)) {
        m_watched.insert(page);
        connect(page, &QObject::destroyed, this, [this, page] {
            m_watched.remove(page);
            m_detected.remove(page);
        });
    }
    page->runJavaScript(QString::fromLatin1(kProbeScript),
                        [this, guard](const QVariant& result) {
                            if (!guard) {
                                return;
                            }
                            setDetected(guard, result.toBool());
                        });
}

bool Web3Provider::isDetected(QWebEnginePage* page) const
{
    return page && m_detected.value(page, false);
}

QString Web3Provider::lastDetectedOrigin() const
{
    return m_lastDetectedOrigin;
}

void Web3Provider::setDetected(QWebEnginePage* page, bool detected)
{
    if (!page) {
        return;
    }

    const bool previous = m_detected.value(page, false);
    m_detected.insert(page, detected);

    if (detected) {
        QUrl origin = page->url();
        origin.setPath(QString());
        origin.setQuery(QString());
        origin.setFragment(QString());
        m_lastDetectedOrigin = origin.toString();
    }

    if (previous != detected) {
        emit detectionChanged(page, detected);
    }
}

} // namespace chainx
