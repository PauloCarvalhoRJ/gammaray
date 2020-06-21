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

    std::vector< IJVariographicStructure2D > m_accumulatedStructuresForClusterAnalysis;

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

    void onResetAccumulatedScatterDataOfExperiments();

    void onSaveScatterDataOfExperiments();

private:
    void runExperimentsWithSAandGD(const AutomaticVarFitExperimentsDialog& expParDiag);
    void runExperimentsWithSAandGD(
            int seedI,       int seedF,       int seedSteps,
            double iniTempI, double iniTempF, int iniTempSteps,
            double finTempI, double finTempF, int finTempSteps,
            double hopFactI, double hopFactF, int hopFactSteps
            );
    void runExperimentsWithLSRS( const AutomaticVarFitExperimentsDialog& expParDiag );
    void runExperimentsWithLSRS(
            int seedI,      int seedF,      int seedSteps,
            double nLinesI, double nLinesF, int nLinesSteps
            );
    void runExperimentsWithPSO(const AutomaticVarFitExperimentsDialog &expParDiag);
    void runExperimentsWithPSO(
            int seedI,             int seedF,             int seedSteps,
            double nParticlesI,    double nParticlesF,    int nParticlesSteps,
            double inertiaI,       double inertiaF,       int inertiaSteps,
            double acceleration1I, double acceleration1F, int acceleration1Steps,
            double acceleration2I, double acceleration2F, int acceleration2Steps
            );
    void runExperimentsWithGenetic(const AutomaticVarFitExperimentsDialog& expParDiag);
    void runExperimentsWithGenetic(
            int    seedI,         int seedF,            int seedSteps,
            double popSizeI,      double popSizeF,      int popSizeSteps,
            double selSizeI,      double selSizeF,      int selSizeSteps,
            double xOverProbI,    double xOverProbF,    int xOverProbSteps,
            double pointOfXOverI, double pointOfXOverF, int pointOfXOverSteps,
            double mutRateI,      double mutRateF,      int mutRateSteps
            );

    // First element in each pair: curve caption.
    // Second element in each pair: curve values.
    void showConvergenceCurves(
            QString chartTitle,
            const std::vector< std::pair< QString, std::vector< double > > >& curves ) const;
};

#endif // AUTOMATICVARFITDIALOG_H
