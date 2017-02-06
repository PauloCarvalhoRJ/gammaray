#ifndef NSCOREDIALOG_H
#define NSCOREDIALOG_H

#include <QDialog>

namespace Ui {
class NScoreDialog;
}

class Attribute;
class GSLibParameterFile;
class DataFile;

class NScoreDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NScoreDialog(Attribute* attribute, QWidget *parent = 0);
    ~NScoreDialog();

private:
    Ui::NScoreDialog *ui;
    Attribute* m_attribute;
    GSLibParameterFile* m_gpf_nscore;

private slots:
    void onParams();
    void onHistogram();
    void onSave();

private:
    void doHistogramPointSet();
    void doHistogramGrid();
    void doHistogramCommon(DataFile* input_data_file);
};

#endif // NSCOREDIALOG_H
