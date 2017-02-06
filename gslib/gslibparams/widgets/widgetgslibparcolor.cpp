#include "widgetgslibparcolor.h"
#include "ui_widgetgslibparcolor.h"
#include "../gslibparcolor.h"
#include <QColor>

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
    colors << Qt::red << QColor(255,165,0) << Qt::yellow << Qt::green << QColor( Qt::green ).darker();
    colors << Qt::cyan << Qt::blue <<  QColor(238,130,238) << Qt::white << Qt::black;
    colors << QColor(128,0,128) << QColor(165,42,42) << QColor(255,20,147) << QColor(50,205,50);
    colors << Qt::gray << QColor(26,26,26) << QColor(51,51,51) << QColor(77,77,77) << QColor(102,102,102);
    colors << QColor(128,128,128) << QColor(154,154,154) << QColor(179,179,179) << QColor(205,205,205) << QColor(230,230,230);

    //make the drop down menu items using the two lists above
    QComboBox *cb = ui->cmbColor;
    for( int index = 0; index < labels.size(); ++index){
        QColor color( Qt::red );
        QPixmap pixmap(16,16);
        pixmap.fill( colors.at( index ) );
        cb->insertItem(index, labels.at( index ), QVariant( color ));
        cb->setItemData(index, colors.at( index ), Qt::UserRole);
        cb->setItemIcon(index, QIcon( pixmap ));
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
