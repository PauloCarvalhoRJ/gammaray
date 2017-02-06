#include "distributioncolumnroleselector.h"
#include "ui_distributioncolumnroleselector.h"

DistributionColumnRoleSelector::DistributionColumnRoleSelector(const QString label, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DistributionColumnRoleSelector)
{
    ui->setupUi(this);

    ui->lblColumnName->setText( label );

    ui->cmbRole->addItem( Roles::getRoleIcon( Roles::DistributionColumnRole::UNDEFINED ),
                          Roles::getRoleText( Roles::DistributionColumnRole::UNDEFINED ),
                          QVariant( (uint) Roles::DistributionColumnRole::UNDEFINED ) );
    ui->cmbRole->addItem( Roles::getRoleIcon( Roles::DistributionColumnRole::VALUE ),
                          Roles::getRoleText( Roles::DistributionColumnRole::VALUE ),
                          QVariant( (uint) Roles::DistributionColumnRole::VALUE ) );
    ui->cmbRole->addItem( Roles::getRoleIcon( Roles::DistributionColumnRole::LOGVALUE ),
                          Roles::getRoleText( Roles::DistributionColumnRole::LOGVALUE ),
                          QVariant( (uint) Roles::DistributionColumnRole::LOGVALUE ) );
    ui->cmbRole->addItem( Roles::getRoleIcon( Roles::DistributionColumnRole::PVALUE ),
                          Roles::getRoleText( Roles::DistributionColumnRole::PVALUE ),
                          QVariant( (uint) Roles::DistributionColumnRole::PVALUE ) );
}

DistributionColumnRoleSelector::~DistributionColumnRoleSelector()
{
    delete ui;
}

Roles::DistributionColumnRole DistributionColumnRoleSelector::getSelectedRole()
{
    int selectedIndex = ui->cmbRole->currentIndex();
    return (Roles::DistributionColumnRole)ui->cmbRole->itemData( selectedIndex ).toUInt();
}
