#pragma once

#include <QWidget>

class QPaintEvent;
class QPainter;
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
    void setServerDocVersion(qint64 version);
    [[nodiscard]] qint64 serverDocVersion() const;

signals:
    void dirtyChanged(bool dirty);
    void textEdited(const QString &filePath, const QString &text);
    void cursorMoved(const QString &filePath, int line, int column);

private:
    class LineNumberArea;
    friend class LineNumberArea;
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    void setDirty(bool dirty);
    void emitCursorPosition();

    QPlainTextEdit *editor_ = nullptr;
    LineNumberArea *lineNumberArea_ = nullptr;
    QString filePath_;
    bool dirty_ = false;
    qint64 serverDocVersion_{-1};
    int remoteActivityLine_{0}; // 1-based; 0 = none（协作者的行，用于行号栏标记）
    QString remoteActivityUser_; // 当前协作者昵称（用于按人分配颜色）
};
