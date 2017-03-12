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

QWidget *CategoryDefinition::createWidgetFilledWithContentElement(uint iContent)
{
    //create a parameter set with the code, color and name values.
    GSLibParMultiValuedFixed* par = new GSLibParMultiValuedFixed("","","");
    GSLibParInt *pint = new GSLibParInt("","","");
    pint->_value = get1stValue( iContent ); // the category code
    par->_parameters.append( pint );
    GSLibParColor *pcol = new GSLibParColor("","","");
    pcol->_color_code = get2ndValue( iContent ); // the GSLib color code
    par->_parameters.append( pcol );
    GSLibParString *pstr = new GSLibParString("","","");
    pstr->_value = get3rdValue( iContent ); //the Category name
    par->_parameters.append( pstr );
    //store the pointer to delete some time later.
    m_stashOfCreatedParameters.append( par );
    //return the widget tailored for the data types of the triplet.
    return par->getWidget();
}

void CategoryDefinition::addContentElementFromWidget(QWidget *w)
{
    //surely the widget is a WidgetGSLibParMultiValuedFixed.
    WidgetGSLibParMultiValuedFixed *widget = (WidgetGSLibParMultiValuedFixed*)w;

    //build a parameter set adequate to read the user-entered values in the passed widget.
    GSLibParMultiValuedFixed* par = new GSLibParMultiValuedFixed("","","");
    par->_parameters.append( new GSLibParInt("","","") );
    par->_parameters.append( new GSLibParColor("","","") );
    par->_parameters.append( new GSLibParString("","","") );

    //read the values from the widget into the parameter set object.
    widget->updateValue( par );

    //add the triplet of the values read.
    addTriplet( ((GSLibParInt*)par->_parameters[0])->_value,
                ((GSLibParColor*)par->_parameters[1])->_color_code,
                ((GSLibParString*)par->_parameters[2])->_value );

    //store the parameter set pointer to delete some time later.
    m_stashOfCreatedParameters.append( par );
}
