#ifndef EMDANALYSISDIALOG_H
#define EMDANALYSISDIALOG_H

#include <QDialog>
#include "imagejockey/ijabstractcartesiangrid.h"

namespace Ui {
class EMDAnalysisDialog;
}

/** The dialog used to perform Empirical Mode Decomposition on an image (grid). */
class EMDAnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EMDAnalysisDialog( IJAbstractCartesianGridPtr inputGrid,
                                uint inputVariableIndex,
                                QWidget *parent = nullptr);
    ~EMDAnalysisDialog();


private:
    Ui::EMDAnalysisDialog *ui;
    IJAbstractCartesianGridPtr m_inputGrid;
    uint m_inputVariableIndex;

private Q_SLOTS:
    void onPerformEMD();
};

#endif // EMDANALYSISDIALOG_H
