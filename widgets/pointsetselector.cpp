#include "pointsetselector.h"
#include "ui_pointsetselector.h"
#include "../domain/project.h"
#include "../domain/application.h"
#include "../domain/objectgroup.h"
#include "../domain/file.h"

PointSetSelector::PointSetSelector(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PointSetSelector),
    m_dataFile( nullptr )
{
    ui->setupUi(this);

    Project* project = Application::instance()->getProject();

    ObjectGroup* og = project->getDataFilesGroup();

    for( int i = 0; i < og->getChildCount(); ++i){
        File* varFile = (File*)og->getChildByIndex( i );
        if( varFile->getFileType() == "POINTSET" ){
            ui->cmbGrids->addItem( varFile->getIcon(), varFile->getName() );
        }
    }
}

PointSetSelector::~PointSetSelector()
{
    delete ui;
}

void PointSetSelector::onSelection(int index)
{
    m_dataFile = nullptr;
    Project* project = Application::instance()->getProject();
    ObjectGroup* og = project->getDataFilesGroup();
    for( int i = 0; i < og->getChildCount(); ++i){
        File* varFile = (File*)og->getChildByIndex( i );
        if( varFile->getFileType() == "POINTSET" ){
            if( varFile->getName() == ui->cmbGrids->currentText() ){
                m_dataFile = (DataFile*)varFile;
                emit pointSetSelected( m_dataFile );
            }
        }
    }
}
