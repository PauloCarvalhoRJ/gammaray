#include "variogrammodellist.h"
#include "ui_variogrammodellist.h"
#include "domain/application.h"
#include "domain/project.h"
#include "domain/objectgroup.h"
#include "domain/file.h"

VariogramModelList::VariogramModelList(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VariogramModelList)
{
    ui->setupUi(this);

    Project* project = Application::instance()->getProject();

    ObjectGroup* og = project->getVariogramsGroup();

    for( int i = 0; i < og->getChildCount(); ++i){
        File* varFile = (File*)og->getChildByIndex( i );
        if( varFile->getFileType() == "VMODEL" ){
            ui->lstVariogramModels->addItem(new QListWidgetItem(varFile->getIcon(), varFile->getName()));
        }
    }

}

VariogramModelList::~VariogramModelList()
{
    delete ui;
}

VariogramModel *VariogramModelList::getSelectedVModel()
{
    if( ui->lstVariogramModels->selectedItems().size() != 1 ){
        return nullptr;
    }
    Project* project = Application::instance()->getProject();
    ObjectGroup* og = project->getVariogramsGroup();
    for( int i = 0; i < og->getChildCount(); ++i){
        File* varFile = (File*)og->getChildByIndex( i );
        if( varFile->getFileType() == "VMODEL" ){
            if( varFile->getName() == ui->lstVariogramModels->selectedItems()[0]->text() )
                return (VariogramModel*)varFile;
        }
    }
    return nullptr;
}

void VariogramModelList::onVariogramSelection(QModelIndex /*index*/)
{
    emit variogramClicked();
}
