#include "calculatordialog.h"
#include "ui_calculatordialog.h"

#include "icalcpropertycollection.h"
#include "icalcproperty.h"

#include <QMessageBox>

CalculatorDialog::CalculatorDialog(ICalcPropertyCollection* propertyCollection, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalculatorDialog),
    m_propertyCollection( propertyCollection )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle( "Property calculator" );

    //display the name of the property collection
    this->ui->lblPropColName->setText( m_propertyCollection->getCalcPropertyCollectionName() );

    //populate the list with the properties
    this->ui->lstProperties->clear();
    for( int i = 0; i < m_propertyCollection->getCalcPropertyCount(); ++i ){
        ICalcProperty* prop = m_propertyCollection->getCalcProperty( i );
        this->ui->lstProperties->addItem(new QListWidgetItem(prop->getCalcPropertyIcon(),
                                                             prop->getCalcPropertyName()));
    }

    //clear the script text area.
    this->ui->txtScript->clear();

    connect(ui->lstProperties, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(onPropertyDoubleClicked(QListWidgetItem*)));
}

CalculatorDialog::~CalculatorDialog()
{
    delete ui;
}

void CalculatorDialog::onPropertyDoubleClicked(QListWidgetItem * item)
{
    if( item->text().contains(' ') ){
        QMessageBox::critical( this, "Error", QString("Cannot bind variables with whitespaces in name."));
        return;
    }
    QTextCursor cursor = ui->txtScript->textCursor();
    cursor.insertText( item->text() );
}
