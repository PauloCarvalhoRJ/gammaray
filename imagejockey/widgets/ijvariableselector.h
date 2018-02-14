#ifndef IJVARIABLESELECTOR_H
#define IJVARIABLESELECTOR_H

#include <QWidget>

namespace Ui {
class IJVariableSelector;
}

class IJAbstractCartesianGrid;
class IJAbstractVariable;

class IJVariableSelector : public QWidget
{
    Q_OBJECT

public:
    /** @param show_not_set If true, an item "NOT SET" is added as the first item in the list.*/
    explicit IJVariableSelector( bool show_not_set = false, QWidget *parent = 0);
    ~IJVariableSelector();

    /** Returns the index of the selected variable.
      * @return Negative number if no variable is selected or m_grid is null.
      */
    uint getSelectedVariableIndex();

    /** Returns the name of the selected variable. */
    QString getSelectedVariableName();

    /** Manually add a variable to the list. */
    void addVariable(IJAbstractVariable* var );

    /** Clears the list of variables. */
    void clear();

    /** Returns the selected variable. */
    IJAbstractVariable* getSelectedVariable();

    /** Returns the index of the currently selected item in the combobox. */
    int getCurrentComboIndex();

    /** Sets an option caption text. */
    void setCaption( QString caption );

signals:
    void variableSelected( IJAbstractVariable* var );
    void currentIndexChanged( int index );
    void errorOccurred( QString message );
    void warningOccurred( QString message );

public slots:
    /** Updates the list of variables from the passed grid.*/
    void onListVariables(IJAbstractCartesianGrid* grid );

private:
	Ui::IJVariableSelector *ui;
    bool m_hasNotSetItem;
    IJAbstractCartesianGrid *m_grid;

public slots:
    void onSelection( int index );
};

#endif // IJVARIABLESELECTOR_H
