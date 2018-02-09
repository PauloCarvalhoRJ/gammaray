#ifndef IJCARTESIANGRIDSELECTOR_H
#define IJCARTESIANGRIDSELECTOR_H

#include <QWidget>

namespace Ui {
class IJCartesianGridSelector;
}

class DataFile;

class IJCartesianGridSelector : public QWidget
{
    Q_OBJECT

public:
    /** @param show_not_set If true, adds a "NOT SET" item as the first item. */
    explicit IJCartesianGridSelector(bool show_not_set = false, QWidget *parent = 0);
    ~IJCartesianGridSelector();

    /** Returns null pointer if no file is selected. */
    DataFile* getSelectedDataFile(){ return m_dataFile; }

signals:
    void cartesianGridSelected( DataFile* ps );

private:
    Ui::IJCartesianGridSelector *ui;
    bool m_HasNotSetItem;
    DataFile* m_dataFile;

public slots:
    void onSelection( int index );
};

#endif // IJCARTESIANGRIDSELECTOR_H
