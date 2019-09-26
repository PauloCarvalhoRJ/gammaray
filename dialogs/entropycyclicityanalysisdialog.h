#ifndef ENTROPYCYCLICITYANALYSISDIALOG_H
#define ENTROPYCYCLICITYANALYSISDIALOG_H

#include <QDialog>

class FaciesTransitionMatrix;

namespace Ui {
class EntropyCyclicityAnalysisDialog;
}

class EntropyCyclicityAnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EntropyCyclicityAnalysisDialog( FaciesTransitionMatrix *ftm, QWidget *parent = nullptr );
    ~EntropyCyclicityAnalysisDialog();

private:
    Ui::EntropyCyclicityAnalysisDialog *ui;
    FaciesTransitionMatrix* m_faciesTransitionMatrix;

    void performCalculation();
};

#endif // ENTROPYCYCLICITYANALYSISDIALOG_H
