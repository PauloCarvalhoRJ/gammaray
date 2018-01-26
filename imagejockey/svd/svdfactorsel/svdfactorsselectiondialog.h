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

private:
    Ui::SVDFactorsSelectionDialog *ui;
	std::vector<double> m_weights;
	SVDFactorsSelectionChartView* m_factorsSelChartView;
};

#endif // SVDFactorsSelectionDialog_H
