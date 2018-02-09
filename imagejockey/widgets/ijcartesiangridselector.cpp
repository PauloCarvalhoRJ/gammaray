#include "ijcartesiangridselector.h"
#include "ui_ijcartesiangridselector.h"
#include "../domain/project.h"
#include "../domain/application.h"
#include "../domain/objectgroup.h"
#include "../domain/file.h"

IJCartesianGridSelector::IJCartesianGridSelector(bool show_not_set, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IJCartesianGridSelector),
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

IJCartesianGridSelector::~IJCartesianGridSelector()
{
    delete ui;
}

void IJCartesianGridSelector::onSelection(int /*index*/)
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
