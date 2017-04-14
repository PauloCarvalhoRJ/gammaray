#ifndef SOFTINDICATORCALIBRATIONDIALOG_H
#define SOFTINDICATORCALIBRATIONDIALOG_H

#include <QDialog>

namespace Ui {
class SoftIndicatorCalibrationDialog;
}

class FileSelectorWidget;
class Attribute;

class SoftIndicatorCalibrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SoftIndicatorCalibrationDialog(Attribute *at, QWidget *parent = 0);
    ~SoftIndicatorCalibrationDialog();

private:
    Ui::SoftIndicatorCalibrationDialog *ui;
    FileSelectorWidget *m_fsw;
    Attribute *m_at;
};

#endif // SOFTINDICATORCALIBRATIONDIALOG_H
