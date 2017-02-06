#include "cartesiangridselector.h"
#include "ui_cartesiangridselector.h"
#include "../domain/project.h"
#include "../domain/application.h"
#include "../domain/objectgroup.h"
#include "../domain/file.h"

CartesianGridSelector::CartesianGridSelector(bool show_not_set, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CartesianGridSelector),
    m_HasNotSetItem( show_not_set ),
    m_dataFile( nullptr )
{
    ui->setupUi(this);

    if( m_HasNotSetItem )
        ui->cmbGrids->addItem( "NOT SET" );

    Project* project = Application::instance()->getProject();

    ObjectGroup* og = project->getDataFilesGroup();

    for( int i = 0; i < og->getChildCount(); ++i){
        File* varFile = (File*)og->getChildByIndex( i );
        if( varFile->getFileType() == "CARTESIANGRID" ){
            ui->cmbGrids->addItem( varFile->getIcon(), varFile->getName() );
        }
    }
}

CartesianGridSelector::~CartesianGridSelector()
{
    delete ui;
}

void CartesianGridSelector::onSelection(int index)
{
    m_dataFile = nullptr;
    Project* project = Application::instance()->getProject();
    ObjectGroup* og = project->getDataFilesGroup();
    for( int i = 0; i < og->getChildCount(); ++i){
        File* varFile = (File*)og->getChildByIndex( i );
        if( varFile->getFileType() == "CARTESIANGRID" ){
            if( varFile->getName() == ui->cmbGrids->currentText() ){
                m_dataFile = (DataFile*)varFile;
                emit cartesianGridSelected( (DataFile*)varFile );
                return;
            }
        }
    }
    //the user may select "NOT SET", so emit signal with null pointer.
    emit cartesianGridSelected( nullptr );
}
