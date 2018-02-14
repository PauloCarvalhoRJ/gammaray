#include "ijcartesiangridselector.h"
#include "ui_ijcartesiangridselector.h"
#include "../ijabstractcartesiangrid.h"

IJCartesianGridSelector::IJCartesianGridSelector(const std::vector<IJAbstractCartesianGrid *> &grids,
                                                 bool show_not_set,
                                                 QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IJCartesianGridSelector),
    m_HasNotSetItem( show_not_set ),
    m_cg( nullptr ),
    m_grids( grids )
{
    ui->setupUi(this);

    if( m_HasNotSetItem )
        ui->cmbGrids->addItem( "NOT SET" );

    for( uint i = 0; i < grids.size(); ++i){
        IJAbstractCartesianGrid* grid = grids[i];
        ui->cmbGrids->addItem( grid->getGridIcon(), grid->getGridName() );
    }
}

IJCartesianGridSelector::~IJCartesianGridSelector()
{
    delete ui;
}

void IJCartesianGridSelector::onSelection(int /*index*/)
{
    m_cg = nullptr;
    for( uint i = 0; i < m_grids.size(); ++i){
        IJAbstractCartesianGrid* grid = m_grids[i];
        if( grid->getGridName() == ui->cmbGrids->currentText() ){
            m_cg = grid;
            emit cartesianGridSelected( grid );
            return;
        }
    }
    //the user may select "NOT SET", so emit signal with null pointer.
    emit cartesianGridSelected( nullptr );
}
