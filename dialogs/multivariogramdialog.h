#ifndef MULTIVARIOGRAMDIALOG_H
#define MULTIVARIOGRAMDIALOG_H

#include <QDialog>

class Attribute;

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
    std::vector<Attribute *> _attributes;

private slots:
    void onGam();
};

#endif // MULTIVARIOGRAMDIALOG_H
