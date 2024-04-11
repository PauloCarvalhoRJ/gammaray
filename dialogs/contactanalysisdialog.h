#ifndef CONTACTANALYSISDIALOG_H
#define CONTACTANALYSISDIALOG_H

#include <QDialog>

namespace Ui {
class ContactAnalysisDialog;
}

class Attribute;
class DataFile;
class CategorySelector;

class ContactAnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ContactAnalysisDialog(Attribute* attributeGrade,
                                   Attribute* attributeDomains,
                                   QWidget *parent = nullptr);
    ~ContactAnalysisDialog();

private:
    Ui::ContactAnalysisDialog *ui;

private Q_SLOTS:
    void onLengthUnitSymbolChanged( QString lengthUnitSymbol );
    void onProceed();

private:
    Attribute* m_attributeGrade;
    Attribute* m_attributeDomains;
    DataFile*  m_dataFile;
    CategorySelector* m_selectorDomain1;
    CategorySelector* m_selectorDomain2;
};

#endif // CONTACTANALYSISDIALOG_H
