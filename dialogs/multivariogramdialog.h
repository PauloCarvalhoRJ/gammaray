#ifndef MULTIVARIOGRAMDIALOG_H
#define MULTIVARIOGRAMDIALOG_H

#include <QDialog>

class Attribute;
class GSLibParameterFile;

namespace Ui {
class MultiVariogramDialog;
}

class MultiVariogramDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MultiVariogramDialog(const std::vector<Attribute *> attributes,
                                  QWidget *parent = 0);
    ~MultiVariogramDialog();

private:
    Ui::MultiVariogramDialog *ui;
    std::vector<Attribute *> m_attributes;
    GSLibParameterFile* m_gpf_gam;
    GSLibParameterFile* m_gpf_vargplt;

private slots:
    void onGam();
    void onVargplt(std::vector<QString> &expVarFilePaths);
};

#endif // MULTIVARIOGRAMDIALOG_H
