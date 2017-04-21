#ifndef SOFTINDICATORCALIBRATIONDIALOG_H
#define SOFTINDICATORCALIBRATIONDIALOG_H

#include <QDialog>

namespace Ui {
class SoftIndicatorCalibrationDialog;
}

class FileSelectorWidget;
class Attribute;
class SoftIndicatorCalibPlot;

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
    SoftIndicatorCalibPlot *m_softIndCalibPlot;

private slots:
    void onUpdateNumberOfCalibrationCurves();
    void onSave();

private:
    void saveTmpFileWithSoftIndicators();
};

#endif // SOFTINDICATORCALIBRATIONDIALOG_H
