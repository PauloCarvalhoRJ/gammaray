#ifndef UNIVARIATEDISTRIBUTIONSELECTOR_H
#define UNIVARIATEDISTRIBUTIONSELECTOR_H

#include <QWidget>

class UnivariateDistribution;
class Distribution;

namespace Ui {
class UnivariateDistributionSelector;
}

class UnivariateDistributionSelector : public QWidget
{
    Q_OBJECT

public:
    explicit UnivariateDistributionSelector(bool show_not_set = false, QWidget *parent = 0);
    ~UnivariateDistributionSelector();

    /** Returns the pointer to the selected univariate distribution or nullptr if none is selected. */
    UnivariateDistribution* getSelectedDistribution();

signals:
    void distributionSelected( Distribution* dist );

private:
    Ui::UnivariateDistributionSelector *ui;
    bool m_hasNotSetItem;
    Distribution *m_dist;

private slots:
    void onSelection( int index );

};

#endif // UNIVARIATEDISTRIBUTIONSELECTOR_H
