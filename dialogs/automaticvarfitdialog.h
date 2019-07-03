#ifndef AUTOMATICVARFITDIALOG_H
#define AUTOMATICVARFITDIALOG_H

#include <QDialog>
#include "spectral/spectral.h"
#include "imagejockey/ijvariographicmodel2d.h"

class Attribute;
class CartesianGrid;
class IJGridViewerWidget;
class IJAbstractCartesianGrid;

namespace Ui {
class AutomaticVarFitDialog;
}

/** The parameters domain for the optimization methods (bound conditions). */
struct VariogramParametersDomain {
    IJVariographicStructure2D min;
    IJVariographicStructure2D max;
};

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
                               const int m ) const;

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
    spectral::array getInputPhaseMap() const;

    /**
     * Displays a series of grids in a dialog.
     * TIP C++11: use displayGrids({A}, {"A matrix"}, {false}); to display a single grid.
     * @param modal If true, the dialog blocks execution until it is closed.
     */
    void displayGrids( const std::vector< spectral::array >& grids,
                       const std::vector< std::string >& titles,
                       const std::vector< bool >& shiftByHalves,
                       bool modal ) const;

    /**
     * Applies the principle of the Fourier Integral Method to obtain the map in spatial domain
     * from a variographic map (theoretical or experimental) and a map of FFT phases.
     * REF: The Fourier Integral Method: An Efficient Spectral Method For Simulation of Random Fields. (Pardo-Iguzquiza & Chica-Olmo, 1993)
     * @param gridWithCovariance The grid data with the variographic surface.  It must be actually a correlographic surface,
     *                           that is, with max value at the center and decrasing with distance from the center.
     *                           h=0 must be at the center of the grid.
     * @param gridWithFFTphases The grid data with the FFT phases in radians.  Must vary between -PI and +PI.
     */
    spectral::array computeFIM( const spectral::array& gridWithCovariance,
                                const spectral::array& gridWithFFTphases ) const;

    /** Method called to display a dialog with relevant results from the variogram structures passed.
     * This method is not particularly useful to any client code.  It is used internally to reuse code.
     * @param variogramStructures The nested variogram structures as parameters (azimuth, axis, etc.).
     * @param fftPhaseMapOfInput The FFT phase map of the input grid data.
     * @param varmapOfInput Varmap fo the input grid data.
     * @param modal If true, the dialog blocks execution until it is closed.
     */
    void displayResults(const std::vector< IJVariographicStructure2D >& variogramStructures ,
                        const spectral::array &fftPhaseMapOfInput,
                        const spectral::array &varmapOfInput,
                        bool modal ) const;

    /** Method called by the several methods of optimization to initialize the
     *  parameter domain (boundary conditions) and the set of parameters.
     * All non-const method parameters are output parameters.
     * @param inputVarmap The varmap of the input grid data.
     * @param m The number of wanted variogram nested structures.
     * @param domain The domain boundaries as a VariogramParamateresDomainObject
     * @param vw The set of parameters as a linear array.
     * @param L_wMin The lower parameters boundaries as a linear array.
     * @param L_wMax The upper parameters boundaries as a linear array.
     * @param variogramStructures The set of parameters as a structured object.
     */
    void initDomainAndParameters( const spectral::array &inputVarmap,
                                  int m,
                                  VariogramParametersDomain& domain,
                                  spectral::array& vw,
                                  spectral::array& L_wMin,
                                  spectral::array& L_wMax,
                                  std::vector<IJVariographicStructure2D> &variogramStructures ) const;

private Q_SLOTS:
    void onDoWithSAandGD();
    void onDoWithLSRS();
    void onDoWithPSO();
    void onDoWithGenetic();
};

#endif // AUTOMATICVARFITDIALOG_H
