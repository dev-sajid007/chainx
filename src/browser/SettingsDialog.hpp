#pragma once

#include <QDialog>
#include <QString>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;

namespace chainx {

class Settings;

/**
 * Preferences page for the implemented browser settings: homepage, search
 * engine, download directory, history retention and bookmark bar visibility.
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(Settings& settings, QWidget* parent = nullptr);

signals:
    void settingsChanged();

private:
    void buildUi();
    void loadFromSettings();
    void apply();
    void chooseDownloadDirectory();
    void selectEngineForTemplate(const QString& searchTemplate);

    Settings& m_settings;

    QLineEdit* m_homepage = nullptr;
    QComboBox* m_searchEngine = nullptr;
    QLineEdit* m_searchTemplate = nullptr;
    QLineEdit* m_downloadDirectory = nullptr;
    QSpinBox* m_historyRetention = nullptr;
    QCheckBox* m_bookmarkBar = nullptr;
    QCheckBox* m_javascript = nullptr;
    QCheckBox* m_blockInsecureContent = nullptr;
};

} // namespace chainx
