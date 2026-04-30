#pragma once

#include <QWidget>

class QTabWidget;

class EditorAreaWidget final : public QWidget {
    Q_OBJECT

public:
    explicit EditorAreaWidget(QWidget *parent = nullptr);

    [[nodiscard]] QString currentFilePath() const;

signals:
    void currentFilePathChanged(const QString &absolutePath);
    void currentFileSaved(const QString &absolutePath);

public slots:
    bool openFile(const QString &filePath);
    bool openFileAt(const QString &filePath, int line, int column);
    bool saveCurrentFile();

private:
    [[nodiscard]] int findTabByFilePath(const QString &filePath) const;
    void updateTabTitle(int index);

    QTabWidget *tabs_ = nullptr;
};
