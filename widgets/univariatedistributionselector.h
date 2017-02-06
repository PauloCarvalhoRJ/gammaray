#ifndef UNIVARIATEDISTRIBUTIONSELECTOR_H
#define UNIVARIATEDISTRIBUTIONSELECTOR_H

#include <QWidget>

class UnivariateDistribution;

namespace Ui {
class UnivariateDistributionSelector;
}

class UnivariateDistributionSelector : public QWidget
{
    Q_OBJECT

public:
    explicit UnivariateDistributionSelector(QWidget *parent = 0);
    ~UnivariateDistributionSelector();

    /** Returns the pointer to the selected univariate distribution or nullptr if none is selected. */
    UnivariateDistribution* getSelectedDistribution();

private:
    Ui::UnivariateDistributionSelector *ui;
};

#endif // UNIVARIATEDISTRIBUTIONSELECTOR_H
