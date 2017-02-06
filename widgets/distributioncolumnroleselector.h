#ifndef DISTRIBUTIONCOLUMNROLESELECTOR_H
#define DISTRIBUTIONCOLUMNROLESELECTOR_H

#include "../domain/roles.h"

#include <QWidget>

namespace Ui {
class DistributionColumnRoleSelector;
}

class DistributionColumnRoleSelector : public QWidget
{
    Q_OBJECT

public:
    explicit DistributionColumnRoleSelector(const QString label, QWidget *parent = 0);
    ~DistributionColumnRoleSelector();

    Roles::DistributionColumnRole getSelectedRole();

private:
    Ui::DistributionColumnRoleSelector *ui;
};

#endif // DISTRIBUTIONCOLUMNROLESELECTOR_H
