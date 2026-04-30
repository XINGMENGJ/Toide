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
    void moveCursorTo(int line, int column);
    void applyRemoteText(const QString &text);
    void showRemoteActivity(const QString &username, int line, int column);

signals:
    void dirtyChanged(bool dirty);
    void textEdited(const QString &filePath, const QString &text);
    void cursorMoved(const QString &filePath, int line, int column);

private:
    void setDirty(bool dirty);
    void emitCursorPosition();

    QPlainTextEdit *editor_ = nullptr;
    QString filePath_;
    bool dirty_ = false;
};
