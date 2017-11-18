#include "distributionfieldselector.h"
#include "ui_distributionfieldselector.h"

#include "domain/distribution.h"
#include "domain/distributioncolumn.h"
#include "domain/application.h"

DistributionFieldSelector::DistributionFieldSelector(Roles::DistributionColumnRole purpose, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DistributionFieldSelector),
    m_dist( nullptr ),
    m_purpose( purpose )
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

uint DistributionFieldSelector::getSelectedFieldGEOEASIndex()
{
    if( ! m_dist )
        return 0;
//    if( m_hasNotSetItem )
//        if( ui->cmbVariable->currentIndex() == 0 )
//            return 0;
    QString field_name = ui->cmbField->currentText();
    ProjectComponent *pc = m_dist->getChildByName( field_name );
    if( ! pc ){
        Application::instance()->logError("DistributionFieldSelector::getSelectedFieldGEOEASIndex(): field not found for name: " + field_name + ".");
        return 0;
    }
    uint index =((DistributionColumn*)pc)->getAttributeGEOEASgivenIndex();
    if( index < 1 )
        Application::instance()->logError("DistributionFieldSelector::getSelectedFieldGEOEASIndex(): invalid GEO-EAS index returned for field: " + field_name + ".");
    return index;
}

void DistributionFieldSelector::onListFields(Distribution *dist)
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
            Attribute *at_aspec = (Attribute*)pc;
            uint givenGEOEASindex = at_aspec->getAttributeGEOEASgivenIndex();
            ui->cmbField->addItem( pc->getIcon(), pc->getName() );
            if( m_purpose == Roles::DistributionColumnRole::PVALUE &&
                    givenGEOEASindex == dist->getTheColumnWithProbabilityRole() )
                ui->cmbField->setCurrentIndex( ui->cmbField->count()-1 );
            if( (m_purpose == Roles::DistributionColumnRole::VALUE ||
                 m_purpose == Roles::DistributionColumnRole::LOGVALUE) &&
                    givenGEOEASindex == dist->getTheColumnWithValueRole() )
                ui->cmbField->setCurrentIndex( ui->cmbField->count()-1 );
        }
    }
}
