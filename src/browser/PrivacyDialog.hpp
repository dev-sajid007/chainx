#pragma once

#include <QDialog>

class QCheckBox;
class QWebEngineProfile;

namespace chainx {

class DownloadService;
class HistoryService;

/**
 * "Clear browsing data" dialog for the current window's profile. Supports
 * history, download history, cookies/site data and the HTTP cache.
 */
class PrivacyDialog : public QDialog
{
    Q_OBJECT

public:
    PrivacyDialog(QWebEngineProfile* profile,
                  HistoryService& history,
                  DownloadService& downloads,
                  QWidget* parent = nullptr);

signals:
    void dataCleared();

private:
    void clearSelected();

    QWebEngineProfile* m_profile = nullptr;
    HistoryService& m_history;
    DownloadService& m_downloads;

    QCheckBox* m_historyCheck = nullptr;
    QCheckBox* m_downloadsCheck = nullptr;
    QCheckBox* m_cookiesCheck = nullptr;
    QCheckBox* m_cacheCheck = nullptr;
};

} // namespace chainx
