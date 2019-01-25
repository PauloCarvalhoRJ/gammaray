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
    /**
     * This unusual signal is triggered when this dialog wants some grid with the given name.
     * If there is a grid by the given name, its pointer is set to the passed pointer
     * reference, otherwise, the context must set it to nullptr.
     */
    void requestGrid( const QString name, IJAbstractCartesianGrid*& pointer );

private:
    Ui::WaveletTransformDialog *ui;
    IJAbstractCartesianGrid* m_inputGrid;
    uint m_inputVariableIndex;
    spectral::array m_DWTbuffer;
    IJAbstractCartesianGrid* m_pointerToRequestedGrid; //used with the unusual requestGrid() signal
    WaveletFamily getSelectedWaveletFamily();
    static void debugGrid( const spectral::array &grid );

private Q_SLOTS:
    void onPerformTransform();
    void onWaveletFamilySelected( QString waveletFamilyName );
    void onSaveDWTResultAsGrid();
    void onReadDWTResultFromGrid();
};

#endif // WAVELETTRANSFORMDIALOG_H
