#ifndef NDVESTIMATIONDIALOG_H
#define NDVESTIMATIONDIALOG_H

#include <QDialog>

class Attribute;

namespace Ui {
class NDVEstimationDialog;
}

class NDVEstimationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NDVEstimationDialog(Attribute *at, QWidget *parent = 0);
    ~NDVEstimationDialog();

private:
    Ui::NDVEstimationDialog *ui;
    Attribute *_at;

private slots:
    void updateMetricSizeLabels();
    void run();
};

#endif // NDVESTIMATIONDIALOG_H
