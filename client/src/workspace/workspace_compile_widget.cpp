#include "workspace/workspace_compile_widget.h"

#include <QCoreApplication>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QStandardPaths>
#include <QTime>
#include <QVBoxLayout>

namespace {

QString toolBaseName(const QString &name)
{
#if defined(Q_OS_WIN)
    if (!name.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)) {
        return name + QStringLiteral(".exe");
    }
#endif
    return name;
}

QString resolveCompilerTool(const QString &toolName)
{
    const QString base = QStandardPaths::findExecutable(toolName);
    if (!base.isEmpty()) {
        return base;
    }

    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString exe = toolBaseName(toolName);

    const auto tryBinUnder = [&exe](const QString &root) -> QString {
        if (root.isEmpty()) {
            return {};
        }
        const QString candidate = QDir::cleanPath(QDir(root).filePath(QStringLiteral("bin/") + exe));
        return QFileInfo::exists(candidate) ? candidate : QString{};
    };

    QString fromEnv = tryBinUnder(env.value(QStringLiteral("MINGW_DIR")));
    if (!fromEnv.isEmpty()) {
        return fromEnv;
    }
    fromEnv = tryBinUnder(env.value(QStringLiteral("MINGW_HOME")));
    if (!fromEnv.isEmpty()) {
        return fromEnv;
    }

    const QString cxx = env.value(QStringLiteral("CMAKE_CXX_COMPILER"));
    if (toolName == QStringLiteral("g++") && !cxx.isEmpty() && QFileInfo::exists(cxx)) {
        return QFileInfo(cxx).absoluteFilePath();
    }
    const QString cc = env.value(QStringLiteral("CMAKE_C_COMPILER"));
    if (toolName == QStringLiteral("gcc") && !cc.isEmpty() && QFileInfo::exists(cc)) {
        return QFileInfo(cc).absoluteFilePath();
    }

    const QString qtDir = env.value(QStringLiteral("QTDIR"));
    if (!qtDir.isEmpty()) {
        const QDir kitVersionRoot(QFileInfo(qtDir).absolutePath());
        const QString sibling = QDir::cleanPath(kitVersionRoot.filePath(QStringLiteral("../Tools/mingw1310_64")));
        fromEnv = tryBinUnder(sibling);
        if (!fromEnv.isEmpty()) {
            return fromEnv;
        }
        const QDir toolsRoot(QDir::cleanPath(kitVersionRoot.filePath(QStringLiteral("../Tools"))));
        if (toolsRoot.exists()) {
            const QFileInfoList mingwDirs =
                toolsRoot.entryInfoList(QStringList{QStringLiteral("mingw*")}, QDir::Dirs | QDir::NoDotAndDotDot,
                                        QDir::Name);
            for (const QFileInfo &info : mingwDirs) {
                fromEnv = tryBinUnder(info.absoluteFilePath());
                if (!fromEnv.isEmpty()) {
                    return fromEnv;
                }
            }
        }
    }

#ifdef TOIDE_BUILD_CXX_BIN_DIR
    {
        const QString candidate =
            QDir::cleanPath(QDir(QStringLiteral(TOIDE_BUILD_CXX_BIN_DIR)).filePath(exe));
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
#endif

    return {};
}

bool runCaptured(const QString &program, const QStringList &arguments, QString *merged)
{
    if (merged != nullptr) {
        merged->clear();
    }
    if (program.isEmpty()) {
        if (merged != nullptr) {
            *merged = QStringLiteral("未找到编译器可执行文件。");
        }
        return false;
    }
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(program, arguments);
    if (!p.waitForStarted(8000)) {
        if (merged != nullptr) {
            *merged = QStringLiteral("无法启动 %1。若从资源管理器启动便携版，请将 MinGW 的 bin 加入 PATH，"
                                      "或设置环境变量 MINGW_DIR（指向 MinGW 根目录），"
                                      "或由 Qt 在线安装器安装的套件确保已设置 QTDIR。")
                          .arg(program);
        }
        return false;
    }
    p.waitForFinished(120000);
    if (merged != nullptr) {
        *merged = QString::fromLocal8Bit(p.readAllStandardOutput());
    }
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

void prependPathForMinGwRuntime(QProcessEnvironment *env, const QString &compilerPath)
{
    if (env == nullptr) {
        return;
    }
    const QString pathKey = QStringLiteral("PATH");
    QStringList prefixes;
    if (QCoreApplication::instance() != nullptr) {
        const QString appDir = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
        if (!appDir.isEmpty() && QDir(appDir).exists()) {
            prefixes.append(appDir);
        }
    }
    if (!compilerPath.isEmpty()) {
        const QString binDir = QDir::toNativeSeparators(QFileInfo(compilerPath).absolutePath());
        if (!binDir.isEmpty() && QDir(binDir).exists() && !prefixes.contains(binDir)) {
            prefixes.append(binDir);
        }
    }
    if (prefixes.isEmpty()) {
        return;
    }
    const QString extra = prefixes.join(QString(QDir::listSeparator()));
    env->insert(pathKey, extra + QDir::listSeparator() + env->value(pathKey));
}

QString runExecutableOutputName()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("toide-workspace-run.exe");
#else
    return QStringLiteral("toide-workspace-run");
#endif
}

} // namespace

