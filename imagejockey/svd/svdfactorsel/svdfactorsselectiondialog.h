#ifndef SVDFactorsSelectionDialog_H
#define SVDFactorsSelectionDialog_H

#include <QDialog>

class SVDFactorsSelectionChartView;

namespace Ui {
class SVDFactorsSelectionDialog;
}

class SVDFactorsSelectionDialog : public QDialog
{
    Q_OBJECT

public:
	explicit SVDFactorsSelectionDialog( const std::vector<double>& weights, QWidget *parent = 0);
    ~SVDFactorsSelectionDialog();

    /** Returns zero if the user didn't make a selection. */
    int getNumberOfFactors(){ return m_numberOfFactors; }

private:
    Ui::SVDFactorsSelectionDialog *ui;
	std::vector<double> m_weights;
	SVDFactorsSelectionChartView* m_factorsSelChartView;
    int m_numberOfFactors;

private slots:
    void numberOfFactorsSelected( int number ){ m_numberOfFactors = number; }
};

#endif // SVDFactorsSelectionDialog_H
