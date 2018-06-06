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

	/** Displays a single volume grid for debugging purposes. */
	static void displayGrid(const spectral::array& grid,
							const std::string& title,
							bool shiftByHalf );

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

private Q_SLOTS:
	void doVariographicDecomposition();
	//you can use this function to see the contents of large matrices and grids during debug.
	//TIP C++11: use displayGrids({A}, {"A matrix"}, {false}); to display a single object.
	void displayGrids(const std::vector< spectral::array >& grids,
					  const std::vector< std::string >& titles,
					  const std::vector< bool >& shiftByHalves );
    void onSumOfFactorsWasComputed(spectral::array *gridData); //called to save grid data as a Cartesian grid
    void doVariographicDecompositionSVDonData();
};

#endif // VARIOGRAPHICDECOMPOSITIONDIALOG_H
