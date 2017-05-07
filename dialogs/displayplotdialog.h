#ifndef DISPLAYPLOTDIALOG_H
#define DISPLAYPLOTDIALOG_H

#include <QDialog>
#include <QString>
#include "gslib/gslibparameterfiles/gslibparameterfile.h"

namespace Ui {
class DisplayPlotDialog;
}

class PSWidget;

class DisplayPlotDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DisplayPlotDialog(const QString path_to_postscript, const QString diag_title, GSLibParameterFile gpf, QWidget *parent = 0);
    ~DisplayPlotDialog();

private slots:
    void onChangeParameters();
    void onSavePlot();

private:
    Ui::DisplayPlotDialog *ui;
    GSLibParameterFile _gpf;
    QString _ps_file_path;
    PSWidget *_psw;
};

#endif // DISPLAYPLOTDIALOG_H
