#include <QtTest/QtTest>

#include "editor/editor_tab.h"

#include <QSignalSpy>
#include <QTemporaryDir>

class EditorTabTest final : public QObject {
    Q_OBJECT

private slots:
    void loadFileReadsContentAndStartsClean();
    void editingContentMarksTabDirtyAndSavePersists();
};

void EditorTabTest::loadFileReadsContentAndStartsClean()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const auto filePath = directory.filePath(QStringLiteral("main.cpp"));
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("int main() { return 0; }\n");
    file.close();

    EditorTab editor;

    QVERIFY(editor.loadFile(filePath));

    QCOMPARE(editor.filePath(), filePath);
    QCOMPARE(editor.text(), QStringLiteral("int main() { return 0; }\n"));
    QVERIFY(!editor.isDirty());
}

void EditorTabTest::editingContentMarksTabDirtyAndSavePersists()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const auto filePath = directory.filePath(QStringLiteral("notes.txt"));
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("before\n");
    file.close();

    EditorTab editor;
    QSignalSpy dirtySpy(&editor, &EditorTab::dirtyChanged);

    QVERIFY(editor.loadFile(filePath));
    editor.setText(QStringLiteral("after\n"));

    QVERIFY(editor.isDirty());
    QVERIFY(dirtySpy.count() > 0);
    QVERIFY(editor.save());
    QVERIFY(!editor.isDirty());

    QFile savedFile(filePath);
    QVERIFY(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(savedFile.readAll()), QStringLiteral("after\n"));
}

QTEST_MAIN(EditorTabTest)

#include "editor_tab_test.moc"
