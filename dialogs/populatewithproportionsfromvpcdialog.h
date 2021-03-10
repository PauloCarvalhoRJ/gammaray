#ifndef POPULATEWITHPROPORTIONSFROMVPCDIALOG_H
#define POPULATEWITHPROPORTIONSFROMVPCDIALOG_H

#include <QDialog>

class CartesianGrid;
class VerticalProportionCurve;

namespace Ui {
class PopulateWithProportionsFromVPCDialog;
}

/** This dialog is used to populate objects with proportions read from
 *  a Vertical Proportion Curve object.
 */
class PopulateWithProportionsFromVPCDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PopulateWithProportionsFromVPCDialog(QWidget *parent = nullptr);
    ~PopulateWithProportionsFromVPCDialog();

    void setCartesianGrid( CartesianGrid* cg );

    void setVPC( VerticalProportionCurve* vpc );

private:
    Ui::PopulateWithProportionsFromVPCDialog *ui;
    CartesianGrid* m_cg;
    VerticalProportionCurve* m_vpc;

    void updateInterface();

private Q_SLOTS:
    void onProcess();
};

#endif // POPULATEWITHPROPORTIONSFROMVPCDIALOG_H
