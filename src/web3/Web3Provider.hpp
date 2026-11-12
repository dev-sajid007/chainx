#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>

class QWebEnginePage;

namespace chainx {

/**
 * Web3 integration entry point.
 *
 * Milestone 05 will add an EIP-1193 provider bridge. Until the security model
 * has been validated, Web3 injection is intentionally disabled: ChainX only
 * *detects* pages that already expose an injected provider (for example via a
 * browser extension) and never exposes wallet secrets or native APIs.
 */
class Web3Provider : public QObject
{
    Q_OBJECT

public:
    explicit Web3Provider(QObject* parent = nullptr);

    /// Always false in v1.0.0. Enabling this requires the security work
    /// described in SPEC.md section 9.
    bool isEnabled() const;

    /// Asynchronously checks whether the page exposes window.ethereum.
    void probe(QWebEnginePage* page);

    bool isDetected(QWebEnginePage* page) const;

    /// Origin of the most recent page where a provider was detected.
    QString lastDetectedOrigin() const;

signals:
    void detectionChanged(QWebEnginePage* page, bool detected);

private:
    void setDetected(QWebEnginePage* page, bool detected);

    bool m_enabled = false;
    QHash<QWebEnginePage*, bool> m_detected;
    QSet<QWebEnginePage*> m_watched;
    QString m_lastDetectedOrigin;
};

} // namespace chainx
