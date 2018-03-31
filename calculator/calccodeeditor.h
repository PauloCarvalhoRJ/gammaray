#ifndef CALCCODEEDITOR_H
#define CALCCODEEDITOR_H

#include <QPlainTextEdit>

/** This class extends QPlainTextEdit to include script line numbers. */
class CalcCodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CalcCodeEditor(QWidget *parent = 0);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    QWidget *lineNumberArea;
};

#endif // CALCCODEEDITOR_H
