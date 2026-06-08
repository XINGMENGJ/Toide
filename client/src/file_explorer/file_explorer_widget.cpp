#include "file_explorer/file_explorer_widget.h"

#include <QAbstractItemView>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPoint>
#include <QTimer>
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
    treeView_->setContextMenuPolicy(Qt::CustomContextMenu);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(treeView_);

    connect(treeView_, &QTreeView::doubleClicked, this, &FileExplorerWidget::handleDoubleClicked);
    connect(treeView_, &QTreeView::customContextMenuRequested, this, &FileExplorerWidget::showContextMenu);
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

void FileExplorerWidget::noteFileChangedOnDisk(const QString &absoluteFilePath)
{
    if (model_ == nullptr || rootPath_.isEmpty() || absoluteFilePath.isEmpty()) {
        return;
    }
    const QString cleanRoot = QDir::cleanPath(rootPath_);
    const QString cleanFile = QDir::cleanPath(QFileInfo(absoluteFilePath).absoluteFilePath());
    const QString rel = QDir(cleanRoot).relativeFilePath(cleanFile);
    if (rel.isEmpty() || rel.startsWith(QLatin1String(".."))) {
        return;
    }
    const QModelIndex idx = model_->index(cleanFile);
    if (idx.isValid() && QFileInfo::exists(cleanFile)) {
        return;
    }
    QFileInfo fi(absoluteFilePath);
    refreshAfterMutation(fi.absolutePath(), cleanFile);
}

void FileExplorerWidget::refreshAfterMutation(const QString &parentDir, const QString &highlightPath)
{
    if (model_ == nullptr || rootPath_.isEmpty()) {
        return;
    }
    const QString savedRoot = QDir::cleanPath(rootPath_);
    model_->setRootPath(QString());
    const QModelIndex newRoot = model_->setRootPath(savedRoot);
    treeView_->setRootIndex(newRoot);

    const QString parentClean = QDir::cleanPath(QFileInfo(parentDir).absoluteFilePath());
    const QModelIndex parentIdx = model_->index(parentClean);
    if (parentIdx.isValid()) {
        treeView_->expand(parentIdx);
    }
    if (!highlightPath.isEmpty()) {
        const QString h = QDir::cleanPath(QFileInfo(highlightPath).absoluteFilePath());
        const QString rel = QDir(savedRoot).relativeFilePath(h);
        if (!rel.isEmpty() && !rel.startsWith(QLatin1String(".."))) {
            const QModelIndex hi = model_->index(h);
            if (hi.isValid()) {
                treeView_->setCurrentIndex(hi);
                treeView_->scrollTo(hi, QAbstractItemView::EnsureVisible);
            }
        }
        // 模型异步刷新时 index 可能暂时无效，稍后再对齐一次选中项与滚动位置
        QTimer::singleShot(200, this, [this, h, savedRoot]() {
            if (model_ == nullptr || treeView_ == nullptr) {
                return;
            }
            QModelIndex hi = model_->index(h);
            if (!hi.isValid()) {
                model_->setRootPath(QString());
                const QModelIndex newRoot = model_->setRootPath(savedRoot);
                treeView_->setRootIndex(newRoot);
                hi = model_->index(h);
            }
            if (hi.isValid()) {
                QModelIndex p = hi.parent();
                while (p.isValid()) {
                    treeView_->expand(p);
                    p = p.parent();
                }
                treeView_->setCurrentIndex(hi);
                treeView_->scrollTo(hi, QAbstractItemView::EnsureVisible);
            }
        });
    }
}

QString FileExplorerWidget::createFileInDirectory(const QString &directoryPath,
                                                  const QString &name,
                                                  QString *errorMessage)
{
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }
    const QString cleanName = trimmedName(name);
    if (!isValidEntryName(cleanName)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("文件名不能为空，且不能包含路径分隔符。");
        }
        return {};
    }
    const QFileInfo dirInfo(directoryPath);
    if (!dirInfo.exists() || !dirInfo.isDir()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("目标文件夹不存在。");
        }
        return {};
    }
    const QString path = QDir(dirInfo.absoluteFilePath()).filePath(cleanName);
    if (QFileInfo::exists(path)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("同名文件或文件夹已存在。");
        }
        return {};
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("无法创建文件：%1").arg(file.errorString());
        }
        return {};
    }
    file.close();
    refreshAfterMutation(dirInfo.absoluteFilePath(), path);
    emit fileOpenRequested(path);
    return path;
}

