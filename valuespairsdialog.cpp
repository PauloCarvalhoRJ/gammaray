#include "valuespairsdialog.h"
#include "ui_valuespairsdialog.h"
#include "widgets/valuepairvertical.h"
#include "domain/file.h"
#include "domain/application.h"
#include "domain/project.h"
#include "domain/thresholdcdf.h"
#include "domain/categorypdf.h"
#include <QDir>
#include <QMessageBox>

ValuesPairsDialog::ValuesPairsDialog(File *valuePairsFile, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ValuesPairsDialog),
    m_valuePairsFile( valuePairsFile )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //set the icon to signal to the user the type of value pair list
    ui->lblTypeIcon->setPixmap( m_valuePairsFile->getIcon().pixmap(16, 16) );

    //set the edit box for file name
    ui->txtFileName->setText( m_valuePairsFile->getFileName() );

    //loads the file contents
    if( m_valuePairsFile->exists() )
        m_valuePairsFile->readFromFS();

    //prepare the interface according to the specific file
    if( m_valuePairsFile->getFileType() == "THRESHOLDCDF" ){
        ui->lblWhat->setText("<html><strong>Threshold cumulative density function definition.</strong></html>");
        this->setWindowTitle( "Create/Edit threshold c.d.f." );
        ui->lbl1stCaption->setText("<html><strong>thresholds:</strong></html>");
        ui->lbl2ndCaption->setText("<html><strong>cumulative probabilities:");
        ThresholdCDF* tcdf = (ThresholdCDF*)m_valuePairsFile;
        for( int i = 0; i < tcdf->getPairCount(); ++i){
            onAddPair();
            ValuePairVertical* vpvWidget = (ValuePairVertical*)m_pairWidgets.last();
            vpvWidget->set1st( QString::number( tcdf->get1stValue( i ) ) );
            vpvWidget->set2nd( QString::number( tcdf->get2ndValue( i ) ) );
        }
    } else if(m_valuePairsFile->getFileType() == "CATEGORYPDF"){
        ui->lblWhat->setText("<html><strong>Category probability density function definition.</strong></html>");
        this->setWindowTitle( "Create/Edit category p.d.f." );
        ui->lbl1stCaption->setText("<html><strong>category ids:</strong></html>");
        ui->lbl2ndCaption->setText("<html><strong>probabilities:</strong></html>");
        CategoryPDF* cpdf = (CategoryPDF*)m_valuePairsFile;
        for( int i = 0; i < cpdf->getPairCount(); ++i){
            onAddPair();
            ValuePairVertical* vpvWidget = (ValuePairVertical*)m_pairWidgets.last();
            vpvWidget->set1st( QString::number( cpdf->get1stValue( i ) ) );
            vpvWidget->set2nd( QString::number( cpdf->get2ndValue( i ) ) );
        }
    }

    adjustSize();
}

ValuesPairsDialog::~ValuesPairsDialog()
{
    delete ui;
}

void ValuesPairsDialog::onAddPair()
{
    ValuePairVertical* vpvWidget = new ValuePairVertical();
    m_pairWidgets.push_back( vpvWidget );
    ui->hlayoutPairs->addWidget( vpvWidget );
}

void ValuesPairsDialog::onRemovePair()
{
    if( ! m_pairWidgets.empty() ){
        ValuePairVertical* vpvWidget = (ValuePairVertical*)m_pairWidgets.takeLast();
        ui->hlayoutPairs->removeWidget( vpvWidget );
        delete vpvWidget;
    }
}

void ValuesPairsDialog::onSave()
{

    if( ui->txtFileName->text().trimmed().isEmpty() ){
        QMessageBox::warning( this, "Warning", "Please, give a name to the new file.");
        return;
    }

    if( m_pairWidgets.isEmpty() ){
        QMessageBox::warning( this, "Warning", "No values to save.");
        return;
    }

    //sets the file path as the project's directory + the file name given by the user
    m_valuePairsFile->setPath( Application::instance()->getProject()->getPath() +
                               QDir::separator() +
                               ui->txtFileName->text() );

    //determine whether the file exists (create new or overwrite)
    bool isNew = ! m_valuePairsFile->exists();

    //clear any previously loaded pairs (will write the user-entered pairs)
    if( ! isNew )
        m_valuePairsFile->clearLoadedContents();

    //read the values entered by the user
    //TODO: the design of this with these if's is not very good.
    QList<QWidget*>::iterator it = m_pairWidgets.begin();
    for(; it  != m_pairWidgets.end(); ++it){
        ValuePairVertical* vpvWidget = (ValuePairVertical*)*it;
        if( m_valuePairsFile->getFileType() == "THRESHOLDCDF" ){
            ThresholdCDF* tcdfAspect = (ThresholdCDF*)m_valuePairsFile;
            tcdfAspect->addPair( vpvWidget->get1st().toDouble(),
                                 vpvWidget->get2nd().toDouble() );
        } else if( m_valuePairsFile->getFileType() == "CATEGORYPDF" ){
            CategoryPDF* cpdfAspect = (CategoryPDF*)m_valuePairsFile;
            cpdfAspect->addPair( vpvWidget->get1st().toInt(),
                                 vpvWidget->get2nd().toDouble() );
        }
    }

    //save the pairs to file system.
    m_valuePairsFile->writeToFS();

    //if the file is new, then adds it to the project.
    //TODO: the design of this with these if's is not very good.
    if( isNew ){
        if( m_valuePairsFile->getFileType() == "THRESHOLDCDF" )
            Application::instance()->getProject()->registerThresholdCDF( (ThresholdCDF*)m_valuePairsFile );
        else if( m_valuePairsFile->getFileType() == "CATEGORYPDF" )
            Application::instance()->getProject()->registerCategoryPDF( (CategoryPDF*)m_valuePairsFile );
    }
}
