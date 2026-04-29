#pragma once

#include <QMainWindow>

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void createActions();
    void createLayout();

    QAction *openProjectAction_ = nullptr;
    QAction *exitAction_ = nullptr;
};
