#ifndef THINSECTIONANALYSISRESULTSDIALOG_H
#define THINSECTIONANALYSISRESULTSDIALOG_H

#include <QDialog>

namespace Ui {
class ThinSectionAnalysisResultsDialog;
}

class ThinSectionAnalysisResultsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ThinSectionAnalysisResultsDialog(QWidget *parent = nullptr);
    ~ThinSectionAnalysisResultsDialog();

private:
    Ui::ThinSectionAnalysisResultsDialog *ui;
};

#endif // THINSECTIONANALYSISRESULTSDIALOG_H
