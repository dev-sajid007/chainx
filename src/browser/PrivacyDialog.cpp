#include "browser/PrivacyDialog.hpp"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>

#include "services/DownloadService.hpp"
#include "services/HistoryService.hpp"

namespace chainx {

PrivacyDialog::PrivacyDialog(QWebEngineProfile* profile,
                             HistoryService& history,
                             DownloadService& downloads,
                             QWidget* parent)
    : QDialog(parent)
    , m_profile(profile)
    , m_history(history)
    , m_downloads(downloads)
{
    setWindowTitle(tr("Clear Browsing Data — ChainX"));
    setMinimumWidth(460);

    auto* layout = new QVBoxLayout(this);

    auto* intro = new QLabel(
        tr("Select the data you want to remove from this browser profile."), this);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    m_historyCheck = new QCheckBox(tr("Browsing history"), this);
    m_historyCheck->setChecked(true);
    layout->addWidget(m_historyCheck);

    m_downloadsCheck = new QCheckBox(tr("Download history"), this);
    m_downloadsCheck->setChecked(true);
    layout->addWidget(m_downloadsCheck);

    m_cookiesCheck = new QCheckBox(tr("Cookies and site data"), this);
    m_cookiesCheck->setChecked(true);
    m_cookiesCheck->setEnabled(m_profile != nullptr);
    layout->addWidget(m_cookiesCheck);

    m_cacheCheck = new QCheckBox(tr("Cached files"), this);
    m_cacheCheck->setChecked(true);
    m_cacheCheck->setEnabled(m_profile != nullptr);
    layout->addWidget(m_cacheCheck);

    if (m_profile) {
        const QString path = m_profile->isOffTheRecord()
                                 ? tr("Private (in-memory profile)")
                                 : m_profile->persistentStoragePath();
        auto* profileLabel = new QLabel(tr("Profile: %1").arg(path), this);
        profileLabel->setWordWrap(true);
        profileLabel->setStyleSheet(QStringLiteral("color: #6b7385; font-size: 11px;"));
        layout->addWidget(profileLabel);
    }

    layout->addStretch(1);

    auto* buttons = new QDialogButtonBox(this);
    auto* clearButton =
        buttons->addButton(tr("Clear Selected…"), QDialogButtonBox::AcceptRole);
    buttons->addButton(QDialogButtonBox::Cancel);
    clearButton->setDefault(true);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &PrivacyDialog::clearSelected);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void PrivacyDialog::clearSelected()
{
    const bool clearHistoryData = m_historyCheck->isChecked();
    const bool clearDownloads = m_downloadsCheck->isChecked();
    const bool clearCookies = m_profile && m_cookiesCheck->isChecked();
    const bool clearCache = m_profile && m_cacheCheck->isChecked();

    if (!clearHistoryData && !clearDownloads && !clearCookies && !clearCache) {
        QMessageBox::information(this,
                                 tr("Nothing Selected"),
                                 tr("Select at least one type of data to clear."));
        return;
    }

    const QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        tr("Clear Browsing Data"),
        tr("This cannot be undone. Continue?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (answer != QMessageBox::Yes) {
        return;
    }

    if (clearHistoryData) {
        m_history.clear();
    }
    if (clearDownloads) {
        m_downloads.clearHistory();
    }
    if (clearCookies) {
        m_profile->cookieStore()->deleteAllCookies();
    }
    if (clearCache) {
        m_profile->clearHttpCache();
    }

    emit dataCleared();
    accept();
}

} // namespace chainx
