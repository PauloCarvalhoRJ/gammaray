#ifndef AUTOMATICVARFITDIALOG_H
#define AUTOMATICVARFITDIALOG_H

#include <QDialog>
#include "geostats/nestedvariogramstructuresparameters.h"
#include "geostats/automaticvariogramfitting.h"

class Attribute;
class CartesianGrid;
class IJGridViewerWidget;
class AutomaticVarFitExperimentsDialog;

namespace Ui {
class AutomaticVarFitDialog;
}

class AutomaticVarFitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AutomaticVarFitDialog( Attribute* at, QWidget *parent = nullptr);
    ~AutomaticVarFitDialog();

private:
    Ui::AutomaticVarFitDialog *ui;
    Attribute* m_at;
    CartesianGrid* m_cg;

    IJGridViewerWidget* m_gridViewerInput;
    IJGridViewerWidget* m_gridViewerVarmap;

    NestedVariogramStructuresParametersPtr m_nestedVariogramStructuresParametersForManual;

    AutomaticVariogramFitting m_autoVarFit;

private Q_SLOTS:

    void onDoWithSAandGD();
    void onDoWithLSRS();
    void onDoWithPSO();
    void onDoWithGenetic();
    void onDoWithManual();

    void onVarmapMethodChanged();
    void onNumberOfStructuresChanged(int number);

    void onRunExperiments();

    void onObjectiveFunctionChanged();

    void onMethodTabChanged( int tabIndex );

private:
    void runExperimentsWithSAandGD(const AutomaticVarFitExperimentsDialog& expParDiag);
    void runExperimentsWithSAandGD(
            int seedI,       int seedF,       int seedSteps,
            double iniTempI, double iniTempF, int iniTempSteps,
            double finTempI, double finTempF, int finTempSteps,
            double hopFactI, double hopFactF, int hopFactSteps
            );
    void runExperimentsWithLSRS();
    void runExperimentsWithPSO();
    void runExperimentsWithGenetic();

    void showConvergenceCurves(
            const std::vector< std::vector< double > >& curves ) const;
};

#endif // AUTOMATICVARFITDIALOG_H
