#ifndef IJCARTESIANGRIDSELECTOR_H
#define IJCARTESIANGRIDSELECTOR_H

#include <QWidget>

namespace Ui {
class IJCartesianGridSelector;
}

class IJAbstractCartesianGrid;

class IJCartesianGridSelector : public QWidget
{
    Q_OBJECT

public:
    /** @param show_not_set If true, adds a "NOT SET" item as the first item. */
    explicit IJCartesianGridSelector( const std::vector<IJAbstractCartesianGrid*>& grids,
                                      bool show_not_set = false,
                                      QWidget *parent = 0);
    ~IJCartesianGridSelector();

    /** Returns null pointer if no grid is selected. */
    IJAbstractCartesianGrid* getSelectedGrid(){ return m_cg; }

signals:
    void cartesianGridSelected( IJAbstractCartesianGrid* cg );

private:
    Ui::IJCartesianGridSelector *ui;
    bool m_HasNotSetItem;
    IJAbstractCartesianGrid* m_cg;
    const std::vector<IJAbstractCartesianGrid*>& m_grids;

public slots:
    void onSelection( int index );
};

#endif // IJCARTESIANGRIDSELECTOR_H
