#include "editor/editor_tab.h"

#include <QAbstractSlider>
#include <QFile>
#include <QHBoxLayout>
#include <QPainter>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPaintEvent>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextFormat>
#include <QWheelEvent>

namespace {

struct CollaboratorAccent {
    QColor editorHighlight;
    QColor gutterFill;
    QColor gutterText;
};

static CollaboratorAccent collaboratorAccentForUsername(const QString &user)
{
    static const QColor bases[] = {
        QColor(64, 156, 255),
        QColor(255, 128, 64),
        QColor(160, 96, 255),
        QColor(64, 200, 130),
        QColor(255, 96, 180),
        QColor(230, 190, 60),
        QColor(72, 200, 230),
        QColor(200, 100, 200),
        QColor(120, 200, 80),
        QColor(255, 160, 96),
    };
    constexpr int n = static_cast<int>(sizeof(bases) / sizeof(bases[0]));
    const int idx = user.isEmpty() ? 0 : (qAbs(static_cast<int>(qHash(user))) % n);
    CollaboratorAccent a;
    a.editorHighlight = bases[idx];
    a.editorHighlight.setAlpha(78);
    a.gutterFill = bases[idx];
    a.gutterFill.setAlpha(118);
    a.gutterText = bases[idx].darker(160);
    return a;
}

class EditorPlainTextEdit final : public QPlainTextEdit {
public:
    explicit EditorPlainTextEdit(QWidget *parent = nullptr)
        : QPlainTextEdit(parent)
    {
    }

    QTextBlock firstVisibleBlockPublic() const { return firstVisibleBlock(); }

    QRectF blockBoundingGeometryPublic(const QTextBlock &block) const
    {
        return blockBoundingGeometry(block);
    }

    QRectF blockBoundingRectPublic(const QTextBlock &block) const { return blockBoundingRect(block); }

    QPointF contentOffsetPublic() const { return contentOffset(); }

protected:
    void wheelEvent(QWheelEvent *e) override
    {
        QPlainTextEdit::wheelEvent(e);
        if (parentWidget() != nullptr) {
            parentWidget()->update();
        }
    }
};

} // namespace

class EditorTab::LineNumberArea final : public QWidget {
public:
    explicit LineNumberArea(EditorTab *editorTab)
        : QWidget(editorTab)
        , editorTab_(editorTab)
    {
        setObjectName(QStringLiteral("editorLineNumberArea"));
    }

    QSize sizeHint() const override { return {editorTab_->lineNumberAreaWidth(), 0}; }

protected:
    void paintEvent(QPaintEvent *event) override { editorTab_->lineNumberAreaPaintEvent(event); }

private:
    EditorTab *editorTab_;
};

EditorTab::EditorTab(QWidget *parent)
    : QWidget(parent)
    , editor_(new EditorPlainTextEdit(this))
    , lineNumberArea_(new LineNumberArea(this))
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(lineNumberArea_);
    layout->addWidget(editor_, 1);

    connect(editor_, &QPlainTextEdit::textChanged, this, [this]() {
        setDirty(true);
        emit textEdited(filePath_, text());
        updateLineNumberAreaWidth();
        lineNumberArea_->update();
    });
    connect(editor_, &QPlainTextEdit::cursorPositionChanged, this, [this]() {
        emitCursorPosition();
        lineNumberArea_->update();
    });
    connect(editor_, &QPlainTextEdit::updateRequest, this, &EditorTab::updateLineNumberArea);
    connect(editor_->verticalScrollBar(), &QAbstractSlider::valueChanged, lineNumberArea_,
            qOverload<>(&QWidget::update));
    connect(editor_->document(), &QTextDocument::blockCountChanged, this, [this]() {
        updateLineNumberAreaWidth();
        lineNumberArea_->update();
    });

    updateLineNumberAreaWidth();
}

QString EditorTab::filePath() const
{
    return filePath_;
}

QString EditorTab::text() const
{
    return editor_->toPlainText();
}

bool EditorTab::isDirty() const
{
    return dirty_;
}

bool EditorTab::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    filePath_ = filePath;
    const auto content = QString::fromUtf8(file.readAll());

    const QSignalBlocker blocker(editor_);
    editor_->setPlainText(content);
    setDirty(false);
    serverDocVersion_ = -1;
    updateLineNumberAreaWidth();
    lineNumberArea_->update();
    return true;
}

bool EditorTab::save()
{
    if (filePath_.isEmpty()) {
        return false;
    }

    QFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    const QByteArray body = text().toUtf8();
    if (file.write(body) != body.size()) {
        return false;
    }
    if (!file.flush()) {
        return false;
    }
    setDirty(false);
    return true;
}

void EditorTab::setText(const QString &text)
{
    editor_->setPlainText(text);
    updateLineNumberAreaWidth();
    lineNumberArea_->update();
}

