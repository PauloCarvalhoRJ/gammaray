#ifndef FACIESRELATIONSHIPDIAGRAMDIALOG_H
#define FACIESRELATIONSHIPDIAGRAMDIALOG_H

#include <QDialog>

class FaciesTransitionMatrix;

namespace Ui {
class FaciesRelationShipDiagramDialog;
}

class FaciesRelationShipDiagramDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FaciesRelationShipDiagramDialog(FaciesTransitionMatrix *ftm, QWidget *parent = nullptr);
    ~FaciesRelationShipDiagramDialog();

private:
    Ui::FaciesRelationShipDiagramDialog *ui;

    FaciesTransitionMatrix* m_faciesTransitionMatrix;

private Q_SLOTS:
    void performCalculation();
};

#endif // FACIESRELATIONSHIPDIAGRAMDIALOG_H
