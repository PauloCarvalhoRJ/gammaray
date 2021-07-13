#include "variogrammodel.h"

#include <QFileInfo>
#include <QTextStream>

#include "domain/application.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "util.h"

VariogramModel::VariogramModel(const QString path) : File( path ),
    _forceReread (true)
{
}

VariogramModel::VariogramModel() : File( "NOFILE" ),
	_forceReread(false)
{

}

double VariogramModel::getSill()
{
    if(_forceReread) readParameters();
    return m_Sill;
}

double VariogramModel::getNugget()
{
    if(_forceReread) readParameters();
    return m_Nugget;
}

uint VariogramModel::getNst()
{
    if(_forceReread) readParameters();
    return m_nst;
}

VariogramStructureType VariogramModel::getIt(int structure)
{
    if(_forceReread) readParameters();
    return m_it.at( structure );
}

double VariogramModel::getCC(int structure)
{
    if(_forceReread) readParameters();
    return m_cc.at( structure );
}

double VariogramModel::get_a_hMax(int structure)
{
    if(_forceReread) readParameters();
    return m_a_hMax.at( structure );
}

double VariogramModel::get_a_hMin( int structure )
{
    if(_forceReread) readParameters();
    return m_a_hMin.at( structure );
}

double VariogramModel::get_a_vert(int structure)
{
    if(_forceReread) readParameters();
    return m_a_vert.at( structure );
}

double VariogramModel::getAzimuth(int structure)
{
    if(_forceReread) readParameters();
    return m_Azimuth.at( structure );
}

double VariogramModel::getDip(int structure)
{
    if(_forceReread) readParameters();
    return m_Dip.at( structure );
}

double VariogramModel::getRoll(int structure)
{
    if(_forceReread) readParameters();
    return m_Roll.at( structure );
}

double VariogramModel::get_max_hMax()
{
    double value = get_a_hMax( 0 );
    for( uint i = 1; i < this->getNst(); ++i ){
        double tmp = get_a_hMax( i );
        if( tmp > value )
            value = tmp;
    }
    return value;
}

double VariogramModel::get_max_hMin()
{
    double value = get_a_hMin( 0 );
    for( uint i = 1; i < this->getNst(); ++i ){
        double tmp = get_a_hMin( i );
        if( tmp > value )
            value = tmp;
    }
    return value;
}

double VariogramModel::get_max_vert()
{
    double value = get_a_vert( 0 );
    for( uint i = 1; i < this->getNst(); ++i ){
        double tmp = get_a_vert( i );
        if( tmp > value )
            value = tmp;
    }
	return value;
}

VariogramModel VariogramModel::makeVModelFromSingleStructure( int structure )
{
	VariogramModel result;
	if( structure == 0 ){
		result.m_Sill = m_Nugget; //sill of a nugget variogram.
		result.m_nst = 0; //number of structures of a nugget variogram is always zero.
		result.m_Nugget = m_Nugget;
	}else{
		int ist = structure-1;
		result.m_Sill = m_cc[ist]; //sill of a single-structure variogram equals the contribution of its single structure.
		result.m_Nugget = 0.0; //nugget of a single-structure variogram is zero.
		result.m_nst = 1; //number of structures is always one.
		result.m_it.push_back( m_it[ist] );
		result.m_cc.push_back( m_cc[ist] );
		result.m_a_hMax.push_back( m_a_hMax[ist] );
		result.m_a_hMin.push_back( m_a_hMin[ist] );
		result.m_a_vert.push_back( m_a_vert[ist] );
		result.m_Azimuth.push_back( m_Azimuth[ist] );
		result.m_Dip.push_back( m_Dip[ist] );
		result.m_Roll.push_back( m_Roll[ist] );
	}
	return result;
}

uint VariogramModel::getNstWithNugget()
{
    return getNst()+1;
}

QString VariogramModel::getStructureDescription(int structure)
{
    QString desc = Util::getGSLibVariogramStructureName( static_cast<int>(m_it[structure]) ) + " ( ";
    desc += QString::number( m_a_hMax[structure] ) + " x ";
    desc += QString::number( m_a_hMin[structure] ) + " x ";
    desc += QString::number( m_a_vert[structure] );
    desc += " )";
    return desc;
}

bool VariogramModel::isPureNugget()
{
    return getNst() == 0;
}


QIcon VariogramModel::getIcon()
{
    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
        return QIcon(":icons/vmodel16");
    else
        return QIcon(":icons32/vmodel32");
}

