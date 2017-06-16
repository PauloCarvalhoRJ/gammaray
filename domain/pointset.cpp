#include "pointset.h"
#include <QTextStream>
#include <QFile>
#include <QStringList>
#include <QRegularExpression>
#include "../util.h"
#include "attribute.h"
#include "application.h"
#include "../exceptions/invalidgslibdatafileexception.h"
#include "weight.h"
#include "util.h"

PointSet::PointSet( QString path ) : DataFile( path )
{
    this->_x_field_index = 0;
    this->_y_field_index = 0;
    this->_z_field_index = 0;
    this->_no_data_value = "";
}

QIcon PointSet::getIcon()
{
    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
        return QIcon(":icons/pointset16");
    else
        return QIcon(":icons32/pointset32");
}

void PointSet::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
    //also saves the metadata file.
    this->updateMetaDataFile();
}

vtkSmartPointer<vtkProp> PointSet::buildVTKActor()
{
    return View3DBuilders::build( this );
}

void PointSet::setInfo(int x_index, int y_index, int z_index, const QString no_data_value)
{
    QMap<uint, uint> empty;
    QMap< uint, QPair<uint, QString> > empty2;
    QList< QPair< uint, QString> > empty3;
    this->setInfo( x_index, y_index, z_index, no_data_value, empty, empty2, empty3);
}

void PointSet::setInfo(int x_index, int y_index, int z_index, const QString no_data_value,
                       const QMap<uint, uint> &wgt_var_pairs,
                       const QMap<uint, QPair<uint, QString> > &nvar_var_trn_triads,
                       const QList< QPair<uint,QString> > &categorical_attributes)
{
    //updates metadata
    this->_x_field_index = x_index;
    this->_y_field_index = y_index;
    this->_z_field_index = z_index;
    this->_no_data_value = no_data_value;
    _wgt_var_pairs.clear();
    _wgt_var_pairs.unite( wgt_var_pairs );
    _nsvar_var_trn.clear();
    _nsvar_var_trn.unite( nvar_var_trn_triads );
    _categorical_attributes.clear();
    _categorical_attributes << categorical_attributes;
    this->updatePropertyCollection();
}


bool PointSet::isWeight(Attribute *at)
{
    uint index_in_GEOEAS_file = this->getFieldGEOEASIndex( at->getName() );
    return _wgt_var_pairs.contains( index_in_GEOEAS_file );
}

Attribute *PointSet::getVariableOfWeight(Attribute *weight)
{
    uint wgt_index_in_GEOEAS_file = this->getFieldGEOEASIndex( weight->getName() );
    Attribute* variable = nullptr;
    if( _wgt_var_pairs.contains( wgt_index_in_GEOEAS_file ) ){
        uint var_index_in_GEOEAS_file = _wgt_var_pairs[ wgt_index_in_GEOEAS_file ];
        variable = this->getAttributeFromGEOEASIndex( var_index_in_GEOEAS_file );
    }
    return variable;
}

void PointSet::setInfoFromMetadataFile()
{
    QString md_file_path( this->_path );
    QFile md_file( md_file_path.append(".md") );
    int x_index = 0, y_index = 0, z_index = 0;
    QMap<uint, uint> wgt_var_pairs;
    QMap<uint, QPair<uint,QString> > nsvar_var_trn;
    QList< QPair<uint,QString> > categorical_attributes;
    QString ndv;
    if( md_file.exists() ){
        md_file.open( QFile::ReadOnly | QFile::Text );
        QTextStream in(&md_file);
        for (int i = 0; !in.atEnd(); ++i)
        {
           QString line = in.readLine();
           if( line.startsWith( "X:" ) ){
               QString value = line.split(":")[1];
               x_index = value.toInt();
           }else if( line.startsWith( "Y:" ) ){
               QString value = line.split(":")[1];
               y_index = value.toInt();
           }else if( line.startsWith( "Z:" ) ){
               QString value = line.split(":")[1];
               z_index = value.toInt();
           }else if( line.startsWith( "NDV:" ) ){
               QString value = line.split(":")[1];
               ndv = value;
           }else if( line.startsWith( "WEIGHT:" ) ){
               QString pair = line.split(":")[1];
               QString var = pair.split(">")[0];
               QString wgt = pair.split(">")[1];
               //weight index is key
               //variable index is value
               wgt_var_pairs.insert( wgt.toUInt(), var.toUInt() );
           }else if( line.startsWith( "NSCORE:" ) ){
               QString triad = line.split(":")[1];
               QString var = triad.split(">")[0];
               QString pair = triad.split(">")[1];
               QString ns_var = pair.split("=")[0];
               QString trn_filename = pair.split("=")[1];
               //normal variable index is key
               //variable index and transform table filename are the value
               nsvar_var_trn.insert( ns_var.toUInt(), QPair<uint,QString>(var.toUInt(), trn_filename ));
           }else if( line.startsWith( "CATEGORICAL:" ) ){
               QString var_and_catDefName = line.split(":")[1];
               QString var = var_and_catDefName.split(",")[0];
               QString catDefName = var_and_catDefName.split(",")[1];
               categorical_attributes.append( QPair<uint,QString>( var.toUInt(), catDefName ) );
           }
        }
        md_file.close();
        this->setInfo( x_index, y_index, z_index, ndv, wgt_var_pairs, nsvar_var_trn, categorical_attributes );
    }
}

int PointSet::getXindex()
{
    return this->_x_field_index;
}

int PointSet::getYindex()
{
    return this->_y_field_index;
}

int PointSet::getZindex()
{
    return this->_z_field_index;
}

bool PointSet::is3D()
{
    return getZindex() > 0;
}

void PointSet::addVariableWeightRelationship(uint variableGEOEASindex, uint weightGEOEASindex)
{
    this->_wgt_var_pairs[weightGEOEASindex] = variableGEOEASindex;
    //save the updated metadata to disk.
    this->updateMetaDataFile();
}

bool PointSet::canHaveMetaData()
{
    return true;
}

QString PointSet::getFileType()
{
    return "POINTSET";
}

void PointSet::updateMetaDataFile()
{
    QFile file( this->getMetaDataFilePath() );
    file.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&file);
    out << APP_NAME << " metadata file.  This file is generated automatically.  Do not edit this file.\n";
    out << "version=" << APP_VERSION << '\n';
    out << "X:" << this->_x_field_index << '\n';
    out << "Y:" << this->_y_field_index << '\n';
    out << "Z:" << this->_z_field_index << '\n';
    out << "NDV:" << this->_no_data_value << '\n';
    QMapIterator<uint, uint> i( this->_wgt_var_pairs );
    while (i.hasNext()) {
        i.next();
        out << "WEIGHT:" << i.value() << '>' << i.key() << '\n';
    }
    QMapIterator<uint, QPair<uint,QString> > j( this->_nsvar_var_trn );
    while (j.hasNext()) {
        j.next();
        out << "NSCORE:" << j.value().first << '>' << j.key() << '=' << j.value().second << '\n';
    }
    QList< QPair<uint,QString> >::iterator k = _categorical_attributes.begin();
    for(; k != _categorical_attributes.end(); ++k){
        out << "CATEGORICAL:" << (*k).first << "," << (*k).second << '\n';
    }
    file.close();
}
