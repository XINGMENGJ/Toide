#pragma once



#include <QWidget>



#include <QString>



class QLabel;

class QPlainTextEdit;

class QPushButton;



class WorkspaceCompileWidget final : public QWidget {

    Q_OBJECT



public:

    explicit WorkspaceCompileWidget(QWidget *parent = nullptr);



    void setWorkspaceRoot(const QString &absoluteRootPath);



public slots:

    void runCompile();

    void runWorkspaceProgram();



private:

    struct SourceBundle {

        QStringList headers;

        QStringList cSources;

        QStringList cppSources;

    };



    static void collectSources(const QString &includeDir, const QString &srcDir, SourceBundle *out);

    void appendLine(const QString &line);

    bool ensureWorkspaceDirs(QString *includeDir, QString *srcDir);



    QString workspaceRoot_;

    QLabel *hintLabel_ = nullptr;

    QPushButton *compileButton_ = nullptr;

    QPushButton *runButton_ = nullptr;

    QPlainTextEdit *outputView_ = nullptr;

};

