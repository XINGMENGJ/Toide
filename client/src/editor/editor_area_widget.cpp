#include "editor/editor_area_widget.h"

#include "editor/editor_tab.h"

#include <QFileInfo>
#include <QTabWidget>
#include <QVBoxLayout>

EditorAreaWidget::EditorAreaWidget(QWidget *parent)
    : QWidget(parent)
    , tabs_(new QTabWidget(this))
{
    tabs_->setObjectName(QStringLiteral("editorTabs"));
    tabs_->setTabsClosable(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tabs_);

    connect(tabs_, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QWidget *widget = tabs_->widget(index);
        tabs_->removeTab(index);
        widget->deleteLater();
        const auto *ed = qobject_cast<EditorTab *>(tabs_->currentWidget());
        emit currentFilePathChanged(ed != nullptr ? ed->filePath() : QString());
    });

    connect(tabs_, &QTabWidget::currentChanged, this, [this](int) {
        const auto *ed = qobject_cast<EditorTab *>(tabs_->currentWidget());
        emit currentFilePathChanged(ed != nullptr ? ed->filePath() : QString());
    });
}

QString EditorAreaWidget::currentFilePath() const
{
    const auto *ed = qobject_cast<EditorTab *>(tabs_->currentWidget());
    return ed != nullptr ? ed->filePath() : QString();
}

bool EditorAreaWidget::openFile(const QString &filePath)
{
    const auto existingIndex = findTabByFilePath(filePath);
    if (existingIndex >= 0) {
        tabs_->setCurrentIndex(existingIndex);
        emit currentFilePathChanged(filePath);
        return true;
    }

    auto *editor = new EditorTab(tabs_);
    if (!editor->loadFile(filePath)) {
        editor->deleteLater();
        return false;
    }

    const auto index = tabs_->addTab(editor, QFileInfo(filePath).fileName());
    tabs_->setCurrentIndex(index);

    connect(editor, &EditorTab::dirtyChanged, this, [this, editor](bool) {
        updateTabTitle(tabs_->indexOf(editor));
    });
    connect(editor, &EditorTab::textEdited, this, &EditorAreaWidget::fileTextEdited);
    connect(editor, &EditorTab::cursorMoved, this, &EditorAreaWidget::cursorPositionChanged);

    emit currentFilePathChanged(filePath);
    return true;
}

bool EditorAreaWidget::openFileAt(const QString &filePath, int line, int column)
{
    if (!openFile(filePath)) {
        return false;
    }

    auto *editor = qobject_cast<EditorTab *>(tabs_->currentWidget());
    if (editor == nullptr) {
        return false;
    }

    editor->moveCursorTo(line, column);
    emit currentFilePathChanged(filePath);
    return true;
}

bool EditorAreaWidget::saveCurrentFile()
{
    auto *editor = qobject_cast<EditorTab *>(tabs_->currentWidget());
    if (editor == nullptr) {
        return false;
    }

    const auto saved = editor->save();
    if (saved) {
        updateTabTitle(tabs_->currentIndex());
        emit currentFileSaved(editor->filePath());
    }

    return saved;
}

void EditorAreaWidget::applyRemoteFileText(const QString &absolutePath, const QString &text)
{
    const int index = findTabByFilePath(absolutePath);
    if (index < 0) {
        return;
    }
    auto *editor = qobject_cast<EditorTab *>(tabs_->widget(index));
    if (editor == nullptr) {
        return;
    }
    editor->applyRemoteText(text);
    updateTabTitle(index);
}

void EditorAreaWidget::showRemoteCursor(const QString &absolutePath, const QString &username, int line, int column)
{
    const int index = findTabByFilePath(absolutePath);
    if (index < 0) {
        return;
    }
    auto *editor = qobject_cast<EditorTab *>(tabs_->widget(index));
    if (editor == nullptr) {
        return;
    }
    editor->showRemoteActivity(username, line, column);
}

int EditorAreaWidget::findTabByFilePath(const QString &filePath) const
{
    for (int index = 0; index < tabs_->count(); ++index) {
        const auto *editor = qobject_cast<EditorTab *>(tabs_->widget(index));
        if (editor != nullptr && editor->filePath() == filePath) {
            return index;
        }
    }

    return -1;
}

void EditorAreaWidget::setServerDocVersionForPath(const QString &absolutePath, qint64 version)
{
    const int index = findTabByFilePath(absolutePath);
    if (index < 0) {
        return;
    }
    auto *editor = qobject_cast<EditorTab *>(tabs_->widget(index));
    if (editor != nullptr) {
        editor->setServerDocVersion(version);
    }
}

qint64 EditorAreaWidget::serverDocVersionForPath(const QString &absolutePath) const
{
    const int index = findTabByFilePath(absolutePath);
    if (index < 0) {
        return -1;
    }
    const auto *editor = qobject_cast<EditorTab *>(tabs_->widget(index));
    return editor != nullptr ? editor->serverDocVersion() : -1;
}

void EditorAreaWidget::updateTabTitle(int index)
{
    if (index < 0) {
        return;
    }

    const auto *editor = qobject_cast<EditorTab *>(tabs_->widget(index));
    if (editor == nullptr) {
        return;
    }

    auto title = QFileInfo(editor->filePath()).fileName();
    if (editor->isDirty()) {
        title.append(QStringLiteral("*"));
    }

    tabs_->setTabText(index, title);
}
