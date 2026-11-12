#include "navigation/AddressBar.hpp"

#include <QAbstractItemView>
#include <QCompleter>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QStringListModel>

#include "utils/UrlUtils.hpp"

namespace chainx {

AddressBar::AddressBar(QWidget* parent)
    : QLineEdit(parent)
    , m_searchTemplate(QString::fromLatin1(UrlUtils::DefaultSearchTemplate))
{
    setObjectName(QStringLiteral("addressBar"));
    setPlaceholderText(tr("Search or enter address"));
    setClearButtonEnabled(true);

    m_model = new QStringListModel(this);

    m_completer = new QCompleter(m_model, this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchContains);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setMaxVisibleItems(8);
    setCompleter(m_completer);

    connect(m_completer, QOverload<const QString&>::of(&QCompleter::activated),
            this, [this](const QString& text) {
                setText(text);
                commitCurrentText();
            });

    connect(this, &QLineEdit::textEdited, this, &AddressBar::updateSuggestions);
}

void AddressBar::setUrl(const QUrl& url)
{
    m_url = url;
    if (!hasFocus()) {
        const QString text = UrlUtils::displayString(url);
        setText(text);
        setCursorPosition(0);
    }
}

QUrl AddressBar::url() const
{
    return m_url;
}

void AddressBar::setSearchTemplate(const QString& searchTemplate)
{
    if (!searchTemplate.isEmpty()) {
        m_searchTemplate = searchTemplate;
    }
}

void AddressBar::setSuggestionProvider(std::function<QStringList(const QString&)> provider)
{
    m_suggestionProvider = std::move(provider);
}

void AddressBar::focusAndSelectAll()
{
    setFocus(Qt::ShortcutFocusReason);
    selectAll();
}

void AddressBar::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        if (m_completer->popup()->isVisible()) {
            m_completer->popup()->hide();
        } else {
            setText(UrlUtils::displayString(m_url));
            clearFocus();
        }
        event->accept();
        return;

    case Qt::Key_Return:
    case Qt::Key_Enter: {
        if (m_completer->popup()->isVisible()) {
            const QModelIndex index = m_completer->popup()->currentIndex();
            if (index.isValid()) {
                setText(index.data().toString());
            }
            m_completer->popup()->hide();
        }
        commitCurrentText();
        event->accept();
        return;
    }

    default:
        break;
    }

    QLineEdit::keyPressEvent(event);
}

void AddressBar::focusInEvent(QFocusEvent* event)
{
    QLineEdit::focusInEvent(event);
    selectAll();
}

void AddressBar::focusOutEvent(QFocusEvent* event)
{
    QLineEdit::focusOutEvent(event);
    if (m_completer->popup()->isVisible()) {
        m_completer->popup()->hide();
    }
    setText(UrlUtils::displayString(m_url));
}

void AddressBar::commitCurrentText()
{
    const QUrl resolved = UrlUtils::resolveInput(text(), m_searchTemplate);
    if (!resolved.isValid() || resolved.isEmpty()) {
        return;
    }

    m_url = resolved;
    setText(UrlUtils::displayString(resolved));
    setCursorPosition(0);
    emit navigateRequested(resolved);
    clearFocus();
}

void AddressBar::updateSuggestions(const QString& text)
{
    if (!m_suggestionProvider || text.trimmed().isEmpty()) {
        m_completer->popup()->hide();
        return;
    }

    QStringList suggestions = m_suggestionProvider(text.trimmed());
    suggestions.removeDuplicates();

    if (suggestions.isEmpty()) {
        m_completer->popup()->hide();
        return;
    }

    m_model->setStringList(suggestions);
    m_completer->setCompletionPrefix(text.trimmed());
    m_completer->complete();
}

} // namespace chainx