void VariogramModel::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
}

void VariogramModel::readParameters()
{

    //get info on the physical file that stores the variogram model data
    QFileInfo info( _path );

    //if read data is not empty and was loaded before
    if( ! m_it.empty() && ! _lastModifiedDateTimeLastRead.isNull() ){
        QDateTime currentLastModified = info.lastModified();
        //if modified datetime didn't change since last call to readParameters
        if( currentLastModified <= _lastModifiedDateTimeLastRead ){
            return; //does nothing
        }
    }

    //record the current datetime of file change
    _lastModifiedDateTimeLastRead = info.lastModified();

    //reset the ranges and angles collections
    m_it.clear();
    m_cc.clear();
    m_a_hMax.clear();
    m_a_hMin.clear();
    m_a_vert.clear();
    m_Azimuth.clear();
    m_Dip.clear();
    m_Roll.clear();

    //Variogram models are stored as parameter files for the vmodel GSLib program,
    //ergo, to read the values, just create a parameter file object for vmodel
    GSLibParameterFile par_vmodel( "vmodel" );
    par_vmodel.setValuesFromParFile( getPath() );

    //get the nugget effect
    GSLibParMultiValuedFixed *par3 = par_vmodel.getParameter<GSLibParMultiValuedFixed*>(3);
    m_Nugget = par3->getParameter<GSLibParDouble*>(1)->_value;

    //Sill starts with the nugget effect value.
    m_Sill = m_Nugget;

    //get the number of variogram structures
    GSLibParRepeat *par4 = par_vmodel.getParameter<GSLibParRepeat*>(4); //repeat nst-times
    m_nst = par3->getParameter<GSLibParUInt*>(0)->_value;
    par4->setCount( m_nst );

    //for each structure...
    for( uint inst = 0; inst < m_nst; ++inst)
    {
        GSLibParMultiValuedFixed *par4_0 = par4->getParameter<GSLibParMultiValuedFixed*>(inst, 0);
        //...collect the struture type
        m_it.append( (VariogramStructureType)par4_0->getParameter<GSLibParOption*>(0)->_selected_value );
        //...collect the contribution
        m_cc.append( par4_0->getParameter<GSLibParDouble*>(1)->_value );
        //...add the contribution to the Sill value
        m_Sill += par4_0->getParameter<GSLibParDouble*>(1)->_value;
        //...collect the angles
        m_Azimuth.append( par4_0->getParameter<GSLibParDouble*>(2)->_value );
        m_Dip.append( par4_0->getParameter<GSLibParDouble*>(3)->_value );
        m_Roll.append( par4_0->getParameter<GSLibParDouble*>(4)->_value );
        //...collect the ranges
        GSLibParMultiValuedFixed *par4_1 = par4->getParameter<GSLibParMultiValuedFixed*>(inst, 1);
        m_a_hMax.append( par4_1->getParameter<GSLibParDouble*>(0)->_value );
        m_a_hMin.append( par4_1->getParameter<GSLibParDouble*>(1)->_value );
        m_a_vert.append( par4_1->getParameter<GSLibParDouble*>(2)->_value );
	}
}

VariogramModel VariogramModel::makeVModelWithoutNugget()
{
	VariogramModel result;
	result.m_Sill = m_Sill - m_Nugget;
	result.m_Nugget = 0.0;
	result.m_nst = m_nst; //number of structures is the same, since nugget does not comput as one actual structure.
	result.m_it = m_it;
	result.m_cc = m_cc;
	result.m_a_hMax = m_a_hMax;
	result.m_a_hMin = m_a_hMin;
	result.m_a_vert = m_a_vert;
	result.m_Azimuth = m_Azimuth;
	result.m_Dip = m_Dip;
	result.m_Roll = m_Roll;
    return result;
}

File *VariogramModel::duplicatePhysicalFiles(const QString new_file_name)
{
    QString duplicateFilePath = duplicateDataAndMetaDataFiles( new_file_name );
    return new VariogramModel( duplicateFilePath );
}

bool VariogramModel::forceReread() const
{
    return _forceReread;
}

void VariogramModel::setForceReread(bool forceReread)
{
    if( ! forceReread)
        Application::instance()->logWarn("VariogramModel::setForceReread(): WARNING! Automatic reread disabled!");
    else
        Application::instance()->logInfo("VariogramModel::setForceReread(): Automatic reread restored.");
    _forceReread = forceReread;
}

