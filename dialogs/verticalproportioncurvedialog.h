#ifndef VERTICALPROPORTIONCURVEDIALOG_H
#define VERTICALPROPORTIONCURVEDIALOG_H

#include <QDialog>

namespace Ui {
class VerticalProportionCurveDialog;
}

class VerticalProportionCurveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VerticalProportionCurveDialog(QWidget *parent = nullptr);
    ~VerticalProportionCurveDialog();

    void dragEnterEvent(QDragEnterEvent *e);
    void dragMoveEvent(QDragMoveEvent *e);
    void dropEvent(QDropEvent *e);

private:
    Ui::VerticalProportionCurveDialog *ui;
};

#endif // VERTICALPROPORTIONCURVEDIALOG_H
