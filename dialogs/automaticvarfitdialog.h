#ifndef AUTOMATICVARFITDIALOG_H
#define AUTOMATICVARFITDIALOG_H

#include <QDialog>
#include "geostats/nestedvariogramstructuresparameters.h"
#include "geostats/automaticvariogramfitting.h"

class Attribute;
class CartesianGrid;
class IJGridViewerWidget;

namespace Ui {
class AutomaticVarFitDialog;
}

class AutomaticVarFitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AutomaticVarFitDialog( Attribute* at, QWidget *parent = nullptr);
    ~AutomaticVarFitDialog();

private:
    Ui::AutomaticVarFitDialog *ui;
    Attribute* m_at;
    CartesianGrid* m_cg;

    IJGridViewerWidget* m_gridViewerInput;
    IJGridViewerWidget* m_gridViewerVarmap;

    NestedVariogramStructuresParametersPtr m_nestedVariogramStructuresParametersForManual;

    AutomaticVariogramFitting m_autoVarFit;

private Q_SLOTS:

    void onDoWithSAandGD();
    void onDoWithLSRS();
    void onDoWithPSO();
    void onDoWithGenetic();
    void onDoWithManual();

    void onVarmapMethodChanged();
    void onSaveAResult( spectral::array* result );
    void onNumberOfStructuresChanged(int number);
};

#endif // AUTOMATICVARFITDIALOG_H
