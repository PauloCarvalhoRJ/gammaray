#include "distributionfieldselector.h"
#include "ui_distributionfieldselector.h"

#include "domain/distribution.h"
#include "domain/distributioncolumn.h"
#include "domain/application.h"

DistributionFieldSelector::DistributionFieldSelector(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DistributionFieldSelector),
    m_dist( nullptr )
{
    ui->setupUi(this);
}

DistributionFieldSelector::~DistributionFieldSelector()
{
    delete ui;
}

void DistributionFieldSelector::onSelection(int /*index*/)
{
    if( ! m_dist ){
        Application::instance()->logWarn("DistributionFieldSelector::onSelection(): Attempt to call this slot with m_dist == nullptr. Ignoring.");
        return;
    }
    //by selecting a column name, surely the object is a DistributionColumn
    DistributionColumn* dc = (DistributionColumn*)m_dist->getChildByName( ui->cmbField->currentText() );
    if( ! dc ){
        Application::instance()->logWarn("DistributionFieldSelector::onSelection(): Selection event resulted in null distribution column. Ignoring.");
        return;
    }
    emit fieldSelected( dc );
}

QString DistributionFieldSelector::getSelectedVariableName()
{
    return ui->cmbField->currentText();
}

void DistributionFieldSelector::onListVariables(Distribution *dist)
{
    m_dist = dist;
    ui->cmbField->clear();
    if( ! dist )
        return;
    std::vector<ProjectComponent*> all_contained_objects;
    dist->getAllObjects( all_contained_objects );
    std::vector<ProjectComponent*>::iterator it = all_contained_objects.begin();
    for(; it != all_contained_objects.end(); ++it){
        ProjectComponent* pc = (ProjectComponent*)(*it);
        if( pc->isAttribute() ){
            ui->cmbField->addItem( pc->getIcon(), pc->getName() );
        }
    }
}
