#include "categorydefinition.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparams/widgets/widgetgslibparmultivaluedfixed.h"

CategoryDefinition::CategoryDefinition(QString path) : IntIntQStringTriplets ( path )
{
}

CategoryDefinition::~CategoryDefinition()
{
    //TODO: delete the objects in m_createdParameters here.
}

void CategoryDefinition::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
}

QWidget *CategoryDefinition::createContentElementWidget()
{
    //create a parameter set with default values to represent a triplet of code, color, name.
    GSLibParMultiValuedFixed* par = new GSLibParMultiValuedFixed("","","");
    GSLibParInt *pint = new GSLibParInt("","","");
    int num_cats = m_stashOfCreatedParameters.size() + 1;
    pint->_value = num_cats;
    par->_parameters.append( pint );
    GSLibParColor *pcol = new GSLibParColor("","","");
    pcol->_color_code = num_cats % 25; //max. color code is 24.
    par->_parameters.append( pcol );
    GSLibParString *pstr = new GSLibParString("","","");
    pstr->_value = QString("Category ") + QString::number( num_cats );
    par->_parameters.append( pstr );
    //store the pointer to delete some time later.
    m_stashOfCreatedParameters.append( par );
    //return the widget tailored for the data types of the triplet.
    return par->getWidget();
}

void CategoryDefinition::addContentElementFromWidget(QWidget *w)
{
    //surely the widget is a WidgetGSLibParMultiValuedFixed
    WidgetGSLibParMultiValuedFixed *widget = (WidgetGSLibParMultiValuedFixed*)w;
    //widget->updateValue();
}
