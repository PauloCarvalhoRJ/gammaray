#ifndef BIDISTRIBUTIONMODELINGDIALOG_H
#define BIDISTRIBUTIONMODELINGDIALOG_H

#include <QDialog>

class Attribute;
class UnivariateDistributionSelector;
class GSLibParameterFile;

namespace Ui {
class BidistributionModelingDialog;
}

class BidistributionModelingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BidistributionModelingDialog(Attribute* atX, Attribute* atY,  QWidget *parent = 0);
    ~BidistributionModelingDialog();

private:
    Ui::BidistributionModelingDialog *ui;
    Attribute* m_atX;
    Attribute* m_atY;
    UnivariateDistributionSelector* m_cmbXDist;
    UnivariateDistributionSelector* m_cmbYDist;
    GSLibParameterFile* m_gpf_scatsmth;
    GSLibParameterFile* m_gpf_bivplt;

private slots:
    void onParameters();
    void onScatsmthCompletion();
    void onPlot();
    void onSave();
    void onSaveXDistr();
    void onSaveYDistr();
};

#endif // BIDISTRIBUTIONMODELINGDIALOG_H