void WorkspaceCompileWidget::collectSources(const QString &includeDir, const QString &srcDir, SourceBundle *out)
{
    if (out == nullptr) {
        return;
    }
    const auto classifyTree = [&](const QString &root) {
        if (!QDir(root).exists()) {
            return;
        }
        QDirIterator it(root,
                        QDir::Files,
                        QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
        while (it.hasNext()) {
            it.next();
            const QString path = it.filePath();
            const QString lower = path.toLower();
            if (lower.endsWith(QStringLiteral(".c"))) {
                out->cSources.append(path);
            } else if (lower.endsWith(QStringLiteral(".cpp")) || lower.endsWith(QStringLiteral(".cc"))
                       || lower.endsWith(QStringLiteral(".cxx"))) {
                out->cppSources.append(path);
            } else if (lower.endsWith(QStringLiteral(".h")) || lower.endsWith(QStringLiteral(".hpp"))) {
                out->headers.append(path);
            }
        }
    };
    classifyTree(includeDir);
    classifyTree(srcDir);
}

bool WorkspaceCompileWidget::ensureWorkspaceDirs(QString *includeDir, QString *srcDir)
{
    if (workspaceRoot_.isEmpty()) {
        QMessageBox::information(this,
                                 QStringLiteral("无法继续"),
                                 QStringLiteral("请先打开包含 include/ 与 src/ 的工作区。"));
        return false;
    }
    *includeDir = QDir(workspaceRoot_).filePath(QStringLiteral("include"));
    *srcDir = QDir(workspaceRoot_).filePath(QStringLiteral("src"));
    if (!QDir(*includeDir).exists() && !QDir(*srcDir).exists()) {
        QMessageBox::information(this,
                                 QStringLiteral("无法继续"),
                                 QStringLiteral("当前工作区下未找到 include 或 src 目录。"));
        return false;
    }
    return true;
}

WorkspaceCompileWidget::WorkspaceCompileWidget(QWidget *parent)
    : QWidget(parent)
    , hintLabel_(new QLabel(this))
    , compileButton_(new QPushButton(QStringLiteral("编译 include 与 src"), this))
    , runButton_(new QPushButton(QStringLiteral("运行（链接并执行）"), this))
    , outputView_(new QPlainTextEdit(this))
{
    compileButton_->setObjectName(QStringLiteral("workspaceCompileButton"));
    runButton_->setObjectName(QStringLiteral("workspaceRunButton"));
    outputView_->setObjectName(QStringLiteral("workspaceCompileOutput"));
    outputView_->setReadOnly(true);
    QFont mono(QStringLiteral("Consolas"));
    if (!mono.exactMatch()) {
        mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    }
    outputView_->setFont(mono);
    outputView_->setLineWrapMode(QPlainTextEdit::NoWrap);

    hintLabel_->setObjectName(QStringLiteral("workspaceCompileHint"));
    hintLabel_->setWordWrap(true);
    hintLabel_->setTextFormat(Qt::RichText);
    hintLabel_->setText(
        QStringLiteral("<p style='margin:0 0 6px 0'><b>工作区 C/C++</b><br/>"
                       "<span style='color:#444'><b>编译</b>：对 <code>include</code> 与 <code>src</code> 下的 "
                       "<code>.h</code>、<code>.c</code>、<code>.cpp</code> 做 <code>g++ -fsyntax-only</code>。"
                       "<b>运行</b>：用同一批源文件链接为可执行文件（输出在 <code>.toide-build/</code>）并在下方显示进程输出。"
                       "需要工作区内有且仅有一个合理的 <code>main</code>（勿把多个带 main 的文件都放在 <code>src/</code> 下）。"
                       "编译器查找顺序同 PATH / <code>MINGW_DIR</code> / <code>QTDIR</code> / 构建期嵌入路径。</span></p>"));

    auto *top = new QHBoxLayout();
    top->setContentsMargins(0, 0, 0, 0);
    top->addWidget(compileButton_, 0);
    top->addWidget(runButton_, 0);
    top->addStretch(1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(hintLabel_);
    layout->addLayout(top);
    layout->addWidget(outputView_, 1);

    connect(compileButton_, &QPushButton::clicked, this, &WorkspaceCompileWidget::runCompile);
    connect(runButton_, &QPushButton::clicked, this, &WorkspaceCompileWidget::runWorkspaceProgram);
}

void WorkspaceCompileWidget::setWorkspaceRoot(const QString &absoluteRootPath)
{
    if (absoluteRootPath.isEmpty()) {
        workspaceRoot_.clear();
        appendLine(QStringLiteral("[工作区] 未打开项目。"));
        return;
    }
    workspaceRoot_ = QFileInfo(absoluteRootPath).absoluteFilePath();
}

void WorkspaceCompileWidget::appendLine(const QString &line)
{
    outputView_->appendPlainText(line);
}

void WorkspaceCompileWidget::runCompile()
{
    QString includeDir;
    QString srcDir;
    if (!ensureWorkspaceDirs(&includeDir, &srcDir)) {
        return;
    }

    SourceBundle bundle;
    collectSources(includeDir, srcDir, &bundle);

    outputView_->clear();
    const QString stamp = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));
    appendLine(QStringLiteral("[%1] 工作区：%2").arg(stamp, workspaceRoot_));
    appendLine(QStringLiteral("[%1] 头文件 %2，C 源 %3，C++ 源 %4")
                   .arg(stamp)
                   .arg(bundle.headers.size())
                   .arg(bundle.cSources.size())
                   .arg(bundle.cppSources.size()));

    if (bundle.headers.isEmpty() && bundle.cSources.isEmpty() && bundle.cppSources.isEmpty()) {
        appendLine(QStringLiteral("未找到任何 .h/.c/.cpp 文件。"));
        return;
    }

    const QString gxxPath = resolveCompilerTool(QStringLiteral("g++"));
    const QString gccPath = resolveCompilerTool(QStringLiteral("gcc"));
    const bool needGxx = !bundle.headers.isEmpty() || !bundle.cppSources.isEmpty();
    const bool needGcc = !bundle.cSources.isEmpty();
    if (needGxx && gxxPath.isEmpty()) {
        appendLine(QStringLiteral("未找到 g++。请将 MinGW 的 bin 加入用户 PATH，或设置 MINGW_DIR，或使用带 QTDIR 的 Qt 开发环境启动本程序。"));
        return;
    }
    if (needGcc && gccPath.isEmpty() && gxxPath.isEmpty()) {
        appendLine(QStringLiteral("未找到 gcc/g++，无法检查 .c 文件。"));
        return;
    }
    if (!gxxPath.isEmpty()) {
        appendLine(QStringLiteral("g++：%1").arg(gxxPath));
    }
    if (!gccPath.isEmpty() && (!needGxx || gccPath != gxxPath)) {
        appendLine(QStringLiteral("gcc：%1").arg(gccPath));
    }

    QStringList includeArgs;
    if (QDir(includeDir).exists()) {
        includeArgs.append(QStringLiteral("-I"));
        includeArgs.append(includeDir);
    }
    if (QDir(srcDir).exists()) {
        includeArgs.append(QStringLiteral("-I"));
        includeArgs.append(srcDir);
    }

    compileButton_->setEnabled(false);
    runButton_->setEnabled(false);
    bool allOk = true;

    for (const QString &h : bundle.headers) {
        QString out;
        const QStringList args = QStringList()
            << QStringLiteral("-std=c++20") << QStringLiteral("-fno-diagnostics-color")
            << QStringLiteral("-fsyntax-only") << includeArgs << QStringLiteral("-x") << QStringLiteral("c++-header")
            << h;
        const bool ok = runCaptured(gxxPath, args, &out);
        if (!out.trimmed().isEmpty()) {
            appendLine(out.trimmed());
        }
        if (!ok) {
            allOk = false;
            appendLine(QStringLiteral("失败：%1").arg(h));
        }
    }

    if (!bundle.cppSources.isEmpty()) {
        QString out;
        QStringList args = QStringList() << QStringLiteral("-std=c++20") << QStringLiteral("-fno-diagnostics-color")
                                         << QStringLiteral("-fsyntax-only") << includeArgs;
        args.append(bundle.cppSources);
        const bool ok = runCaptured(gxxPath, args, &out);
        if (!out.trimmed().isEmpty()) {
            appendLine(out.trimmed());
        }
        if (!ok) {
            allOk = false;
            appendLine(QStringLiteral("C++ 源文件批处理检查失败。"));
        }
    }

    if (!bundle.cSources.isEmpty()) {
        QString out;
        QStringList args = QStringList() << QStringLiteral("-std=c17") << QStringLiteral("-fno-diagnostics-color")
                                         << QStringLiteral("-fsyntax-only") << includeArgs;
        args.append(bundle.cSources);
        const QString gccExe = gccPath.isEmpty() ? gxxPath : gccPath;
        bool ok = runCaptured(gccExe, args, &out);
        if (!ok) {
            QString out2;
            QStringList args2 = QStringList() << QStringLiteral("-std=c11") << QStringLiteral("-fno-diagnostics-color")
                                              << QStringLiteral("-fsyntax-only") << QStringLiteral("-x")
                                              << QStringLiteral("c") << includeArgs;
            args2.append(bundle.cSources);
            ok = runCaptured(gxxPath, args2, &out2);
            out += out2;
        }
        if (!out.trimmed().isEmpty()) {
            appendLine(out.trimmed());
        }
        if (!ok) {
            allOk = false;
            appendLine(QStringLiteral("C 源文件检查失败。"));
        }
    }

    appendLine(allOk ? QStringLiteral("全部完成（无报告错误即表示语法检查通过）。")
                     : QStringLiteral("已完成，但存在错误。请查看上方输出。"));
    compileButton_->setEnabled(true);
    runButton_->setEnabled(true);
}

