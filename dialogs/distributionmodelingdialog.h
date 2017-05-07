#ifndef DISTRIBUTIONMODELINGDIALOG_H
#define DISTRIBUTIONMODELINGDIALOG_H

#include <QDialog>

class Attribute;
class GSLibParameterFile;

namespace Ui {
class DistributionModelingDialog;
}

class DistributionModelingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DistributionModelingDialog( Attribute* at, QWidget *parent = 0);
    ~DistributionModelingDialog();

private:
    Ui::DistributionModelingDialog *ui;
    GSLibParameterFile* m_gpf_histsmth;
    Attribute * m_attribute;

private slots:
    void onParameters();
    void onHistsmthCompletion();
    void onPlot();
    void onSave();
};

#endif // DISTRIBUTIONMODELINGDIALOG_H
