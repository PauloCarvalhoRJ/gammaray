#ifndef WAVELETTRANSFORMDIALOG_H
#define WAVELETTRANSFORMDIALOG_H

#include "imagejockey/wavelet/waveletutils.h"

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
    WaveletFamily getSelectedWaveletFamily();

private Q_SLOTS:
    void onPerformTransform();
    void onWaveletFamilySelected( QString waveletFamilyName );
};

#endif // WAVELETTRANSFORMDIALOG_H
