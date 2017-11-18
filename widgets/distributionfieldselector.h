#ifndef DISTRIBUTIONFIELDSELECTOR_H
#define DISTRIBUTIONFIELDSELECTOR_H

#include <QWidget>
#include "domain/roles.h"

namespace Ui {
class DistributionFieldSelector;
}

class Distribution;
class DistributionColumn;

class DistributionFieldSelector : public QWidget
{
    Q_OBJECT

public:
    explicit DistributionFieldSelector( Roles::DistributionColumnRole purpose, QWidget *parent = 0);
    ~DistributionFieldSelector();

    /** Returns the name of the selected variable. */
    QString getSelectedVariableName();

    /** Returns the GEO-EAS column index of the selected distribution field.
      * @return Zero if no variable is selected or m_dist is null.
      */
    uint getSelectedFieldGEOEASIndex();

signals:
    void fieldSelected( DistributionColumn* field );

public slots:
    void onListFields(Distribution *dist);

private slots:
    void onSelection( int index );

private:
    Ui::DistributionFieldSelector *ui;
    Distribution *m_dist;
    Roles::DistributionColumnRole m_purpose;
};

#endif // DISTRIBUTIONFIELDSELECTOR_H
