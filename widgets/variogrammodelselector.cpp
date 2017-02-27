#include "variogrammodelselector.h"
#include "ui_variogrammodelselector.h"
#include "../domain/project.h"
#include "../domain/application.h"
#include "../domain/objectgroup.h"
#include "../domain/file.h"

VariogramModelSelector::VariogramModelSelector(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VariogramModelSelector)
{
    ui->setupUi(this);
    updateList();
}

VariogramModelSelector::~VariogramModelSelector()
{
    delete ui;
}

VariogramModel *VariogramModelSelector::getSelectedVModel()
{
    Project* project = Application::instance()->getProject();
    ObjectGroup* og = project->getVariogramsGroup();
    for( int i = 0; i < og->getChildCount(); ++i){
        File* varFile = (File*)og->getChildByIndex( i );
        if( varFile->getFileType() == "VMODEL" ){
            if( varFile->getName() == ui->cmbVariogramModels->currentText() )
                return (VariogramModel*)varFile;
        }
    }
    return nullptr;
}

void VariogramModelSelector::updateList()
{
    ui->cmbVariogramModels->clear();

    ////////TODO: consider refactoring this code with the similar one in VariogramModelList's constructor.//////////
    Project* project = Application::instance()->getProject();

    ObjectGroup* og = project->getVariogramsGroup();

    for( int i = 0; i < og->getChildCount(); ++i){
        File* varFile = (File*)og->getChildByIndex( i );
        if( varFile->getFileType() == "VMODEL" ){
            ui->cmbVariogramModels->addItem( varFile->getIcon(), varFile->getName() );
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

void VariogramModelSelector::selectVariogram(const QString name)
{
    ui->cmbVariogramModels->setCurrentText( name );
    emit variogramSelected();
}

void VariogramModelSelector::onVariogramSelected(int /*index*/)
{
    emit variogramSelected();
}
