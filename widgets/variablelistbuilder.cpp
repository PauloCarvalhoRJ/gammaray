#include "variablelistbuilder.h"
#include "ui_variablelistbuilder.h"

#include "util.h"
#include "domain/datafile.h"
#include "domain/attribute.h"
#include "dialogs/listbuilderdialog.h"
#include "widgets/listbuilder.h"

VariableListBuilder::VariableListBuilder(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VariableListBuilder),
    m_dataFile( nullptr )
{
    ui->setupUi(this);
}

VariableListBuilder::~VariableListBuilder()
{
    delete ui;
}

void VariableListBuilder::setCaption(QString caption)
{
    ui->lblCaption->setText( caption );
}

void VariableListBuilder::setCaptionBGColor(const QColor &color)
{
    QColor foreGroudColor = Util::makeContrast( color );

    QString fgRGBAValues = QString::number(foreGroudColor.red())   + "," +
                           QString::number(foreGroudColor.green()) + "," +
                           QString::number(foreGroudColor.blue())  + ",255";

    QString bgRGBAValues = QString::number(color.red())   + "," +
                           QString::number(color.green()) + "," +
                           QString::number(color.blue())  + ",255";

    ui->lblCaption->setStyleSheet("QLabel { background-color : rgba(" + bgRGBAValues + "); color : rgba(" + fgRGBAValues + "); }");
}

void VariableListBuilder::onOpenBuildList()
{
    if( m_dataFile ){
        ListBuilderDialog lbd( this );
        lbd.setWindowTitle("Build list of " + m_dataFile->getName() + " attributes.");
        lbd.getListBuilder()->onInitListWithVariables( m_dataFile );
        lbd.getListBuilder()->preSelectAttributes( m_attributeList );
        lbd.exec();

        if( lbd.result() == QDialog::Accepted ){
            QString label_text;
            m_attributeList = lbd.getListBuilder()->getSelectedAttributes();
            for( Attribute* at : m_attributeList )
                label_text.append( at->getName() + "; " );
            if( label_text.size() > 100 )
                label_text = label_text.left(97) + "...";
            ui->lblList->setText( label_text );
        }
    }
}

void VariableListBuilder::onListVariables(DataFile *file)
{
    m_dataFile = file;
}
