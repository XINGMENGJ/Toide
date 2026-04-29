#pragma once

#include <QWidget>

class QTabWidget;

class EditorAreaWidget final : public QWidget {
    Q_OBJECT

public:
    explicit EditorAreaWidget(QWidget *parent = nullptr);

public slots:
    bool openFile(const QString &filePath);

private:
    [[nodiscard]] int findTabByFilePath(const QString &filePath) const;
    void updateTabTitle(int index);

    QTabWidget *tabs_ = nullptr;
};
