#ifndef WAVELETTRANSFORMDIALOG_H
#define WAVELETTRANSFORMDIALOG_H

#include "imagejockey/wavelet/waveletutils.h"
#include "spectral/spectral.h"

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

Q_SIGNALS:
    void saveDWTTransform( const QString name, const spectral::array& DWTtransform );

private:
    Ui::WaveletTransformDialog *ui;
    IJAbstractCartesianGrid* m_inputGrid;
    uint m_inputVariableIndex;
    spectral::array m_DWTbuffer;
    WaveletFamily getSelectedWaveletFamily();
    static void debugGrid( const spectral::array &grid );

private Q_SLOTS:
    void onPerformTransform();
    void onWaveletFamilySelected( QString waveletFamilyName );
    void onSaveDWTResultAsGrid();
};

#endif // WAVELETTRANSFORMDIALOG_H
