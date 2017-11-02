#include "univariatedistributionselector.h"
#include "ui_univariatedistributionselector.h"
#include "../domain/project.h"
#include "../domain/application.h"
#include "../domain/objectgroup.h"
#include "../domain/file.h"

UnivariateDistributionSelector::UnivariateDistributionSelector(bool show_not_set, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UnivariateDistributionSelector),
    m_hasNotSetItem( show_not_set ),
    m_dist( nullptr )
{
    ui->setupUi(this);

    Project* project = Application::instance()->getProject();

    ObjectGroup* og = project->getDistributionsGroup();

    if( m_hasNotSetItem )
        ui->cmbUnivariateDistributions->addItem( "NOT SET" );

    for( int i = 0; i < og->getChildCount(); ++i){
        File* varFile = (File*)og->getChildByIndex( i );
        if( varFile->getFileType() == "UNIDIST" ){
            ui->cmbUnivariateDistributions->addItem( varFile->getIcon(), varFile->getName() );
        }
    }
}

UnivariateDistributionSelector::~UnivariateDistributionSelector()
{
    delete ui;
}

UnivariateDistribution *UnivariateDistributionSelector::getSelectedDistribution()
{
    if( ui->cmbUnivariateDistributions->currentIndex() == -1 ){
        return nullptr;
    }
    Project* project = Application::instance()->getProject();
    ObjectGroup* og = project->getDistributionsGroup();
    for( int i = 0; i < og->getChildCount(); ++i){
        File* varFile = (File*)og->getChildByIndex( i );
        if( varFile->getFileType() == "UNIDIST" ){
            if( varFile->getName() == ui->cmbUnivariateDistributions->currentText() )
                return (UnivariateDistribution*)varFile;
        }
    }
    return nullptr;
}

void UnivariateDistributionSelector::onSelection(int /*index*/)
{
    m_dist = nullptr;
    Project* project = Application::instance()->getProject();
    ObjectGroup* og = project->getDistributionsGroup();
    for( int i = 0; i < og->getChildCount(); ++i){
        File* varFile = (File*)og->getChildByIndex( i );
        if( varFile->getFileType() == "UNIDIST" ){
            if( varFile->getName() == ui->cmbUnivariateDistributions->currentText() ){
                m_dist = (Distribution*)varFile;
                emit distributionSelected( m_dist );
                return;
            }
        }
    }
    //the user may select "NOT SET", so emit signal with null pointer.
    emit distributionSelected( nullptr );
}
