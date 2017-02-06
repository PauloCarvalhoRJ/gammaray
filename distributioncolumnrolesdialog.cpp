#include "distributioncolumnrolesdialog.h"
#include "ui_distributioncolumnrolesdialog.h"
#include "domain/univariatedistribution.h"
#include "domain/distributioncolumn.h"
#include "widgets/distributioncolumnroleselector.h"

DistributionColumnRolesDialog::DistributionColumnRolesDialog(const QString pathToDistrFile, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DistributionColumnRolesDialog),
    m_pathToDistrFile( pathToDistrFile )
{
    ui->setupUi(this);
    //instantiate a concrete subclass of Distribution
    UnivariateDistribution dist( m_pathToDistrFile );
    //make object populate its column component object collection
    dist.updateColumnCollection();
    //for each distribution file column
    for (int i = 0; i < dist.getChildCount(); ++i){
        DistributionColumn* dc = (DistributionColumn*)( dist.getChildByIndex( i ) );
        m_roleSelectors.append( new DistributionColumnRoleSelector( dc->getName() ) );
        ui->frmRoles->layout()->addWidget( m_roleSelectors.last() );
    }
}

DistributionColumnRolesDialog::~DistributionColumnRolesDialog()
{
    delete ui;
}

QMap<uint, Roles::DistributionColumnRole> DistributionColumnRolesDialog::getRoles()
{
    QMap<uint, Roles::DistributionColumnRole> result;
    for( int i = 0; i < m_roleSelectors.count(); ++i){
        DistributionColumnRoleSelector* dcrs = m_roleSelectors.at( i );
        result.insert( i+1, dcrs->getSelectedRole() );
    }
    return result;
}
