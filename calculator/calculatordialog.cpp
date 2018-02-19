#include "calculatordialog.h"
#include "ui_calculatordialog.h"

#include "icalcpropertycollection.h"
#include "icalcproperty.h"
#include "calcscripting.h"

#include <QDesktopServices>
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
															 prop->getScriptCompatibleName()));
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
    QTextCursor cursor = ui->txtScript->textCursor();
    cursor.insertText( item->text() );
    ui->txtScript->setFocus();
}

void CalculatorDialog::onSyntaxPage()
{
    if( ! QDesktopServices::openUrl(QUrl( "http://www.partow.net/programming/exprtk/index.html" )) )
		QMessageBox::critical( this, "Error", QString("Failed to open http://www.partow.net/programming/exprtk/index.html."));
}

void CalculatorDialog::onRun()
{
	m_propertyCollection->computationWillStart();
	CalcScripting cs( m_propertyCollection );
	if( cs.doCalc( ui->txtScript->toPlainText() ) )
		m_propertyCollection->computationCompleted();
	else
		QMessageBox::critical( this, "Script error", cs.getLastError() );
}
