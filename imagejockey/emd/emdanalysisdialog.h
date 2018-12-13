#ifndef EMDANALYSISDIALOG_H
#define EMDANALYSISDIALOG_H

#include <QDialog>

namespace Ui {
class EMDAnalysisDialog;
}

class IJAbstractCartesianGrid;

/** The dialog used to perform Empirical Mode Decomposition on an image (grid). */
class EMDAnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    /**
    * The index of first variable is zero.
    */
    explicit EMDAnalysisDialog( IJAbstractCartesianGrid* inputGrid,
                                uint inputVariableIndex,
                                QWidget *parent = nullptr);
    ~EMDAnalysisDialog();


private:
    Ui::EMDAnalysisDialog *ui;
    IJAbstractCartesianGrid* m_inputGrid;
    uint m_inputVariableIndex;

private Q_SLOTS:
    void onPerformEMD();
};

#endif // EMDANALYSISDIALOG_H
