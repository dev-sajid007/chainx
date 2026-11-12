#include "browser/SettingsDialog.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "settings/Settings.hpp"
#include "utils/UrlUtils.hpp"

namespace chainx {
namespace {

constexpr auto kBookmarkBarKey = "ui/bookmarkToolbarVisible";

struct SearchEngine
{
    const char* name;
    const char* templateUrl;
};

const SearchEngine kSearchEngines[] = {
    {"DuckDuckGo", "https://duckduckgo.com/?q=%1"},
    {"Google", "https://www.google.com/search?q=%1"},
    {"Bing", "https://www.bing.com/search?q=%1"},
    {"Brave Search", "https://search.brave.com/search?q=%1"},
    {"Startpage", "https://www.startpage.com/sp/search?query=%1"},
};

} // namespace

SettingsDialog::SettingsDialog(Settings& settings, QWidget* parent)
    : QDialog(parent)
    , m_settings(settings)
{
    buildUi();
    loadFromSettings();
}

void SettingsDialog::buildUi()
{
    setWindowTitle(tr("Settings — ChainX"));
    setMinimumWidth(560);

    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_homepage = new QLineEdit(this);
    m_homepage->setPlaceholderText(UrlUtils::newTabUrl().toString());
    form->addRow(tr("Home page:"), m_homepage);

    m_searchEngine = new QComboBox(this);
    for (const SearchEngine& engine : kSearchEngines) {
        m_searchEngine->addItem(QString::fromLatin1(engine.name),
                                QString::fromLatin1(engine.templateUrl));
    }
    m_searchEngine->addItem(tr("Custom"), QString());
    form->addRow(tr("Search engine:"), m_searchEngine);

    m_searchTemplate = new QLineEdit(this);
    m_searchTemplate->setPlaceholderText(QStringLiteral("https://example.com/search?q=%1"));
    form->addRow(tr("Search URL:"), m_searchTemplate);

    auto* downloadRow = new QWidget(this);
    auto* downloadLayout = new QHBoxLayout(downloadRow);
    downloadLayout->setContentsMargins(0, 0, 0, 0);
    m_downloadDirectory = new QLineEdit(downloadRow);
    auto* browseButton = new QPushButton(tr("Browse…"), downloadRow);
    downloadLayout->addWidget(m_downloadDirectory, 1);
    downloadLayout->addWidget(browseButton);
    form->addRow(tr("Download folder:"), downloadRow);

    m_historyRetention = new QSpinBox(this);
    m_historyRetention->setRange(0, 3650);
    m_historyRetention->setSuffix(tr(" days"));
    m_historyRetention->setSpecialValueText(tr("Keep forever"));
    form->addRow(tr("Keep history for:"), m_historyRetention);

    m_bookmarkBar = new QCheckBox(tr("Show the bookmarks bar"), this);
    form->addRow(QString(), m_bookmarkBar);

    m_javascript = new QCheckBox(tr("Enable JavaScript"), this);
    form->addRow(QString(), m_javascript);

    m_blockInsecureContent =
        new QCheckBox(tr("Block insecure content on secure pages"), this);
    form->addRow(QString(), m_blockInsecureContent);

    layout->addLayout(form);

    auto* hint = new QLabel(
        tr("Search URL must contain %1 where the query is inserted."), this);
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color: #6b7385; font-size: 11px;"));
    layout->addWidget(hint);
    layout->addStretch(1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                             | QDialogButtonBox::Cancel,
                                         this);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::apply);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(browseButton, &QPushButton::clicked,
            this, &SettingsDialog::chooseDownloadDirectory);
    connect(m_searchEngine, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                const QString templ = m_searchEngine->itemData(index).toString();
                if (!templ.isEmpty()) {
                    m_searchTemplate->setText(templ);
                }
            });
}

void SettingsDialog::loadFromSettings()
{
    m_homepage->setText(m_settings.homepage());
    m_downloadDirectory->setText(m_settings.downloadDirectory());
    m_historyRetention->setValue(m_settings.historyRetentionDays());
    m_bookmarkBar->setChecked(
        m_settings.value(QString::fromLatin1(kBookmarkBarKey), true).toBool());
    m_javascript->setChecked(m_settings.javascriptEnabled());
    m_blockInsecureContent->setChecked(m_settings.blockInsecureContent());

    const QString templ = m_settings.searchEngineTemplate();
    m_searchTemplate->setText(templ);
    selectEngineForTemplate(templ);
}

void SettingsDialog::selectEngineForTemplate(const QString& searchTemplate)
{
    for (int index = 0; index < m_searchEngine->count(); ++index) {
        const QString itemTemplate = m_searchEngine->itemData(index).toString();
        if (!itemTemplate.isEmpty() && itemTemplate == searchTemplate) {
            m_searchEngine->setCurrentIndex(index);
            return;
        }
    }
    m_searchEngine->setCurrentIndex(m_searchEngine->count() - 1); // Custom
}

void SettingsDialog::chooseDownloadDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(
        this,
        tr("Choose Download Folder"),
        m_downloadDirectory->text().isEmpty() ? m_settings.downloadDirectory()
                                              : m_downloadDirectory->text());
    if (!directory.isEmpty()) {
        m_downloadDirectory->setText(directory);
    }
}

void SettingsDialog::apply()
{
    const QString searchTemplate = m_searchTemplate->text().trimmed();
    if (!searchTemplate.contains(QStringLiteral("%1"))) {
        QMessageBox::warning(this,
                             tr("Invalid Search URL"),
                             tr("The search URL must contain %1 where the query is "
                                "inserted."));
        return;
    }

    const QString downloadDirectory = m_downloadDirectory->text().trimmed();
    if (!downloadDirectory.isEmpty() && !QDir().mkpath(downloadDirectory)) {
        QMessageBox::warning(this,
                             tr("Invalid Download Folder"),
                             tr("The download folder could not be created:\n%1")
                                 .arg(downloadDirectory));
        return;
    }

    QString homepage = m_homepage->text().trimmed();
    if (homepage.isEmpty()) {
        homepage = UrlUtils::newTabUrl().toString();
    } else {
        const QUrl resolved = UrlUtils::resolveInput(homepage);
        if (!resolved.isValid() || resolved.isEmpty()) {
            QMessageBox::warning(this,
                                 tr("Invalid Home Page"),
                                 tr("The home page is not a valid address."));
            return;
        }
        homepage = resolved.toString();
    }

    m_settings.setHomepage(homepage);
    m_settings.setSearchEngine(m_searchEngine->currentText(), searchTemplate);
    m_settings.setDownloadDirectory(downloadDirectory);
    m_settings.setHistoryRetentionDays(m_historyRetention->value());
    m_settings.setValue(QString::fromLatin1(kBookmarkBarKey), m_bookmarkBar->isChecked());
    m_settings.setJavascriptEnabled(m_javascript->isChecked());
    m_settings.setBlockInsecureContent(m_blockInsecureContent->isChecked());
    m_settings.sync();

    emit settingsChanged();
    accept();
}

} // namespace chainx
