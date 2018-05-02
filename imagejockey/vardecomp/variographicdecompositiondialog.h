#ifndef VARIOGRAPHICDECOMPOSITIONDIALOG_H
#define VARIOGRAPHICDECOMPOSITIONDIALOG_H

#include <QDialog>

namespace Ui {
class VariographicDecompositionDialog;
}

class IJCartesianGridSelector;
class IJVariableSelector;
class IJAbstractVariable;
class IJAbstractCartesianGrid;
namespace spectral {
    class array;
    class complex_array;
}

class VariographicDecompositionDialog : public QDialog
{
    Q_OBJECT

public:
	explicit VariographicDecompositionDialog(const std::vector<IJAbstractCartesianGrid *> && grids, QWidget *parent = 0);
    ~VariographicDecompositionDialog();

Q_SIGNALS:
    void saveArray( spectral::array *gridData );
	void error( QString message );
	void warning( QString message );
	void info( QString message );

private:
    Ui::VariographicDecompositionDialog *ui;

	/** Selector of the grid with the data. */
	IJCartesianGridSelector* m_gridSelector;

	/** Target variable. */
	IJVariableSelector* m_variableSelector;

	/** The list of available Cartesian grids. */
	std::vector< IJAbstractCartesianGrid* > m_grids;

    /** The objective function for the optimization process.
     * See complete theory in the program manual for in-depth explanation of the method's parameters below.
     * @param originalGrid  The grid with original data for comparison.
     * @param vectorOfParameters The column-vector with the free paramateres.
     * @param A The LHS of the linear system originated from the information conservation constraints.
     * @param Adagger The pseudo-inverse of A.
     * @param B The RHS of the linear system originated from the information conservation constraints.
     * @param I The identity matrix compatible with the formula: [a] = Adagger.B + (I-Adagger.A)[w]
     * @param m The desired number of geological factor.
     * @param fundamentalFactors  The list with the original data's fundamental factors computed with SVD.
     * @param fftOriginalGridMagAndPhase The Fourier image of the original data in polar form.
     * @return
     */
    double F(const spectral::array &originalGrid,
             const spectral::array& vectorOfParameters,
             const spectral::array& A,
             const spectral::array& Adagger,
             const spectral::array& B,
             const spectral::array& I,
             int m,
             const std::vector< spectral::array >& fundamentalFactors,
             const spectral::complex_array& fftOriginalGridMagAndPhase );

private Q_SLOTS:
	void doVariographicDecomposition();
	void displayGrids(const std::vector< spectral::array >& grids,
					   const std::vector< std::string >& titles,
					   const std::vector< bool >& shiftByHalves );
    void onSumOfFactorsWasComputed(spectral::array *gridData); //called to save grid data as a Cartesian grid
};

#endif // VARIOGRAPHICDECOMPOSITIONDIALOG_H