void WorkspaceCompileWidget::runWorkspaceProgram()
{
    QString includeDir;
    QString srcDir;
    if (!ensureWorkspaceDirs(&includeDir, &srcDir)) {
        return;
    }

    SourceBundle bundle;
    collectSources(includeDir, srcDir, &bundle);

    outputView_->clear();
    const QString stamp = QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));
    appendLine(QStringLiteral("[%1] 运行 — 工作区：%2").arg(stamp, workspaceRoot_));

    if (bundle.cppSources.isEmpty() && bundle.cSources.isEmpty()) {
        appendLine(QStringLiteral("没有 .c/.cpp 源文件可链接，无法运行。"));
        return;
    }

    const QString gxxPath = resolveCompilerTool(QStringLiteral("g++"));
    const QString gccPath = resolveCompilerTool(QStringLiteral("gcc"));
    if (!bundle.cppSources.isEmpty() && gxxPath.isEmpty()) {
        appendLine(QStringLiteral("未找到 g++，无法链接 C++ 源文件。"));
        return;
    }
    if (bundle.cppSources.isEmpty() && !bundle.cSources.isEmpty() && gccPath.isEmpty()) {
        appendLine(QStringLiteral("纯 C 工程需要 gcc，但未找到。"));
        return;
    }

    QStringList includeArgs;
    if (QDir(includeDir).exists()) {
        includeArgs.append(QStringLiteral("-I"));
        includeArgs.append(includeDir);
    }
    if (QDir(srcDir).exists()) {
        includeArgs.append(QStringLiteral("-I"));
        includeArgs.append(srcDir);
    }

    const QString buildDir = QDir(workspaceRoot_).filePath(QStringLiteral(".toide-build"));
    QDir().mkpath(buildDir);
    const QString outExe = QDir(buildDir).filePath(runExecutableOutputName());

    if (QFileInfo::exists(outExe)) {
        QFile::remove(outExe);
    }

    compileButton_->setEnabled(false);
    runButton_->setEnabled(false);

    QString linkOut;
    bool linked = false;
    if (!bundle.cppSources.isEmpty()) {
        QStringList args = QStringList() << QStringLiteral("-std=c++20") << QStringLiteral("-O0") << QStringLiteral("-g")
                                         << QStringLiteral("-fno-diagnostics-color") << includeArgs;
        args.append(bundle.cppSources);
        args.append(bundle.cSources);
        args << QStringLiteral("-o") << outExe;
        linked = runCaptured(gxxPath, args, &linkOut);
    } else {
        QStringList args = QStringList() << QStringLiteral("-std=c17") << QStringLiteral("-O0") << QStringLiteral("-g")
                                         << QStringLiteral("-fno-diagnostics-color") << includeArgs;
        args.append(bundle.cSources);
        args << QStringLiteral("-o") << outExe;
        linked = runCaptured(gccPath, args, &linkOut);
    }

    if (!linkOut.trimmed().isEmpty()) {
        appendLine(linkOut.trimmed());
    }
    if (!linked) {
        appendLine(QStringLiteral("链接失败，未运行。"));
        compileButton_->setEnabled(true);
        runButton_->setEnabled(true);
        return;
    }

