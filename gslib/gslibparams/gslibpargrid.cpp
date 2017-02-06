#include "gslibpargrid.h"
#include "gslibparmultivaluedfixed.h"
#include "gslibparuint.h"
#include "gslibpardouble.h"
#include "../../domain/cartesiangrid.h"
#include "widgets/gslibparamwidgets.h"

GSLibParGrid::GSLibParGrid(const QString name, const QString label, const QString description) :
    GSLibParType(name, label, description)
{
    _specs_x = new GSLibParMultiValuedFixed( "", "", "nx,xmn,xsiz" );
    _specs_y = new GSLibParMultiValuedFixed( "", "", "ny,ymn,ysiz" );
    _specs_z = new GSLibParMultiValuedFixed( "", "", "nz,zmn,zsiz" );

    _specs_x->_parameters.append( new GSLibParUInt("", "", "nx") );
    _specs_x->_parameters.append( new GSLibParDouble("", "", "xmn") );
    _specs_x->_parameters.append( new GSLibParDouble("", "", "xsiz") );

    _specs_y->_parameters.append( new GSLibParUInt("", "", "ny") );
    _specs_y->_parameters.append( new GSLibParDouble("", "", "ymn") );
    _specs_y->_parameters.append( new GSLibParDouble("", "", "ysiz") );

    _specs_z->_parameters.append( new GSLibParUInt("", "", "nz") );
    _specs_z->_parameters.append( new GSLibParDouble("", "", "zmn") );
    _specs_z->_parameters.append( new GSLibParDouble("", "", "zsiz") );
}

GSLibParGrid::~GSLibParGrid()
{
    delete _specs_x;
    delete _specs_y;
    delete _specs_z;
}

void GSLibParGrid::setFromCG(CartesianGrid *cg)
{
    _specs_x->getParameter<GSLibParUInt*>( 0 )->_value = cg->getNX();
    _specs_x->getParameter<GSLibParDouble*>( 1 )->_value = cg->getX0();
    _specs_x->getParameter<GSLibParDouble*>( 2 )->_value = cg->getDX();
    _specs_y->getParameter<GSLibParUInt*>( 0 )->_value = cg->getNY();
    _specs_y->getParameter<GSLibParDouble*>( 1 )->_value = cg->getY0();
    _specs_y->getParameter<GSLibParDouble*>( 2 )->_value = cg->getDY();
    _specs_z->getParameter<GSLibParUInt*>( 0 )->_value = cg->getNZ();
    _specs_z->getParameter<GSLibParDouble*>( 1 )->_value = cg->getZ0();
    _specs_z->getParameter<GSLibParDouble*>( 2 )->_value = cg->getDZ();
}

void GSLibParGrid::save(QTextStream *out)
{
    _specs_x->save( out );
    _specs_y->save( out );
    _specs_z->save( out );
}

QWidget *GSLibParGrid::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibParGrid();
    ((WidgetGSLibParGrid*)this->_widget)->fillFields( this );
    return this->_widget;
}

bool GSLibParGrid::update()
{
    ((WidgetGSLibParGrid*)this->_widget)->updateValue( this );
    return true;
}
