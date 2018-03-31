#include "calclinenumberarea.h"
#include "calccodeeditor.h"

CalcLineNumberArea::CalcLineNumberArea(CalcCodeEditor *editor) :
    QWidget(editor)
{
    codeEditor = editor;
}

QSize CalcLineNumberArea::sizeHint() const
{
    return QSize(codeEditor->lineNumberAreaWidth(), 0);
}

void CalcLineNumberArea::paintEvent(QPaintEvent *event)
{
    codeEditor->lineNumberAreaPaintEvent(event);
}
