#ifndef GABORSCANDIALOG_H
#define GABORSCANDIALOG_H

#include <QDialog>

class IJAbstractCartesianGrid;
class IJGridViewerWidget;

namespace Ui {
class GaborScanDialog;
}

class GaborScanDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GaborScanDialog(
            IJAbstractCartesianGrid* inputGrid,
            uint inputVariableIndex,
            double meanMajorAxis,
            double meanMinorAxis,
            double sigmaMajorAxis,
            double sigmaMinorAxis,
            int kernelSizeI,
            int kernelSizeJ,
            QWidget *parent = 0);
    ~GaborScanDialog();

private:
    Ui::GaborScanDialog *ui;
    IJAbstractCartesianGrid* m_inputGrid;
    uint m_inputVariableIndex;
    IJGridViewerWidget* m_ijgv;

    double m_meanMajorAxis;
    double m_meanMinorAxis;
    double m_sigmaMajorAxis;
    double m_sigmaMinorAxis;
    int m_kernelSizeI;
    int m_kernelSizeJ;

private Q_SLOTS:
    void onScan();
};

#endif // GABORSCANDIALOG_H
