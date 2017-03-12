#include "widgetgslibparcolor.h"
#include "ui_widgetgslibparcolor.h"
#include "../gslibparcolor.h"
#include <QColor>
#include "util.h"

WidgetGSLibParColor::WidgetGSLibParColor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParColor),
    _color()
{
    ui->setupUi(this);

    //make list of GSLib color labels
    QStringList labels;
    labels << "red (1)" << "orange (2)" << "yellow (3)" << "light green (4)" << "green (5)";
    labels << "light blue (6)" << "dark blue (7)" << "violet (8)" << "white (9)" << "black (10)";
    labels << "purple (11)" << "brown (12)" << "pink (13)" << "intermediate green (14)";
    labels << "gray (15)" << "gray 10% (16)" << "gray 20% (17)" << "gray 30% (18)" << "gray 40% (19)";
    labels << "gray 50% (20)" << "gray 60% (21)" << "gray 70% (22)" << "gray 80% (23)" << "gray 90% (24)";

    //make list of GSLib colors
    QList<QColor> colors;
    Util::makeGSLibColorsList( colors );

    //make the drop down menu items using the two lists above
    QComboBox *cb = ui->cmbColor;
    for( int index = 0; index < labels.size(); ++index){
        QColor color( Qt::red );
        cb->insertItem(index, labels.at( index ), QVariant( color ));
        cb->setItemData(index, colors.at( index ), Qt::UserRole);
        cb->setItemIcon(index, Util::makeGSLibColorIcon( index + 1 ));
    }

    //notify this object whenever the user makes a choice
    connect( cb, SIGNAL(currentIndexChanged(int)), this, SLOT(updateColor(int)) );
}

WidgetGSLibParColor::~WidgetGSLibParColor()
{
    delete ui;
}

void WidgetGSLibParColor::fillFields(GSLibParColor *param)
{
    this->fillFields( param, param->getDescription() );
}

void WidgetGSLibParColor::fillFields(GSLibParColor *param, const QString label)
{
    ui->lblDescription->setText( label );
    ui->cmbColor->setCurrentIndex( param->_color_code - 1 );
}

void WidgetGSLibParColor::updateValue(GSLibParColor *param)
{
    param->_color_code = ui->cmbColor->currentIndex() + 1;
}

void WidgetGSLibParColor::updateColor(int item_index)
{
    QComboBox *cb = ui->cmbColor;
    QVariant item = cb->itemData( item_index, Qt::UserRole );
    _color = qvariant_cast<QColor>(item);
    //QColor color = qvariant_cast<QColor>(item);
    /*QString css = "QComboBox { background-color: rgb(";
    css = css.append(QString::number(color.red()));
    css = css.append(", ");
    css = css.append(QString::number(color.green()));
    css = css.append(", ");
    css = css.append(QString::number(color.blue()));
    css = css.append("); }");
    cb->setStyleSheet(css);*/
}
