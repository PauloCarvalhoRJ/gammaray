#include "cartesiangridselector.h"
#include "ui_cartesiangridselector.h"
#include "../domain/project.h"
#include "../domain/application.h"
#include "../domain/objectgroup.h"
#include "../domain/file.h"
#include "../domain/datafile.h"

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

	std::vector< ProjectComponent*> allChildObjects;
	og->getAllObjects( allChildObjects );

	std::vector< ProjectComponent*>::iterator it = allChildObjects.begin();
	for( ; it != allChildObjects.end(); ++it ){
		File* varFile = dynamic_cast<File*>( *it );
		if( varFile && varFile->getFileType() == "CARTESIANGRID" ){
			ui->cmbGrids->addItem( varFile->getIcon(), varFile->getName() );
		}
	}
}

CartesianGridSelector::~CartesianGridSelector()
{
    delete ui;
}

int CartesianGridSelector::getCurrentIndex()
{
    return ui->cmbGrids->currentIndex();
}

void CartesianGridSelector::onSelection(int /*index*/)
{
    m_dataFile = nullptr;
    Project* project = Application::instance()->getProject();
    ObjectGroup* og = project->getDataFilesGroup();

	std::vector< ProjectComponent*> allChildObjects;
	og->getAllObjects( allChildObjects );

	std::vector< ProjectComponent*>::iterator it = allChildObjects.begin();
	for( ; it != allChildObjects.end(); ++it ){
		File* varFile = dynamic_cast<File*>( *it );
		if( varFile && varFile->getFileType() == "CARTESIANGRID" ){
			if( varFile->getName() == ui->cmbGrids->currentText() ){
				m_dataFile = dynamic_cast<DataFile*>(varFile);
				emit cartesianGridSelected( m_dataFile );
				return;
			}
		}
	}

    //the user may select "NOT SET", so emit signal with null pointer.
    emit cartesianGridSelected( nullptr );
}