#if defined(Q_OS_WIN)
    {
        const QString compilerBin = QDir::toNativeSeparators(QFileInfo(!bundle.cppSources.isEmpty() ? gxxPath : gccPath).absolutePath());
        const QString appBin = QCoreApplication::instance() != nullptr
            ? QDir::toNativeSeparators(QCoreApplication::applicationDirPath())
            : QString{};
        const QStringList runtimeDlls{QStringLiteral("libgcc_s_seh-1.dll"), QStringLiteral("libstdc++-6.dll"),
                                      QStringLiteral("libwinpthread-1.dll")};
        for (const QString &dll : runtimeDlls) {
            const QString inBuild = QDir(buildDir).filePath(dll);
            if (QFileInfo::exists(inBuild)) {
                continue;
            }
            QString from;
            if (!appBin.isEmpty()) {
                const QString candidate = QDir(appBin).filePath(dll);
                if (QFileInfo::exists(candidate)) {
                    from = candidate;
                }
            }
            if (from.isEmpty() && !compilerBin.isEmpty()) {
                const QString candidate = QDir(compilerBin).filePath(dll);
                if (QFileInfo::exists(candidate)) {
                    from = candidate;
                }
            }
            if (!from.isEmpty()) {
                QFile::copy(from, inBuild);
            }
        }
    }
