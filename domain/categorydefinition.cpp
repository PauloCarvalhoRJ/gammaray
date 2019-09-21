#include "categorydefinition.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparams/widgets/widgetgslibparmultivaluedfixed.h"
#include "util.h"

CategoryDefinition::CategoryDefinition(QString path) : IntIntQStringQColorQStringQuintuplets ( path )
{
}

CategoryDefinition::~CategoryDefinition()
{
    //TODO: delete the objects in m_createdParameters here.
    clearStashOfCreatedParameters();
}

QString CategoryDefinition::getCategoryNameByCode(int category_code)
{
    uint nQuintuplets = getCategoryCount();
    for( uint i = 0; i < nQuintuplets; ++i){
        if( category_code == getCategoryCode( i ) )
            return getCategoryName( i );
    }
    return "CATEGORY-NOT-FOUND";
}

uint CategoryDefinition::getCategoryColorByCode(int category_code)
{
    uint nQuintuplets = getCategoryCount();
    for( uint i = 0; i < nQuintuplets; ++i){
        if( category_code == getCategoryCode( i ) )
            return getColorCode( i );
    }
    return 0;
}

int CategoryDefinition::getCategoryIndex(int category_code)
{
    uint nQuintuplets = getCategoryCount();
    for( uint i = 0; i < nQuintuplets; ++i){
        if( category_code == getCategoryCode( i ) )
            return i;
    }
    return -1;
}

bool CategoryDefinition::codeExists(int category_code)
{
    uint nQuintuplets = getCategoryCount();
    for( uint i = 0; i < nQuintuplets; ++i){
        if( category_code == getCategoryCode( i ) )
            return true;
    }
    return false;
}

void CategoryDefinition::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
}

QWidget *CategoryDefinition::createContentElementWidget()
{
    //create a parameter set with default values to represent a quintuplet of code, color, name, custom color, extended name.
    GSLibParMultiValuedFixed* par = new GSLibParMultiValuedFixed("","","");
    int num_cats = m_stashOfCreatedParameters.size() + 1;

    GSLibParInt *pint = new GSLibParInt("","","");
    pint->_value = num_cats;
    par->_parameters.append( pint );

    GSLibParColor *pcol = new GSLibParColor("","","");
    pcol->_color_code = num_cats % 25; //max. color code is 24.
    par->_parameters.append( pcol );

    GSLibParString *pstr = new GSLibParString("","","");
    pstr->_value = QString("Category ") + QString::number( num_cats );
    par->_parameters.append( pstr );

    GSLibParCustomColor *pccol = new GSLibParCustomColor("","","");
    Util::getGSLibColor( pcol->_color_code ).getRgb( &pccol->_r,
                                                     &pccol->_g,
                                                     &pccol->_b );
    par->_parameters.append( pccol );

    GSLibParString *pstrExt = new GSLibParString("","","");
    pstrExt->_value = QString("");
    par->_parameters.append( pstrExt );

    //store the pointer to delete some time later.
    m_stashOfCreatedParameters.append( par );
    //return the widget tailored for the data types of the quintuplet.
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

    GSLibParCustomColor *pccol = new GSLibParCustomColor("","","");
    QColor customColor = get4thValue( iContent );
    customColor.getRgb( &pccol->_r,
                        &pccol->_g,
                        &pccol->_b );
    par->_parameters.append( pccol );

    GSLibParString *pstrExt = new GSLibParString("","","");
    pstrExt->_value = get5thValue( iContent ); //the extended category name
    par->_parameters.append( pstrExt );

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
    par->_parameters.append( new GSLibParCustomColor("","","") );
    par->_parameters.append( new GSLibParString("","","") );

    //read the values from the widget into the parameter set object.
    widget->updateValue( par );

    //add the triplet of the values read.
    addQuintuplet( ((GSLibParInt*)par->_parameters[0])->_value,
                   ((GSLibParColor*)par->_parameters[1])->_color_code,
                   ((GSLibParString*)par->_parameters[2])->_value,
                   QColor( ((GSLibParCustomColor*)par->_parameters[3])->_r,
                           ((GSLibParCustomColor*)par->_parameters[3])->_g,
                           ((GSLibParCustomColor*)par->_parameters[3])->_b ),
                   ((GSLibParString*)par->_parameters[4])->_value
                   );

    //store the parameter set pointer to delete some time later.
    m_stashOfCreatedParameters.append( par );
}

void CategoryDefinition::clearLoadedContents()
{
    Quintuplets::clearLoadedContents();
    clearStashOfCreatedParameters();
}

void CategoryDefinition::clearStashOfCreatedParameters()
{
    QList<GSLibParType*>::iterator it = m_stashOfCreatedParameters.begin();
    for( ; it != m_stashOfCreatedParameters.end(); ){ // erase() already increments the iterator.
        //delete *it; //TODO: this causes a crash.  investigate later.
        it = m_stashOfCreatedParameters.erase( it );
    }
}
