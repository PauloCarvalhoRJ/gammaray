#include "triadseditordialog.h"
#include "ui_triadseditordialog.h"
#include "domain/file.h"
#include "domain/application.h"
#include "domain/project.h"
#include <QMessageBox>
#include <QDir>

TriadsEditorDialog::TriadsEditorDialog(File *triadsFile, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TriadsEditorDialog),
    m_triadsFile( triadsFile )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //set the icon to signal to the user the type of value pair list
    ui->lblIcon->setPixmap( m_triadsFile->getIcon().pixmap(16, 16) );

    //set the edit box for file name
    ui->txtFileName->setText( m_triadsFile->getFileName() );

    //loads the file contents
    if( m_triadsFile->exists() )
        m_triadsFile->readFromFS();

    //prepare the interface according to the specific file
    if( m_triadsFile->getFileType() == "CATEGORYDEFINITION" ){
        ui->lblCaption->setText("<html><strong>Categories definition.</strong></html>");
        this->setWindowTitle( "Create/Edit category definition file." );
    }

    //add widgets for each file record.
    for( int i = 0; i < m_triadsFile->getContentsCount(); ++i){
        //get the widget filled with a triplet.
        QWidget *w = m_triadsFile->createWidgetFilledWithContentElement( i );

        //not all files reimplement File::createWidgetFilledWithContentElement(int), which returns nullptr by default.
        if( ! w ){
            QMessageBox::critical( this, "Error", QString("Files of type ") + m_triadsFile->getFileType() +
                                   QString(" are not reimplementing File::createWidgetFilledWithContentElement(int)."));
        } else {
            m_tripletWidgets.push_back( w );
            ui->frmTriplets->layout()->addWidget( w );
        }
    }
}

TriadsEditorDialog::~TriadsEditorDialog()
{
    Application::instance()->logInfo("TriadsEditorDialog destroyed.");
    delete ui;
}

void TriadsEditorDialog::onAddTriplet()
{
    //get the widget made to edit a triplet.
    QWidget *w = m_triadsFile->createContentElementWidget();

    //not all files reimplement File::createContentElementWidget(), which returns nullptr by default.
    if( ! w ){
        QMessageBox::critical( this, "Error", QString("Files of type ") + m_triadsFile->getFileType() +
                               QString(" are not reimplementing File::createContentElementWidget()."));
        return;
    }

    m_tripletWidgets.push_back( w );
    ui->frmTriplets->layout()->addWidget( w );
}

void TriadsEditorDialog::onRemoveTriplet()
{
    if( ! m_tripletWidgets.empty() ){
        QWidget* w = m_tripletWidgets.takeLast();
        ui->frmTriplets->layout()->removeWidget( w );
        delete w;
    }
}

void TriadsEditorDialog::onSave()
{
    if( ui->txtFileName->text().trimmed().isEmpty() ){
        QMessageBox::warning( this, "Warning", "Please, give a name to the new file.");
        return;
    }

    if( m_tripletWidgets.isEmpty() ){
        QMessageBox::warning( this, "Warning", "No values to save.");
        return;
    }

    //sets the file path as the project's directory + the file name given by the user
    m_triadsFile->setPath( Application::instance()->getProject()->getPath() +
                           QDir::separator() +
                           ui->txtFileName->text() );

    //determine whether the file exists (create new or overwrite)
    bool isNew = ! m_triadsFile->exists();

    //clear any previously loaded pairs (will write the user-entered pairs)
    if( ! isNew )
        m_triadsFile->clearLoadedContents();

    //read the values entered by the user
    QList<QWidget*>::iterator it = m_tripletWidgets.begin();
    for( ; it  != m_tripletWidgets.end(); ++it )
        m_triadsFile->addContentElementFromWidget( *it );

    //save the pairs to file system.
    m_triadsFile->writeToFS();

    //if the file is new, then adds it to the project.
    if( isNew ){
        //By default files are kept as resources.  Use File::getFileType() to assign it elsewhere in the project.
        Application::instance()->getProject()->registerFileAsResource( m_triadsFile );
    }
}
