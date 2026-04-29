#pragma once

#include <QWidget>

class QPlainTextEdit;

class EditorTab final : public QWidget {
    Q_OBJECT

public:
    explicit EditorTab(QWidget *parent = nullptr);

    [[nodiscard]] QString filePath() const;
    [[nodiscard]] QString text() const;
    [[nodiscard]] bool isDirty() const;

    bool loadFile(const QString &filePath);
    bool save();
    void setText(const QString &text);

signals:
    void dirtyChanged(bool dirty);

private:
    void setDirty(bool dirty);

    QPlainTextEdit *editor_ = nullptr;
    QString filePath_;
    bool dirty_ = false;
};
