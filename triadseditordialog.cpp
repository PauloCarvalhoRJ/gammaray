#include "triadseditordialog.h"
#include "ui_triadseditordialog.h"
#include "domain/file.h"
#include "domain/application.h"
#include "domain/project.h"
#include "domain/categorydefinition.h"
#include "domain/univariatecategoryclassification.h"
#include <QMessageBox>
#include <QDir>

TriadsEditorDialog::TriadsEditorDialog(File *triadsFile, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TriadsEditorDialog),
    m_triadsFile( triadsFile )
{
    ui->setupUi(this);

    ui->btnOK->hide();

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
    //TODO: REFACTOR: add method to return a presentation text in the File interface to remove this If.
    if( m_triadsFile->getFileType() == "CATEGORYDEFINITION" ){
        ui->lblCaption->setText("<html><strong>Categories definition.</strong></html>");
        this->setWindowTitle( "Create/Edit category definition file." );
    } else if( m_triadsFile->getFileType() == "UNIVARIATECATEGORYCLASSIFICATION" ){
        ui->lblCaption->setText("<html><strong>Univariate category classification.</strong></html>");
        this->setWindowTitle( "Create/Edit Univariate category classification file." );
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

void TriadsEditorDialog::showOKbutton()
{
    this->ui->btnOK->show();
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

    //determine whether the file is present as an object in the project tree.
    bool isInProject = Application::instance()->getProject()->fileIsChild( m_triadsFile );

    //make a path from the name entered by the user
    QString path_entered_by_user = Application::instance()->getProject()->getPath() +
            "/" + //Qt translates forward slashes to backslash in Windows
            ui->txtFileName->text();
    QFile intended_path( path_entered_by_user );

    //determine whether the intended file physically exists (create new or overwrite)
    bool isNew = ! intended_path.exists();

    //if object already exists in the project tree but the user gave another name
    //then it is necessary to create a new file object.
    if( isNew && isInProject ){
        File* new_file;
        if( m_triadsFile->getFileType() == "CATEGORYDEFINITION" ){
            new_file = new CategoryDefinition( path_entered_by_user );
        } else if( m_triadsFile->getFileType() == "UNIVARIATECATEGORYCLASSIFICATION" ){
            //get the same CategoryDefinition and the file path
            UnivariateCategoryClassification* uccAspect = (UnivariateCategoryClassification*)m_triadsFile;
            new_file = new UnivariateCategoryClassification( uccAspect->getCategoryDefinition(), path_entered_by_user);
        } else {
            Application::instance()->logError("ERROR: TriadsEditorDialog::onSave(): triads files of type " + m_triadsFile->getFileType() + " not supported." );
        }
        //the pointer now points to the new file
        m_triadsFile = new_file;
    } else if( isNew ){  //physical file is new and object is not in the project tree
        //set the path of the new file object
        m_triadsFile->setPath( path_entered_by_user );
    }

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

void TriadsEditorDialog::onOK()
{
    //read the values entered by the user, just in case the file was not saved
    m_triadsFile->clearLoadedContents(); //clear any previously loaded values
    QList<QWidget*>::iterator it = m_tripletWidgets.begin();
    for( ; it  != m_tripletWidgets.end(); ++it )
        m_triadsFile->addContentElementFromWidget( *it );
    //emit the accepted() signal to close the dialog with acceptance.
    emit accepted();
}
