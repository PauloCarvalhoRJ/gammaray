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

signals:
    void variableSelected( Attribute* at );

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
