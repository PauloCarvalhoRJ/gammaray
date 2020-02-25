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

    /** Sets an optional caption text. */
    void setCaption( QString caption );

    /** Sets an optional background color for the caption text.
     * Text color is automatically set to either black or white depending on how dark or light
     * is the background color according to the criterion defined in Util::isDark().
     */
    void setCaptionBGColor( const QColor& color );

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
