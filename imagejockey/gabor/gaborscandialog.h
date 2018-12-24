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
            QWidget *parent = 0);
    ~GaborScanDialog();

private:
    Ui::GaborScanDialog *ui;
    IJAbstractCartesianGrid* m_inputGrid;
    uint m_inputVariableIndex;
    IJGridViewerWidget* m_ijgv;

private Q_SLOTS:
    void onScan();
};

#endif // GABORSCANDIALOG_H