QString FileExplorerWidget::createFolderInDirectory(const QString &directoryPath,
                                                    const QString &name,
                                                    QString *errorMessage)
{
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }
    const QString cleanName = trimmedName(name);
    if (!isValidEntryName(cleanName)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("文件夹名不能为空，且不能包含路径分隔符。");
        }
        return {};
    }
    const QFileInfo dirInfo(directoryPath);
    if (!dirInfo.exists() || !dirInfo.isDir()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("目标文件夹不存在。");
        }
        return {};
    }
    const QString path = QDir(dirInfo.absoluteFilePath()).filePath(cleanName);
    if (QFileInfo::exists(path)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("同名文件或文件夹已存在。");
        }
        return {};
    }
    if (!QDir(dirInfo.absoluteFilePath()).mkdir(cleanName)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("无法创建文件夹。");
        }
        return {};
    }
    refreshAfterMutation(dirInfo.absoluteFilePath(), path);
    return path;
}

void FileExplorerWidget::handleDoubleClicked(const QModelIndex &index)
{
    const auto filePath = model_->filePath(index);
    if (QFileInfo(filePath).isFile()) {
        emit fileOpenRequested(filePath);
    }
}

void FileExplorerWidget::showContextMenu(const QPoint &pos)
{
    if (rootPath_.isEmpty()) {
        return;
    }
    // 右键不会改变 currentIndex，若仍指向旧节点，新建会落到错误目录。
    const QModelIndex at = treeView_->indexAt(pos);
    if (at.isValid()) {
        treeView_->setCurrentIndex(at);
    } else {
        const QModelIndex rootIdx = model_->index(rootPath_);
        if (rootIdx.isValid()) {
            treeView_->setCurrentIndex(rootIdx);
        }
    }
    QMenu menu(this);
    menu.addAction(QStringLiteral("新建文件…"), this, &FileExplorerWidget::promptCreateFile);
    menu.addAction(QStringLiteral("新建文件夹…"), this, &FileExplorerWidget::promptCreateFolder);
    menu.exec(treeView_->viewport()->mapToGlobal(pos));
}

void FileExplorerWidget::promptCreateFile()
{
    const QString targetDir = selectedTargetDirectory();
    if (targetDir.isEmpty()) {
        return;
    }
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("新建文件"), QStringLiteral("文件名："),
                                               QLineEdit::Normal, QString(), &ok);
    if (!ok) {
        return;
    }
    QString error;
    const QString path = createFileInDirectory(targetDir, name, &error);
    if (path.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("新建文件失败"), error);
    }
}

void FileExplorerWidget::promptCreateFolder()
{
    const QString targetDir = selectedTargetDirectory();
    if (targetDir.isEmpty()) {
        return;
    }
    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("新建文件夹"), QStringLiteral("文件夹名："),
                                               QLineEdit::Normal, QString(), &ok);
    if (!ok) {
        return;
    }
    QString error;
    const QString path = createFolderInDirectory(targetDir, name, &error);
    if (path.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("新建文件夹失败"), error);
    }
}

QString FileExplorerWidget::selectedTargetDirectory() const
{
    const QModelIndex index = treeView_->currentIndex();
    if (index.isValid()) {
        const QString selected = model_->filePath(index);
        const QFileInfo info(selected);
        if (info.isDir()) {
            return info.absoluteFilePath();
        }
        return info.absolutePath();
    }
    return rootPath_;
}

bool FileExplorerWidget::isValidEntryName(const QString &name)
{
    return !name.isEmpty() && !name.contains(QLatin1Char('/')) && !name.contains(QLatin1Char('\\'));
}

QString FileExplorerWidget::trimmedName(const QString &name)
{
    return name.trimmed();
}
