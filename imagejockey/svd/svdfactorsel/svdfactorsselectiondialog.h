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
    explicit SVDFactorsSelectionDialog(const std::vector<double>& weights,
                                       bool deleteSelfOnClose,
                                       QWidget *parent = 0);
    ~SVDFactorsSelectionDialog();

    int getSelectedNumberOfFactors( ){ return m_numberOfFactors; }


signals:
    /** If zero, the user didn't make a selection. */
    void numberOfFactorsSelected( int number );

private:
    Ui::SVDFactorsSelectionDialog *ui;
	std::vector<double> m_weights;
	SVDFactorsSelectionChartView* m_factorsSelChartView;
    int m_numberOfFactors;

private slots:
    void onNumberOfFactorsSelected( int number ){ m_numberOfFactors = number; }
};

#endif // SVDFactorsSelectionDialog_H
