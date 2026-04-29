#include "file_explorer/file_explorer_widget.h"

#include <QFileInfo>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QTreeView>

FileExplorerWidget::FileExplorerWidget(QWidget *parent)
    : QWidget(parent)
    , model_(new QFileSystemModel(this))
    , treeView_(new QTreeView(this))
{
    setObjectName(QStringLiteral("fileExplorerPanel"));

    model_->setRootPath({});

    treeView_->setModel(model_);
    treeView_->setHeaderHidden(false);
    treeView_->setAnimated(true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(treeView_);

    connect(treeView_, &QTreeView::doubleClicked, this, &FileExplorerWidget::handleDoubleClicked);
}

QString FileExplorerWidget::rootPath() const
{
    return rootPath_;
}

void FileExplorerWidget::setProjectRoot(const QString &projectPath)
{
    const QFileInfo projectInfo(projectPath);
    if (!projectInfo.exists() || !projectInfo.isDir()) {
        return;
    }

    rootPath_ = projectInfo.absoluteFilePath();
    const auto rootIndex = model_->setRootPath(rootPath_);
    treeView_->setRootIndex(rootIndex);
}

void FileExplorerWidget::handleDoubleClicked(const QModelIndex &index)
{
    const auto filePath = model_->filePath(index);
    if (QFileInfo(filePath).isFile()) {
        emit fileOpenRequested(filePath);
    }
}