#endif

    appendLine(QStringLiteral(">>> 运行：%1").arg(outExe));

    QProcess runProc;
    runProc.setProcessChannelMode(QProcess::MergedChannels);
    runProc.setWorkingDirectory(workspaceRoot_);
    const QString compilerForPath = !bundle.cppSources.isEmpty() ? gxxPath : gccPath;
    QProcessEnvironment runEnv = QProcessEnvironment::systemEnvironment();
    prependPathForMinGwRuntime(&runEnv, compilerForPath);
    runProc.setProcessEnvironment(runEnv);
    runProc.start(outExe, QStringList());
    if (!runProc.waitForStarted(8000)) {
        appendLine(QStringLiteral("无法启动程序（是否缺少 MinGW 运行库？可先把 MinGW bin 加入 PATH 再试）。"));
        compileButton_->setEnabled(true);
        runButton_->setEnabled(true);
        return;
    }
    runProc.waitForFinished(60000);
    const QString runOut = QString::fromLocal8Bit(runProc.readAllStandardOutput());
    if (!runOut.isEmpty()) {
        appendLine(runOut.trimmed());
    }
    appendLine(QStringLiteral(">>> 进程退出码：%1").arg(runProc.exitCode()));

    compileButton_->setEnabled(true);
    runButton_->setEnabled(true);
}
