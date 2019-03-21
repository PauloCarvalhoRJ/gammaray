#ifndef DYNAMICFACIESRELATIONSHIPDIAGRAMDIALOG_H
#define DYNAMICFACIESRELATIONSHIPDIAGRAMDIALOG_H

#include <QDialog>

namespace Ui {
class DynamicFaciesRelationshipDiagramDialog;
}

class DynamicFaciesRelationshipDiagramDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DynamicFaciesRelationshipDiagramDialog(QWidget *parent = nullptr);
    ~DynamicFaciesRelationshipDiagramDialog();

private:
    Ui::DynamicFaciesRelationshipDiagramDialog *ui;
};

#endif // DYNAMICFACIESRELATIONSHIPDIAGRAMDIALOG_H
