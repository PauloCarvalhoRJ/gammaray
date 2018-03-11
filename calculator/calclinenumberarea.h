#ifndef CALCLINENUMBERAREA_H
#define CALCLINENUMBERAREA_H

#include <QWidget>

class CalcCodeEditor;

/** This class is a widget that draws the script line numbers in the editor. */
class CalcLineNumberArea : public QWidget
{

    Q_OBJECT

public:
    explicit CalcLineNumberArea (CalcCodeEditor *editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    CalcCodeEditor *codeEditor;
};

#endif // CALCLINENUMBERAREA_H
