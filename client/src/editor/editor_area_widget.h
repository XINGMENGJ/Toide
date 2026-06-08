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
    void fileTextEdited(const QString &absolutePath, const QString &text);
    void cursorPositionChanged(const QString &absolutePath, int line, int column);

public slots:
    bool openFile(const QString &filePath);
    bool openFileAt(const QString &filePath, int line, int column);
    bool saveCurrentFile();
    void closeAllTabs();
    void applyRemoteFileText(const QString &absolutePath, const QString &text);
    void showRemoteCursor(const QString &absolutePath, const QString &username, int line, int column);
    void setServerDocVersionForPath(const QString &absolutePath, qint64 version);
    [[nodiscard]] qint64 serverDocVersionForPath(const QString &absolutePath) const;

private:
    [[nodiscard]] int findTabByFilePath(const QString &filePath) const;
    void updateTabTitle(int index);

    QTabWidget *tabs_ = nullptr;
};
