#pragma once

#include <QLineEdit>
#include <QUrl>

#include <functional>

class QCompleter;
class QStringListModel;

namespace chainx {

/**
 * Address and search field with URL resolution and history/bookmark
 * autocompletion.
 */
class AddressBar : public QLineEdit
{
    Q_OBJECT

public:
    explicit AddressBar(QWidget* parent = nullptr);

    void setUrl(const QUrl& url);
    QUrl url() const;

    void setSearchTemplate(const QString& searchTemplate);
    void setSuggestionProvider(std::function<QStringList(const QString&)> provider);

    void focusAndSelectAll();

signals:
    void navigateRequested(const QUrl& url);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    void commitCurrentText();
    void updateSuggestions(const QString& text);

    QCompleter* m_completer = nullptr;
    QStringListModel* m_model = nullptr;
    std::function<QStringList(const QString&)> m_suggestionProvider;
    QString m_searchTemplate;
    QUrl m_url;
};

} // namespace chainx
