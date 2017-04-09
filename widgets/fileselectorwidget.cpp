#include "fileselectorwidget.h"
#include "ui_fileselectorwidget.h"
#include "../domain/project.h"
#include "../domain/application.h"
#include "../domain/objectgroup.h"
#include "../domain/file.h"

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
                     ( m_filesOfTypes == FileSelectorType::PDFs && varFile->getFileType() == "CATEGORYPDF" );
        if( toAdd ){
            ui->cmbFile->addItem( varFile->getIcon(), varFile->getName() );
        }
    }

    //adds files from the Data Files Group of type according to the types specified in m_filesOfTypes
    if( m_filesOfTypes == FileSelectorType::DataFiles ){
        og = project->getDataFilesGroup();
        for( int i = 0; i < og->getChildCount(); ++i){
            File* varFile = (File*)og->getChildByIndex( i );
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

void FileSelectorWidget::onSelection(int /*index*/)
{
    m_File = nullptr;
    Project* project = Application::instance()->getProject();

    const uint nogs = 2;
    ObjectGroup* ogs[nogs] = {project->getResourcesGroup(),
                              project->getDataFilesGroup()
                             };

    for( uint j = 0; j < nogs; ++j){
        ObjectGroup* og = ogs[j];
        for( int i = 0; i < og->getChildCount(); ++i){
            File* varFile = (File*)og->getChildByIndex( i );
            if( varFile->getName() == ui->cmbFile->currentText() ){
                m_File = (File*)varFile;
                emit fileSelected( m_File );
                if( m_File->isDataFile() )
                    emit dataFileSelected( (DataFile*)m_File );
                return;
            }
        }
    }
    //the user may select "NOT SET", so emit signal with null pointer.
    emit fileSelected( nullptr );
    emit dataFileSelected( nullptr );
}
