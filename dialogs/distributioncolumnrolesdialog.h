#ifndef DISTRIBUTIONCOLUMNROLESDIALOG_H
#define DISTRIBUTIONCOLUMNROLESDIALOG_H

#include "domain/roles.h"

#include <QDialog>

class DistributionColumnRoleSelector;

namespace Ui {
class DistributionColumnRolesDialog;
}

class DistributionColumnRolesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DistributionColumnRolesDialog( const QString pathToDistrFile, QWidget *parent = 0);
    ~DistributionColumnRolesDialog();
    /** Returns the pairs of GEO-EAS (1st is 1) columns indexes and their respective user-given roles.*/
    QMap<uint, Roles::DistributionColumnRole> getRoles();

private:
    Ui::DistributionColumnRolesDialog *ui;
    QString m_pathToDistrFile;
    QList<DistributionColumnRoleSelector*> m_roleSelectors;
};

#endif // DISTRIBUTIONCOLUMNROLESDIALOG_H
