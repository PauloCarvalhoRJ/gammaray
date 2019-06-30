#ifndef AUTOMATICVARFITDIALOG_H
#define AUTOMATICVARFITDIALOG_H

#include <QDialog>
#include "spectral/spectral.h"

class Attribute;
class CartesianGrid;
class IJGridViewerWidget;
class IJAbstractCartesianGrid;

namespace Ui {
class AutomaticVarFitDialog;
}

/** TODO: Refactor: transfer varfit specific code to a domain class called AutomaticVarFit. */
class AutomaticVarFitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AutomaticVarFitDialog( Attribute* at, QWidget *parent = nullptr);
    ~AutomaticVarFitDialog();

    /** The objective function for the optimization processes (indirect variographic fitting).
     * See complete theory in the program manual for in-depth explanation of the method's parameters below.
     * @param gridWithGeometry A grid object whose geometry will be copied to the generated grid with the varmap for objective function evaluation.
     * @param inputData The grid data for comparison.
     * @param vectorOfParameters The column-vector with the free paramateres (variogram parameters).
     * @param m The desired number of variographic nested structures.
     * @return A distance/difference measure.
     */
    double objectiveFunction ( IJAbstractCartesianGrid& gridWithGeometry,
                               const spectral::array &inputGridData,
                               const spectral::array &vectorOfParameters,
                               const int m );

private:
    Ui::AutomaticVarFitDialog *ui;

    Attribute* m_at;
    CartesianGrid* m_cg;

    IJGridViewerWidget* m_gridViewerInput;
    IJGridViewerWidget* m_gridViewerVarmap;

    /** Computes the varmap of the input data.
     * TODO: consider moving this method to either CartesianGrid,
     * IJAbstractCartesianGrid, GridFile or Util. */
    spectral::array computeVarmap() const;

    /** Utilitary function that encapsulates variographic surface generation from
     * variogram model parameters.
     * @param gridWithGeometry A grid object whose geometry will be copied to the generated grid.
     * @param vectorOfParameters The vector with the variographic parameters follwoing this order:
     *                           [axis0,ratio0,az0,cc0,axis1,ratio1,...].
     * @param m The number of structures.
     * TODO: consider moving this method to GeostatsUtil.
     */
    spectral::array generateVariographicSurface( IJAbstractCartesianGrid& gridWithGeometry,
                                                 const spectral::array &vectorOfParameters,
                                                 const int m ) const;

    /**
     * Returns the FFT phase map of the input data.
     */
    spectral::array getInputPhaseMap();

    /**
     * Displays a series of grids in a dialog.
     * TIP C++11: use displayGrids({A}, {"A matrix"}, {false}); to display a single grid.
     */
    void displayGrids(const std::vector< spectral::array >& grids,
                      const std::vector< std::string >& titles,
                      const std::vector< bool >& shiftByHalves ) ;

private Q_SLOTS:
    void onDoWithSAandGD();
    void onDoWithLSRS();
    void onDoWithPSO();
    void onDoWithGenetic();
};

#endif // AUTOMATICVARFITDIALOG_H
