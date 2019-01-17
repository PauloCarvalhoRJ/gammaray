#ifndef WAVELETTRANSFORMDIALOG_H
#define WAVELETTRANSFORMDIALOG_H

#include <QDialog>

namespace Ui {
class WaveletTransformDialog;
}

class IJAbstractCartesianGrid;

class WaveletTransformDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WaveletTransformDialog(IJAbstractCartesianGrid* inputGrid,
                                    uint inputVariableIndex,
                                    QWidget *parent = 0);
    ~WaveletTransformDialog();

private:
    Ui::WaveletTransformDialog *ui;
    IJAbstractCartesianGrid* m_inputGrid;
    uint m_inputVariableIndex;

private Q_SLOTS:
    void onPerformTransform();
};

#endif // WAVELETTRANSFORMDIALOG_H
