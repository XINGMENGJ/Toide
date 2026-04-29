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
    });
}

bool EditorAreaWidget::openFile(const QString &filePath)
{
    const auto existingIndex = findTabByFilePath(filePath);
    if (existingIndex >= 0) {
        tabs_->setCurrentIndex(existingIndex);
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
    }

    return saved;
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
