#include "editor/editor_tab.h"

#include <QFile>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QTextBlock>
#include <QTextCursor>
#include <QVBoxLayout>

EditorTab::EditorTab(QWidget *parent)
    : QWidget(parent)
    , editor_(new QPlainTextEdit(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(editor_);

    connect(editor_, &QPlainTextEdit::textChanged, this, [this]() {
        setDirty(true);
    });
}

QString EditorTab::filePath() const
{
    return filePath_;
}

QString EditorTab::text() const
{
    return editor_->toPlainText();
}

bool EditorTab::isDirty() const
{
    return dirty_;
}

bool EditorTab::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    filePath_ = filePath;
    const auto content = QString::fromUtf8(file.readAll());

    const QSignalBlocker blocker(editor_);
    editor_->setPlainText(content);
    setDirty(false);
    return true;
}

bool EditorTab::save()
{
    if (filePath_.isEmpty()) {
        return false;
    }

    QFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    file.write(text().toUtf8());
    setDirty(false);
    return true;
}

void EditorTab::setText(const QString &text)
{
    editor_->setPlainText(text);
}

void EditorTab::moveCursorTo(int line, int column)
{
    const auto targetLine = qMax(1, line);
    const auto targetColumn = qMax(1, column);

    const auto block = editor_->document()->findBlockByNumber(targetLine - 1);
    QTextCursor cursor;
    if (!block.isValid()) {
        cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
    } else {
        cursor = QTextCursor(block);
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, targetColumn - 1);
    }

    editor_->setTextCursor(cursor);
    editor_->setFocus();
}

void EditorTab::setDirty(bool dirty)
{
    if (dirty_ == dirty) {
        return;
    }

    dirty_ = dirty;
    emit dirtyChanged(dirty_);
}
