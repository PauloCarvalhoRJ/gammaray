#ifndef LVADATASETDIALOG_H
#define LVADATASETDIALOG_H

#include <QDialog>

namespace Ui {
class LVADataSetDialog;
}

class Attribute;

/** This dialog is used to compute data sets that capture Locally Varying Anosotropy from an imput data set. */
class LVADataSetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LVADataSetDialog( Attribute* at, QWidget *parent = nullptr );
    ~LVADataSetDialog();

private:
    Ui::LVADataSetDialog *ui;
    Attribute* m_at;

    void updateSummary();

private Q_SLOTS:
    void onComputeLVA();
};

#endif // LVADATASETDIALOG_H
