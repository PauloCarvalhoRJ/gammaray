#ifndef CARTESIANGRIDSELECTOR_H
#define CARTESIANGRIDSELECTOR_H

#include <QWidget>

namespace Ui {
class CartesianGridSelector;
}

class DataFile;

class CartesianGridSelector : public QWidget
{
    Q_OBJECT

public:
    /** @param If true, adds a "NOT SET" item as the first item. */
    explicit CartesianGridSelector(bool show_not_set = false, QWidget *parent = 0);
    ~CartesianGridSelector();

    /** Returns null pointer if no file is selected. */
    DataFile* getSelectedDataFile(){ return m_dataFile; }

signals:
    void cartesianGridSelected( DataFile* ps );

private:
    Ui::CartesianGridSelector *ui;
    bool m_HasNotSetItem;
    DataFile* m_dataFile;

public slots:
    void onSelection( int index );
};

#endif // CARTESIANGRIDSELECTOR_H
