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
}

class VariographicDecompositionDialog : public QDialog
{
    Q_OBJECT

public:
	explicit VariographicDecompositionDialog(const std::vector<IJAbstractCartesianGrid *> && grids, QWidget *parent = 0);
    ~VariographicDecompositionDialog();

Q_SIGNALS:
    void saveArray( spectral::array *gridData );

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
    void displayGrids( const std::vector< spectral::array >& grids,
                       const std::vector< std::string >& titles,
                       bool doShiftByHalf = true );
    void onSumOfFactorsWasComputed(spectral::array *gridData); //called to save grid data as a Cartesian grid
};

#endif // VARIOGRAPHICDECOMPOSITIONDIALOG_H