void EditorTab::applyRemoteText(const QString &text)
{
    const QSignalBlocker blocker(editor_);
    editor_->setPlainText(text);
    setDirty(true);
    updateLineNumberAreaWidth();
    lineNumberArea_->update();
}

void EditorTab::showRemoteActivity(const QString &username, int line, int column)
{
    const auto targetLine = qMax(1, line);
    const auto targetColumn = qMax(1, column);
    remoteActivityLine_ = targetLine;
    const auto block = editor_->document()->findBlockByNumber(targetLine - 1);
    if (!block.isValid()) {
        editor_->setExtraSelections({});
        remoteActivityLine_ = 0;
        remoteActivityUser_.clear();
        lineNumberArea_->update();
        return;
    }

    remoteActivityUser_ = username;
    const CollaboratorAccent accent = collaboratorAccentForUsername(username);

    QTextEdit::ExtraSelection selection;
    selection.cursor = QTextCursor(block);
    selection.cursor.clearSelection();
    selection.format.setBackground(accent.editorHighlight);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);

    editor_->setExtraSelections({selection});
    editor_->setToolTip(QStringLiteral("%1 正在编辑：第 %2 行，第 %3 列").arg(username).arg(targetLine).arg(targetColumn));
    lineNumberArea_->update();
}

void EditorTab::moveCursorTo(int line, int column)
{
    const auto targetLine = qMax(1, line);
    const auto targetColumn = qMax(1, column);

    const auto block = editor_->document()->findBlockByNumber(targetLine - 1);
    QTextCursor cursor;
    if (!block.isValid()) {
        cursor = editor_->textCursor();
        cursor.movePosition(QTextCursor::End);
    } else {
        cursor = QTextCursor(block);
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, targetColumn - 1);
    }

    editor_->setTextCursor(cursor);
    editor_->setFocus();
}

void EditorTab::setServerDocVersion(qint64 version)
{
    serverDocVersion_ = version;
}

qint64 EditorTab::serverDocVersion() const
{
    return serverDocVersion_;
}

void EditorTab::emitCursorPosition()
{
    const QTextCursor cursor = editor_->textCursor();
    emit cursorMoved(filePath_, cursor.blockNumber() + 1, cursor.positionInBlock() + 1);
}

void EditorTab::setDirty(bool dirty)
{
    if (dirty_ == dirty) {
        return;
    }

    dirty_ = dirty;
    emit dirtyChanged(dirty_);
}

int EditorTab::lineNumberAreaWidth() const
{
    int digits = 1;
    int maxV = qMax(1, editor_->document()->blockCount());
    while (maxV >= 10) {
        maxV /= 10;
        ++digits;
    }
    const QFontMetrics fm(editor_->font());
    return 4 + fm.horizontalAdvance(QLatin1Char('9')) * digits;
}

void EditorTab::updateLineNumberAreaWidth()
{
    lineNumberArea_->setFixedWidth(lineNumberAreaWidth());
}

void EditorTab::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy != 0) {
        lineNumberArea_->scroll(0, dy);
    } else {
        lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());
    }
}

void EditorTab::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    auto *ed = static_cast<EditorPlainTextEdit *>(editor_);

    QPainter painter(lineNumberArea_);
    painter.fillRect(event->rect(), palette().color(QPalette::AlternateBase));

    QTextBlock block = ed->firstVisibleBlockPublic();
    int blockNumber = block.blockNumber();
    qreal top = ed->blockBoundingGeometryPublic(block).translated(ed->contentOffsetPublic()).top();
    qreal bottom = top + ed->blockBoundingRectPublic(block).height();
    const int currentLine = editor_->textCursor().blockNumber() + 1;
    const int w = lineNumberArea_->width();

    CollaboratorAccent remoteRowAccent{};
    if (remoteActivityLine_ > 0) {
        remoteRowAccent = collaboratorAccentForUsername(remoteActivityUser_);
    }

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const int line = blockNumber + 1;
            if (remoteActivityLine_ == line) {
                painter.fillRect(0, static_cast<int>(top), w, static_cast<int>(bottom - top), remoteRowAccent.gutterFill);
            } else if (line == currentLine) {
                painter.fillRect(0, static_cast<int>(top), w, static_cast<int>(bottom - top),
                                 palette().color(QPalette::Midlight));
            }
            painter.setPen(remoteActivityLine_ == line ? remoteRowAccent.gutterText
                                                       : palette().color(QPalette::ButtonText));
            painter.drawText(0, static_cast<int>(top), w - 4, static_cast<int>(bottom - top),
                             Qt::AlignRight | Qt::AlignVCenter, QString::number(line));
        }

        block = block.next();
        top = bottom;
        bottom = top + ed->blockBoundingRectPublic(block).height();
        ++blockNumber;
    }
}
