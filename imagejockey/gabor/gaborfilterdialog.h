#ifndef GABORFILTERDIALOG_H
#define GABORFILTERDIALOG_H

#include <QDialog>
#include "spectral/spectral.h"

namespace Ui {
class GaborFilterDialog;
}

class IJAbstractCartesianGrid;

class GaborFilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GaborFilterDialog(IJAbstractCartesianGrid* inputGrid,
                               uint inputVariableIndex,
                               QWidget *parent = nullptr);
    ~GaborFilterDialog();

    /** Returns the spectrogram cube generated in the last computation.
     * It returns an empty pointer if the user didn't perform any calculation.
     */
    spectral::arrayPtr getSpectrogram();

Q_SIGNALS:
    /** Client code can use this to be notified when the spectrogram is read.
     * Retrieve results with getSpectrogram().
     */
    void spectrogramGenerated();

private:
    Ui::GaborFilterDialog *ui;
    IJAbstractCartesianGrid* m_inputGrid;
    uint m_inputVariableIndex;
    spectral::arrayPtr m_spectrogram;

private Q_SLOTS:
    void onPerformGaborFilter();
};

#endif // GABORFILTERDIALOG_H
