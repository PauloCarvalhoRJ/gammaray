#ifndef VERTICALPROPORTIONCURVEDIALOG_H
#define VERTICALPROPORTIONCURVEDIALOG_H

#include <QDialog>

#include "domain/verticalproportioncurve.h"

namespace Ui {
class VerticalProportionCurveDialog;
}

class FileSelectorWidget;
class Attribute;
class VariableSelector;
class VerticalProportionCurvesPlot;
class File;
class CategoryPDF;

class VerticalProportionCurveDialog : public QDialog
{
    Q_OBJECT

    /*! A flag that sets the origin of the drag-and-drop gesture managed by this dialog. */
    enum class DragOrigin : int {
        FROM_LIST_OF_VARIABLES, /*! If drag originated in the dialog's own list of variables. */
        FROM_ELSEWHERE /*! If darg started elsewhere, including from outside GammaRay. */
    };

public:
    /** Pass a pointer to an existing vertical proportion curve object to display and edit it. */
    explicit VerticalProportionCurveDialog( VerticalProportionCurve* vpc = nullptr,
                                            QWidget *parent = nullptr);
    ~VerticalProportionCurveDialog();

    /**
     * @name Methods to support drag-and-drop operations.
     */
    //@{
    void dragEnterEvent(QDragEnterEvent *e);
    void dragMoveEvent(QDragMoveEvent *e);
    void dropEvent(QDropEvent *e);
    //@}

private Q_SLOTS:
    void onRun();
    void onSave();
    void onFallbackPDFChanged( File* pdf );
    void onEditCurves();

private:
    void tryToAddAttribute(Attribute *attribute);

    void tryToRemoveAttribute(Attribute *attribute);

    void updateVariablesList();

    void updateCurvesOfPlot();

    void saveNew();

    void saveExisting();

    /** Rereads data from m_currentVPC object and updates the plot accordingly.
     * Does nothing if m_currentVPC is a null pointer.
     */
    void updatePlotWithCurrentVPC();

    VerticalProportionCurve computeProportionsForASegmentSet( Attribute* at );

    Ui::VerticalProportionCurveDialog *ui;

    FileSelectorWidget* m_cmbFallBackPDF;
    FileSelectorWidget* m_cmbTopHorizon;
    FileSelectorWidget* m_cmbBaseHorizon;
    VariableSelector* m_cmbTopVariable;
    VariableSelector* m_cmbBaseVariable;

    VerticalProportionCurvesPlot* m_VPCPlot;

    CategoryPDF* m_fallbackPDF;

    std::vector<Attribute*> m_categoricalAttributes;

    DragOrigin m_dragOrigin;

    VerticalProportionCurve* m_currentVPC;
};

#endif // VERTICALPROPORTIONCURVEDIALOG_H
