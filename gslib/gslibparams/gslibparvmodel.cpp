#include "gslibparvmodel.h"
#include "gslibparuint.h"
#include "gslibpardouble.h"
#include "gslibparmultivaluedfixed.h"
#include "gslibparrepeat.h"
#include "gslibparoption.h"
#include "widgets/widgetgslibparvmodel.h"
#include "domain/variogrammodel.h"

GSLibParVModel::GSLibParVModel(const QString name, const QString label, const QString description) :
    GSLibParType(name, label, description)
{
    _nst_and_nugget = new GSLibParMultiValuedFixed( "", "", "nst, nugget effect" );
    _nst_and_nugget->_parameters.append( new GSLibParUInt("nst", "", "nst") );
    _nst_and_nugget->_parameters.append( new GSLibParDouble("", "", "nugget effect") );

    _variogram_structures = new GSLibParRepeat();
    _variogram_structures->_original_parameters.append( new GSLibParMultiValuedFixed("", "", "it,cc,ang1,ang2,ang3") );
    GSLibParMultiValuedFixed* par0 = (GSLibParMultiValuedFixed*)_variogram_structures->_original_parameters[0];
    GSLibParOption *opt = new GSLibParOption("", "", "it");
        opt->addOption( 1, "spheric" );
        opt->addOption( 2, "exponential" );
        opt->addOption( 3, "gaussian" );
        opt->addOption( 4, "power law" );
        opt->addOption( 5, "cosine hole effect" );
    par0->_parameters.append( opt );
    par0->_parameters.append( new GSLibParDouble("", "", "cc") );
    par0->_parameters.append( new GSLibParDouble("", "", "ang1") );
    par0->_parameters.append( new GSLibParDouble("", "", "ang2") );
    par0->_parameters.append( new GSLibParDouble("", "", "ang3") );
    _variogram_structures->_original_parameters.append( new GSLibParMultiValuedFixed("", "", "a_hmax,a_hmin,a_vert") );
    GSLibParMultiValuedFixed* par1 = (GSLibParMultiValuedFixed*)_variogram_structures->_original_parameters[1];
    par1->_parameters.append( new GSLibParDouble("", "", "a_hmax") );
    par1->_parameters.append( new GSLibParDouble("", "", "a_hmin") );
    par1->_parameters.append( new GSLibParDouble("", "", "a_vert") );
}

GSLibParVModel::~GSLibParVModel()
{
    delete _nst_and_nugget;
    delete _variogram_structures;
}

void GSLibParVModel::makeDefault()
{
    _nst_and_nugget->getParameter<GSLibParUInt*>(0)->_value = 1; //number of structures
    _nst_and_nugget->getParameter<GSLibParDouble*>(1)->_value = 0.0; //nugget effect
    _variogram_structures->setCount( 1 ); //number of structures
    GSLibParMultiValuedFixed* par0 = (GSLibParMultiValuedFixed*)_variogram_structures->_original_parameters[0];
    par0->getParameter<GSLibParOption*>(0)->_selected_value = 1; //structure type
    par0->getParameter<GSLibParDouble*>(1)->_value = 1.0; //covariance contribution
    par0->getParameter<GSLibParDouble*>(2)->_value = 0.0; //azimuth
    par0->getParameter<GSLibParDouble*>(3)->_value = 0.0; //dip
    par0->getParameter<GSLibParDouble*>(4)->_value = 0.0; //roll
    GSLibParMultiValuedFixed* par1 = (GSLibParMultiValuedFixed*)_variogram_structures->_original_parameters[1];
    par1->getParameter<GSLibParDouble*>(0)->_value = 1.0; //range along azimuth
    par1->getParameter<GSLibParDouble*>(1)->_value = 1.0; //range orthogonal to azimuth
    par1->getParameter<GSLibParDouble*>(2)->_value = 1.0; //range along vertical
}

