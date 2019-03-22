#ifndef DYNAMICFACIESRELATIONSHIPDIAGRAMDIALOG_H
#define DYNAMICFACIESRELATIONSHIPDIAGRAMDIALOG_H

#include <QDialog>

#include "util.h"

namespace Ui {
class DynamicFaciesRelationshipDiagramDialog;
}

class Attribute;

class DynamicFaciesRelationshipDiagramDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DynamicFaciesRelationshipDiagramDialog( std::vector<Attribute*>& categoricalAttributes,
                                                     double hInitial,
                                                     double hFinal,
                                                     int nSteps,
                                                     double toleranceCoefficient,
                                                     QWidget *parent = nullptr );
    ~DynamicFaciesRelationshipDiagramDialog();

private:
    Ui::DynamicFaciesRelationshipDiagramDialog *ui;

    std::vector<Attribute*>& m_categoricalAttributes;

    double m_hInitial;
    double m_hFinal;
    int    m_nSteps;
    double m_toleranceCoefficient;

    std::vector<hFTM> m_hFTMs;

    std::vector<QPixmap> m_images;

private Q_SLOTS:
    void onDisplayDiagram( int index );
    void onGenerateDiagrams();
    void onCapture();
    void onSave();
};

#endif // DYNAMICFACIESRELATIONSHIPDIAGRAMDIALOG_H
