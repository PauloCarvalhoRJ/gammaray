#ifndef VERTICALPROPORTIONCURVEDIALOG_H
#define VERTICALPROPORTIONCURVEDIALOG_H

#include <QDialog>

namespace Ui {
class VerticalProportionCurveDialog;
}

class FileSelectorWidget;
class Attribute;

class VerticalProportionCurveDialog : public QDialog
{
    Q_OBJECT

    /*! A flag that sets the origin of the drag-and-drop gesture managed by this dialog. */
    enum class DragOrigin : int {
        FROM_LIST_OF_VARIABLES, /*! If drag originated in the dialog's own list of variables. */
        FROM_ELSEWHERE /*! If darg started elsewhere, including from outside GammaRay. */
    };

public:
    explicit VerticalProportionCurveDialog(QWidget *parent = nullptr);
    ~VerticalProportionCurveDialog();

    /**
     * @name Methods to support drag-and-drop operations.
     */
    //@{
    void dragEnterEvent(QDragEnterEvent *e);
    void dragMoveEvent(QDragMoveEvent *e);
    void dropEvent(QDropEvent *e);
    //@}

private:
    void tryToAddAttribute(Attribute *attribute);

    void tryToRemoveAttribute(Attribute *attribute);

    void updateVariablesList();

    Ui::VerticalProportionCurveDialog *ui;

    FileSelectorWidget* m_cmbFallBackPDF;
    FileSelectorWidget* m_cmbTopHorizon;
    FileSelectorWidget* m_cmbBaseHorizon;

    std::vector<Attribute*> m_categoricalAttributes;

    DragOrigin m_dragOrigin;
};

#endif // VERTICALPROPORTIONCURVEDIALOG_H
