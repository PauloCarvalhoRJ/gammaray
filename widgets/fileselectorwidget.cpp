#include "fileselectorwidget.h"
#include "ui_fileselectorwidget.h"
#include "../domain/project.h"
#include "../domain/application.h"
#include "../domain/objectgroup.h"
#include "../domain/file.h"
#include "../domain/datafile.h"
#include "../domain/distribution.h"

FileSelectorWidget::FileSelectorWidget(FileSelectorType filesOfTypes, bool show_not_set, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FileSelectorWidget),
    m_filesOfTypes( filesOfTypes ),
    m_File( nullptr ),
    m_HasNotSetItem( show_not_set )
{
    ui->setupUi(this);

    Project* project = Application::instance()->getProject();

    if( m_HasNotSetItem )
        ui->cmbFile->addItem( "NOT SET" );

    //adds files from the Resource Files Group of type according to the types specified in m_filesOfTypes
    ObjectGroup* og = project->getResourcesGroup();
    for( int i = 0; i < og->getChildCount(); ++i){
        File* varFile = (File*)og->getChildByIndex( i );
        bool toAdd = ( m_filesOfTypes == FileSelectorType::CDFs && varFile->getFileType() == "THRESHOLDCDF" ) ||
                     ( m_filesOfTypes == FileSelectorType::PDFs && varFile->getFileType() == "CATEGORYPDF" ) ||
                     ( m_filesOfTypes == FileSelectorType::CategoryDefinitions && varFile->getFileType() == "CATEGORYDEFINITION" ) ||
                     ( m_filesOfTypes == FileSelectorType::CDsAndCDFs && (varFile->getFileType() == "CATEGORYDEFINITION" || varFile->getFileType() == "THRESHOLDCDF") ) ||
                     ( m_filesOfTypes == FileSelectorType::CDsCDFsandPDFs && (varFile->getFileType() == "CATEGORYDEFINITION" || varFile->getFileType() == "THRESHOLDCDF" || varFile->getFileType() == "CATEGORYPDF") );
        if( toAdd ){
            ui->cmbFile->addItem( varFile->getIcon(), varFile->getName() );
        }
    }

    //adds files from the Data Files Group of type according to the types specified in m_filesOfTypes
    if( m_filesOfTypes == FileSelectorType::DataFiles ||
        m_filesOfTypes == FileSelectorType::CartesianGrids ||
        m_filesOfTypes == FileSelectorType::PointSets ){
        og = project->getDataFilesGroup();
        for( int i = 0; i < og->getChildCount(); ++i){
            File* varFile = (File*)og->getChildByIndex( i );
            bool toAdd = ( m_filesOfTypes == FileSelectorType::DataFiles ) ||
                         ( m_filesOfTypes == FileSelectorType::PointSets && varFile->getFileType() == "POINTSET" ) ||
                         ( m_filesOfTypes == FileSelectorType::CartesianGrids && varFile->getFileType() == "CARTESIANGRID" );
            if( toAdd )
                ui->cmbFile->addItem( varFile->getIcon(), varFile->getName() );
        }
    }

	//adds files from the Distributions Group of type according to the types specified in m_filesOfTypes
	if( m_filesOfTypes == FileSelectorType::Bidistributions ){
		og = project->getDistributionsGroup();
		for( int i = 0; i < og->getChildCount(); ++i){
			File* varFile = (File*)og->getChildByIndex( i );
			bool toAdd = ( ( m_filesOfTypes == FileSelectorType::Bidistributions && varFile->getFileType() == "BIDIST" ) );
			if( toAdd )
				ui->cmbFile->addItem( varFile->getIcon(), varFile->getName() );
		}
	}
}

FileSelectorWidget::~FileSelectorWidget()
{
    delete ui;
}

File *FileSelectorWidget::getSelectedFile()
{
    Project* project = Application::instance()->getProject();

    const uint nogs = 2;
    ObjectGroup* ogs[nogs] = {project->getResourcesGroup(),
                              project->getDataFilesGroup()
                             };

    for( uint j = 0; j < nogs; ++j){
        ObjectGroup* og = ogs[j];
        for( int i = 0; i < og->getChildCount(); ++i){
            File *varFile = (File*)og->getChildByIndex( i );
            if( varFile->getName() == ui->cmbFile->currentText() ){
                return (File*)varFile;
            }
        }
    }
    return nullptr;
}

void FileSelectorWidget::setCaption(QString caption)
{
    ui->lblCaption->setText( caption );
}

void FileSelectorWidget::onSelection(int /*index*/)
{
    m_File = nullptr;
    Project* project = Application::instance()->getProject();

	//ATTENTION: WHENEVER A NEW GROUP IS USED IN THE CONSTRUCTOR,
	//           YOU NEED TO UPDATE THIS NUMBER AND THE ARRAY BELOW.
	const uint nogs = 3;
    ObjectGroup* ogs[nogs] = {project->getResourcesGroup(),
							  project->getDataFilesGroup(),
							  project->getDistributionsGroup()
                             };

    for( uint j = 0; j < nogs; ++j){
        ObjectGroup* og = ogs[j];
        for( int i = 0; i < og->getChildCount(); ++i){
            File* varFile = (File*)og->getChildByIndex( i );
            if( varFile->getName() == ui->cmbFile->currentText() ){
                m_File = (File*)varFile;
                emit fileSelected( m_File );
                if( m_File->isDataFile() )
                    emit dataFileSelected( dynamic_cast<DataFile*>(m_File) );
				if( m_File->isDistribution() )
					emit distributionSelected( dynamic_cast<Distribution*>(m_File) );
				return;
            }
        }
    }
    //the user may select "NOT SET", so emit signal with null pointer.
    emit fileSelected( nullptr );
    emit dataFileSelected( nullptr );
	emit distributionSelected( nullptr );
}
