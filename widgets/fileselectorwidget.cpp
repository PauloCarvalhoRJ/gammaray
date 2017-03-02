#include "fileselectorwidget.h"
#include "ui_fileselectorwidget.h"
#include "../domain/project.h"
#include "../domain/application.h"
#include "../domain/objectgroup.h"
#include "../domain/file.h"

FileSelectorWidget::FileSelectorWidget(FileSelectorType filesOfTypes, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FileSelectorWidget),
    m_filesOfTypes( filesOfTypes ),
    m_File( nullptr )
{
    ui->setupUi(this);

    Project* project = Application::instance()->getProject();

    //TODO: maybe list from the other groups as well
    ObjectGroup* og = project->getResourcesGroup();

    //adds files of type according to the types specified in m_filesOfTypes
    for( int i = 0; i < og->getChildCount(); ++i){
        File* varFile = (File*)og->getChildByIndex( i );
        bool toAdd = ( m_filesOfTypes == FileSelectorType::CDFs && varFile->getFileType() == "THRESHOLDCDF" ) ||
                     ( m_filesOfTypes == FileSelectorType::PDFs && varFile->getFileType() == "CATEGORYPDF" );
        if( toAdd ){
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
    ObjectGroup* og = project->getResourcesGroup();
    for( int i = 0; i < og->getChildCount(); ++i){
        File *varFile = (File*)og->getChildByIndex( i );
        if( varFile->getName() == ui->cmbFile->currentText() ){
            return (File*)varFile;
        }
    }
    return nullptr;
}

void FileSelectorWidget::onSelection(int /*index*/)
{
    m_File = nullptr;
    Project* project = Application::instance()->getProject();

    //TODO: maybe search in the other groups as well
    ObjectGroup* og = project->getResourcesGroup();
    for( int i = 0; i < og->getChildCount(); ++i){
        File* varFile = (File*)og->getChildByIndex( i );
        if( varFile->getName() == ui->cmbFile->currentText() ){
            m_File = (File*)varFile;
            emit fileSelected( m_File );
            return;
        }
    }
    //the user may select "NOT SET", so emit signal with null pointer.
    emit fileSelected( nullptr );
}