void GSLibParVModel::setFromVariogramModel(VariogramModel *vmodel)
{
    uint nst = vmodel->getNst();
    _nst_and_nugget->getParameter<GSLibParUInt*>(0)->_value = nst; //number of variogram structures
    _nst_and_nugget->getParameter<GSLibParDouble*>(1)->_value = vmodel->getNugget(); //nugget effect
    _variogram_structures->setCount( nst ); //number of variogram structures
    for(uint i = 0; i < nst; ++i){ //for each variogram structure
        GSLibParMultiValuedFixed* par0 = _variogram_structures->getParameter<GSLibParMultiValuedFixed*>(i, 0);
        par0->getParameter<GSLibParOption*>(0)->_selected_value = (uint)vmodel->getIt( i ); //structure type
        par0->getParameter<GSLibParDouble*>(1)->_value = vmodel->getCC( i ); //covariance contribution
        par0->getParameter<GSLibParDouble*>(2)->_value = vmodel->getAzimuth( i ); //azimuth
        par0->getParameter<GSLibParDouble*>(3)->_value = vmodel->getDip( i ); //dip
        par0->getParameter<GSLibParDouble*>(4)->_value = vmodel->getRoll( i ); //roll
        GSLibParMultiValuedFixed* par1 = _variogram_structures->getParameter<GSLibParMultiValuedFixed*>(i, 1);
        par1->getParameter<GSLibParDouble*>(0)->_value = vmodel->get_a_hMax( i ); //range along azimuth
        par1->getParameter<GSLibParDouble*>(1)->_value = vmodel->get_a_hMin( i ); //range orthogonal to azimuth
        par1->getParameter<GSLibParDouble*>(2)->_value = vmodel->get_a_vert( i ); //range along vertical
    }
}

void GSLibParVModel::makeNull()
{
    _nst_and_nugget->getParameter<GSLibParUInt*>(0)->_value = 1; //number of structures
    _nst_and_nugget->getParameter<GSLibParDouble*>(1)->_value = 0.0; //nugget effect
    _variogram_structures->setCount( 1 ); //number of structures
    GSLibParMultiValuedFixed* par0 = (GSLibParMultiValuedFixed*)_variogram_structures->_original_parameters[0];
    par0->getParameter<GSLibParOption*>(0)->_selected_value = 1; //structure type
    par0->getParameter<GSLibParDouble*>(1)->_value = 0.0; //covariance contribution
    par0->getParameter<GSLibParDouble*>(2)->_value = 0.0; //azimuth
    par0->getParameter<GSLibParDouble*>(3)->_value = 0.0; //dip
    par0->getParameter<GSLibParDouble*>(4)->_value = 0.0; //roll
    GSLibParMultiValuedFixed* par1 = (GSLibParMultiValuedFixed*)_variogram_structures->_original_parameters[1];
    par1->getParameter<GSLibParDouble*>(0)->_value = 1.0; //range along azimuth
    par1->getParameter<GSLibParDouble*>(1)->_value = 1.0; //range orthogonal to azimuth
    par1->getParameter<GSLibParDouble*>(2)->_value = 1.0; //range along vertical
}

void GSLibParVModel::save(QTextStream *out)
{
    _nst_and_nugget->save( out );
    _variogram_structures->save( out );
}

QWidget *GSLibParVModel::getWidget()
{
    if( ! this->_widget )
       this->_widget = new WidgetGSLibParVModel();
    ((WidgetGSLibParVModel*)this->_widget)->fillFields( this );
    return this->_widget;
}

GSLibParVModel *GSLibParVModel::clone()
{
    //delete the parameter objects instantiated by default.

    GSLibParVModel* new_par = new GSLibParVModel("", "", _description);

    delete new_par->_nst_and_nugget; //TODO: not good practice (delete members of other objects)
    new_par->_nst_and_nugget = _nst_and_nugget->clone();

    delete new_par->_variogram_structures;
    new_par->_variogram_structures = _variogram_structures->clone();

    return new_par;
}

bool GSLibParVModel::update()
{
    ((WidgetGSLibParVModel*)this->_widget)->updateValue( this );
    return true;
}
