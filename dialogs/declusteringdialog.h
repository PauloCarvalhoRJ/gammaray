#ifndef DECLUSTERINGDIALOG_H
#define DECLUSTERINGDIALOG_H

#include <QDialog>

namespace Ui {
class DeclusteringDialog;
}

class Attribute;
class GSLibParameterFile;

class DeclusteringDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeclusteringDialog(Attribute *attribute, QWidget *parent = 0);
    ~DeclusteringDialog();

private:
    Ui::DeclusteringDialog *ui;
    Attribute* m_attribute;
    GSLibParameterFile* m_gpf_declus;

private slots:
    void onDeclus();
    void onViewSummary();
    void onHistogram();
    void onSave();
    void onLocmap();
};

#endif // DECLUSTERINGDIALOG_H
