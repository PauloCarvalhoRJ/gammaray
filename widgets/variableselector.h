#ifndef VARIABLESELECTOR_H
#define VARIABLESELECTOR_H

#include <QWidget>

namespace Ui {
class VariableSelector;
}

class DataFile;
class Attribute;

class VariableSelector : public QWidget
{
    Q_OBJECT

public:
    /** @param show_not_set If true, an item "NOT SET" is added as the first item in the list.*/
    explicit VariableSelector( bool show_not_set = false, QWidget *parent = 0);
    ~VariableSelector();

    /** Returns the GEO-EAS column index of the selected variable.
      * @return Zero if no variable is selected or m_dataFile is null.
      */
    uint getSelectedVariableGEOEASIndex();

    /** Returns the name of the selected variable. */
    QString getSelectedVariableName();

    /** Manually add a variable to the list. */
    void addVariable( Attribute* at );

    /** Clears the list of variables. */
    void clear();

    /** Returns the selected variable. */
    Attribute* getSelectedVariable();

    /** Returns the index of the currently selected item in the combobox. */
    int getCurrentComboIndex();

    /** Sets an option caption text. */
    void setCaption( QString caption );

signals:
    void variableSelected( Attribute* at );
    void currentIndexChanged( int index );

public slots:
    /** Updates the list of variables from the passed data file.*/
    void onListVariables( DataFile* file );

private:
    Ui::VariableSelector *ui;
    bool m_hasNotSetItem;
    DataFile *m_dataFile;

public slots:
    void onSelection( int index );
};

#endif // VARIABLESELECTOR_H
