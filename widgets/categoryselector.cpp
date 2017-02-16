#include "categoryselector.h"
#include "ui_categoryselector.h"
#include "domain/categorydefinition.h"
#include "util.h"
#include "domain/application.h"

CategorySelector::CategorySelector(CategoryDefinition *cd, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CategorySelector)
{
    ui->setupUi(this);

    if( cd ){
        cd->loadTriplets(); //makes sure the triplets are read from the file system.
        for( int i = 0; i < cd->getCategoryCount(); ++i){
            ui->cmbCategories->addItem( Util::makeGSLibColorIcon( cd->getColorCode( i ) ),
                                        cd->getCategoryName( i ) + " (code = " + QString::number(cd->getCategoryCode( i )) + ")" );
        }
    } else {
        ui->cmbCategories->addItem( "ERROR: NO CATEGORY DEFINITION" );
    }
}

CategorySelector::~CategorySelector()
{
    delete ui;
}
