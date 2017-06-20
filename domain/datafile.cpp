#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <limits>
#include <iomanip>      // std::setprecision
#include <sstream>    // std::stringstream
#include <cmath>
#include "datafile.h"
#include "../exceptions/invalidgslibdatafileexception.h"
#include "application.h"
#include "../util.h"
#include "attribute.h"
#include "weight.h"
#include "normalvariable.h"
#include "cartesiangrid.h"
#include "domain/univariatecategoryclassification.h"
#include "domain/categorydefinition.h"
#include "project.h"
#include "objectgroup.h"

//-------------------------------possible fast string split for the DataFile::loadData() method------------------------
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
template<typename Out>
void fast_split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        if (!item.empty()) //skip blank tokens
            *(result++) = std::stod(item);
    }
}
std::vector<double> fast_split(const std::string &s, char delim) {
    std::vector<double> elems;
    fast_split(s, delim, std::back_inserter(elems));
    return elems;
}
//------------------------------------------------------------------------------------------------------------


DataFile::DataFile(QString path) : File( path )
{
}

void DataFile::loadData()
{
    //TODO: prevent unnecessary data reloads that might cause nuisance with larger files.

    QStringList list;
    QFile file( this->_path );
    file.open( QFile::ReadOnly | QFile::Text );
    QTextStream in(&file);
    int n_vars = 0;
    int var_count = 0;
    uint data_line_count = 0;

    Application::instance()->logInfo(QString("Loading data from ").append(this->_path).append("..."));

    //make sure _data is emply
    _data.clear();

    for (int i = 0; !in.atEnd(); ++i)
    {
       //read file line by line
       QString line = in.readLine();

       //TODO: second line may contain other information in grid files, so it will fail for such cases.
       if( i == 0 ){} //first line is ignored
       else if( i == 1 ){ //second line is the number of variables
           n_vars = Util::getFirstNumber( line );
       } else if ( i > 1 && var_count < n_vars ){ //the variables names
           list << line;
           ++var_count;
       } else { //lines containing data
           std::vector<double> data_line;
           //TODO: this maybe a bottleneck for large data files
           QStringList values = line.split(QRegularExpression("\\s+"), QString::SkipEmptyParts);
           if( values.size() != n_vars ){
               Application::instance()->logError( QString("ERROR: wrong number of values in line ").append(QString::number(i)) );
               Application::instance()->logError( QString("       expected: ").append(QString::number(n_vars)).append(", found:").append(QString::number(values.size())) );
               for( QStringList::Iterator it = values.begin(); it != values.end(); ++it ){
                   Application::instance()->logInfo((*it));
               }
           } else {
               //read each value along the line
               for( QStringList::Iterator it = values.begin(); it != values.end(); ++it ){
                   bool ok = true;
                   data_line.push_back( (*it).toDouble( &ok ) );
                   if( !ok ){
                       Application::instance()->logError( QString("DataFile::loadData(): error in data file (line ").append(QString::number(i)).append("): cannot convert ").append( *it ).append(" to double.") );
                   }
               }
               //add the line to the list
               this->_data.push_back( data_line );
               ++data_line_count;
           }
       }
    }
    file.close();

    //cartesian grids must have a given number of read lines
    if( this->getFileType() == "CARTESIANGRID"){
        CartesianGrid* cg = (CartesianGrid*)this;
        uint expected_total_lines = cg->getNX() * cg->getNY() * cg->getNZ() * cg->getNReal();
        if( data_line_count != expected_total_lines )
            Application::instance()->logWarn( QString("DataFile::loadData(): number of parsed data lines differs from the expected lines computed from the cartesian grid parameters. The application, a GSLib program or Ghostscript may fail.") );
    }

    Application::instance()->logInfo("Finished loading data.");
}

double DataFile::data(uint line, uint column)
{
    return (this->_data.at(line)).at(column);
}

//TODO: consider adding a flag to disable NDV checking (applicable to coordinates)
double DataFile::max(uint column)
{
    if( _data.size() == 0 )
        Application::instance()->logError("DataFile::max(): Data not loaded. Unspecified value was returned.");
    double ndv = this->getNoDataValue().toDouble();
    bool has_ndv = this->hasNoDataValue();
    double result = -std::numeric_limits<double>::max();
    for( uint i = 0; i < _data.size(); ++i ){
        double value = data(i, column);
        if( value > result && ( !has_ndv || !Util::almostEqual2sComplement( ndv, value, 1 ) ) )
            result = value;
    }
    return result;
}

//TODO: consider adding a flag to disable NDV checking (applicable to coordinates)
double DataFile::min(uint column)
{
    if( _data.size() == 0 )
        Application::instance()->logError("DataFile::min(): Data not loaded. Unspecified value was returned.");
    double ndv = this->getNoDataValue().toDouble();
    bool has_ndv = this->hasNoDataValue();
    double result = std::numeric_limits<double>::max();
    for( uint i = 0; i < _data.size(); ++i ){
        double value = data(i, column);
        if( value < result && ( !has_ndv || !Util::almostEqual2sComplement( ndv, value, 1 ) ) )
            result = value;
    }
    return result;
}

//TODO: consider adding a flag to disable NDV checking (applicable to coordinates)
double DataFile::mean(uint column)
{
    if( _data.size() == 0 )
        Application::instance()->logError("DataFile::mean(): Data not loaded. Unspecified value was returned.");
    double ndv = this->getNoDataValue().toDouble();
    bool has_ndv = this->hasNoDataValue();
    double result = 0.0;
    uint count_valid = 0;
    for( uint i = 0; i < _data.size(); ++i ){
        double value = data(i, column);
        if( !has_ndv || !Util::almostEqual2sComplement( ndv, value, 1 ) ){
            result += value;
            ++count_valid;
        }
    }
    if( count_valid > 0)
        return result / count_valid;
    else
        return 0.0;
}

uint DataFile::getFieldGEOEASIndex(QString field_name)
{
    QStringList field_names = Util::getFieldNames( this->_path );
    for( int i = 0; i < field_names.size(); ++i){
        if( field_names.at(i).trimmed() == field_name.trimmed() )
            return i+1;
    }
    return 0;
}

Attribute *DataFile::getAttributeFromGEOEASIndex(uint index)
{
    std::vector<ProjectComponent*>::iterator it = this->_children.begin();
    for( ; it != this->_children.end(); ++it ){
        ProjectComponent* pi = *it;
        if( pi->isAttribute() ){
            Attribute* at = (Attribute*)pi;
            uint at_index = this->getFieldGEOEASIndex( at->getName() );
            if( at_index == index )
                return at;
        }
    }
    return nullptr;
}

uint DataFile::getLastFieldGEOEASIndex()
{
    QStringList field_names = Util::getFieldNames( this->_path );
    return field_names.count();
}

QString DataFile::getNoDataValue()
{
    return this->_no_data_value;
}

double DataFile::getNoDataValueAsDouble()
{
    bool OK;
    double result = getNoDataValue().toDouble( &OK );
    if( ! OK )
        return std::nan("");
    return result;
}

void DataFile::setNoDataValue(const QString new_ndv)
{
    this->_no_data_value = new_ndv;
    this->updateMetaDataFile();
}

bool DataFile::hasNoDataValue()
{
    return !this->_no_data_value.trimmed().isEmpty();
}

bool DataFile::isNormal(Attribute *at)
{
    uint index_in_GEOEAS_file = this->getFieldGEOEASIndex( at->getName() );
    return _nsvar_var_trn.contains( index_in_GEOEAS_file );
}

bool DataFile::isCategorical(Attribute *at)
{
    uint index_in_GEOEAS_file = this->getFieldGEOEASIndex( at->getName() );
    QList< QPair<uint, QString> >::iterator it = _categorical_attributes.begin();
    for(; it != _categorical_attributes.end(); ++it){
        if( (*it).first == index_in_GEOEAS_file )
            return true;
    }
    return false;
}

CategoryDefinition *DataFile::getCategoryDefinition(Attribute *at)
{
    uint index_in_GEOEAS_file = this->getFieldGEOEASIndex( at->getName() );
    QList< QPair<uint, QString> >::iterator it = _categorical_attributes.begin();
    for(; it != _categorical_attributes.end(); ++it){
        if( (*it).first == index_in_GEOEAS_file ){
            QString cd_file_name = (*it).second;
            return (CategoryDefinition*)Application::instance()->
                    getProject()->getResourcesGroup()->getChildByName( cd_file_name );
        }
    }
    return nullptr;
}

Attribute *DataFile::getVariableOfNScoreVar(Attribute *at)
{
    uint ns_var_index_in_GEOEAS_file = this->getFieldGEOEASIndex( at->getName() );
    Attribute* variable = nullptr;
    if( _nsvar_var_trn.contains( ns_var_index_in_GEOEAS_file ) ){
        QPair<uint, QString> var_index_in_GEOEAS_file_and_transform_table =
                _nsvar_var_trn[ ns_var_index_in_GEOEAS_file ];
        variable = this->getAttributeFromGEOEASIndex( var_index_in_GEOEAS_file_and_transform_table.first );
    }
    return variable;
}

void DataFile::deleteFromFS()
{
    File::deleteFromFS(); //delete the file itself.
    //also deletes the metadata file
    QFile file( this->getMetaDataFilePath() );
    file.remove(); //TODO: throw exception if remove() returns false (fails).  Also see QIODevice::errorString() to see error message.
}

void DataFile::writeToFS()
{
    //create a new file for output
    QFile outputFile( QString( this->getPath() ).append(".new") );
    outputFile.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&outputFile);

    //if file already exists, keep copy of the file description or make up one otherwise
    QString comment;
    if( this->exists() )
        comment = Util::getGEOEAScomment( this->getPath() );
    else
        comment = this->getFileType() + " created by GammaRay";
    out << comment << endl;

    //next, we need to know the number of columns
    //(assumes the first data line has the correct number of variables)
    uint nvars = _data[0].size();
    out << nvars << endl;

    //get all child objects (mostly attributes directly under this file or attached under another attribute)
    //we do this because some attributes (columns) may not be in the current GEO-EAS file.
    std::vector<ProjectComponent*> allChildren;
    this->getAllObjects( allChildren );

    //for each GEO-EAS column index (start with 1, not zero)
    uint control = 0;
    for( uint iGEOEAS = 1; iGEOEAS <= nvars; ++iGEOEAS){
        //find the attribute by the GEO-EAS index
        std::vector<ProjectComponent*>::iterator it = allChildren.begin();
        for(; it != allChildren.end(); ++it){
            if( (*it)->isAttribute() ){
                Attribute* at = (Attribute*)(*it);
                if( at->getAttributeGEOEASgivenIndex() == (int)iGEOEAS ){
                    out << at->getName() << endl;
                    ++control;
                    break;
                }
            }
        }
    }

    if( control != nvars ){
        Application::instance()->logWarn("WARNING: DataFile::writeToFS(): mismatch between data column count and Attribute object count.");
        //make up names for mismatched data columns
        for( uint iGEOEAS = control; iGEOEAS <= nvars; ++iGEOEAS){
            out << "ATTRIBUTE " << iGEOEAS << endl;
        }
    }

    //for each data line
    std::vector< std::vector<double> >::iterator itDataLine = _data.begin();
    for(; itDataLine != _data.end(); ++itDataLine){
        //for each data column
        std::vector<double>::iterator itDataColumn = (*itDataLine).begin();
        out << *itDataColumn;
        ++itDataColumn;
        for(; itDataColumn != (*itDataLine).end(); ++itDataColumn){
            //making sure the values are written in GSLib-like precision
            std::stringstream ss;
            ss << std::setprecision( 12 /*std::numeric_limits<double>::max_digits10*/ );
            ss << *itDataColumn;
            out << '\t' << ss.str().c_str();
        }
        out << endl;
    }

    //close output file
    outputFile.close();

    //deletes the current file
    QFile currentFile( this->getPath() );
    currentFile.remove();
    //renames the .new file, effectively replacing the current file.
    outputFile.rename( this->getPath() );
    //updates properties list so any changes appear in the project tree.
    updatePropertyCollection();
    //update the project tree in the main window.
    Application::instance()->refreshProjectTree();
}

QString DataFile::getObjectLocator()
{
    return "DATAFILE:" + getFileType() + ":" + getName();
}

void DataFile::updatePropertyCollection()
{
    //updates attribute collection
    this->_children.clear(); //TODO: deallocate elements/deep delete (minor memory leak)
    QStringList fields = Util::getFieldNames( this->_path );
    for( int i = 0; i < fields.size(); ++i ){
        int index_in_file = i + 1;
        //do not include the x,y,z coordinates among the attributes
        /*if( index_in_file != this->_x_field_index &&
            index_in_file != this->_y_field_index &&
            index_in_file != this->_z_field_index ){*/
            Attribute *at = new Attribute( fields[i].trimmed(), index_in_file );
            if( isWeight( at ) ){ //if attribute is a weight, it is represented as a child object
                                  //of the variable it refers to
                Attribute* variable = this->getVariableOfWeight( at );
                if( ! variable )
                    Application::instance()->logError("Error in variable-weight assignment.");
                else{
                    delete at; //it is not a generic Attribute
                    Weight* wgt = new Weight( fields[i].trimmed(), index_in_file, variable );
                    variable->addChild( wgt );
                }
            } else if( isNormal( at ) ){ //if attribute is a normal transform of another variable,
                                         //it is represented as a child object of the variable it refers to
                Attribute* variable = this->getVariableOfNScoreVar( at );
                if( ! variable )
                    Application::instance()->logError("Error in variable-normal variable assignment.");
                else{
                    delete at; //it is not a generic Attribute
                    NormalVariable* ns_var = new NormalVariable( fields[i].trimmed(), index_in_file, variable );
                    variable->addChild( ns_var );
                }
            } else { //common variables are direct children of files
                if( isCategorical( at ) )
                    at->setCategorical( true );
                this->_children.push_back( at );
                at->setParent( this );
            }
        /*}*/
    }
}

void DataFile::replacePhysicalFile(const QString from_file_path)
{
    //copies the source file over the current physical file in project.
    Util::copyFile( from_file_path, _path );
    //updates properties list so any changes appear in the project tree.
    updatePropertyCollection();
    //update the project tree in the main window.
    Application::instance()->refreshProjectTree();
}

void DataFile::addVariableNScoreVariableRelationship(uint variableGEOEASindex, uint nScoreVariableGEOEASindex, QString trn_file_name)
{
    this->_nsvar_var_trn[nScoreVariableGEOEASindex] = QPair<uint,QString>(variableGEOEASindex, trn_file_name);
    //save the updated metadata to disk.
    this->updateMetaDataFile();
}

void DataFile::addGEOEASColumn(Attribute *at, const QString new_name, bool categorical, CategoryDefinition *cd)
{
    //set the variable name in the destination file
    QString var_name;
    if( new_name.isEmpty() )
        var_name = at->getName();
    else
        var_name = new_name;

    //get the destination file path
    QString file_path = this->getPath();

    //get the source file
    DataFile* attributes_file = (DataFile*)at->getContainingFile();

    //loads the data in source file
    attributes_file->loadData();

    //get the variable's column index in the source file
    uint column_index_in_original_file = attributes_file->getFieldGEOEASIndex( at->getName() ) - 1;

    //get the number of data lines in the source file
    uint data_line_count = attributes_file->getDataLineCount();

    //create a new file for output
    QFile outputFile( QString(file_path).append(".new") );
    outputFile.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&outputFile);

    //set the no-data value to be used
    QString NDV;
    if( this->hasNoDataValue() ) //the expected no-data value is from the destination file (this object)
        NDV = this->getNoDataValue();
    else if( attributes_file->hasNoDataValue() ){ //try the one from the source file
        NDV = attributes_file->getNoDataValue();
        Application::instance()->logWarn("WARNING: DataFile::addGEOEASColumn(): no-data value not set for the destination file.  Using the no-data value from the source file: " + NDV);
    } else {
        NDV = "-9999999";
        Application::instance()->logWarn("WARNING: DataFile::addGEOEASColumn(): no-data value not set for both files.  Using -9999999.");
    }

    //open the destination file for reading
    QFile inputFile( file_path );
    if ( inputFile.open(QIODevice::ReadOnly | QFile::Text ) ) {
       QTextStream in(&inputFile);
       uint line_index = 0;
       uint data_line_index = 0;
       uint n_vars = 0;
       uint var_count = 0;
       uint indexGEOEAS_new_variable = 0;
       //for each line in the destination file...
       while ( !in.atEnd() ){
           //...read its line
          QString line = in.readLine();
          //simply copy the first line (title)
          if( line_index == 0 ){
              out << line << '\n';
          //first number of second line holds the variable count
          //writes an increased number of variables.
          //TODO: try to keep the rest of the second line (not critical, but desirable)
          } else if( line_index == 1 ) {
              n_vars = Util::getFirstNumber( line );
              out << ( n_vars+1 ) << '\n';
              indexGEOEAS_new_variable = n_vars+1; //the GEO-EAS index of the added column equals the new number of columns
          //simply copy the current variable names
          } else if ( var_count < n_vars ) {
              out << line << '\n';
              //if we're at the last existing variable, adds an extra line for the new variable
              if( (var_count+1) == n_vars ){
                  out << var_name << '\n';
              }
              ++var_count;
          //treat the data lines until EOF
          } else {
              //if we didn't overshoot the source file...
              if( data_line_index < data_line_count ) {
                  double value = attributes_file->data( data_line_index, column_index_in_original_file );
                  //if the value was NOT defined as no-data value according to its parent file
                  if( ! attributes_file->isNDV( value ) ){
                      out << line << '\t' << QString::number( value ) << '\n';
                  //if the value is no-data value according to the source file
                  } else {
                      out << line << '\t' << NDV << '\n';
                  }
              } else {
                  //...otherwise append the no-data value of the destination file (this object).
                  out << line << '\t' << NDV << '\n';
              }
              //keep count of the source file data lines
              ++data_line_index;
          } //if's and else's for each file line case (header, var. count, var. name and data line)
          //keep count of the source file lines
          ++line_index;
       } // for each line in destination file (this)
       //close the destination file
       inputFile.close();
       //close the newly created file
       outputFile.close();
       //deletes the destination file
       inputFile.remove();
       //renames the new file, effectively replacing the destination file.
       outputFile.rename( QFile( file_path ).fileName() );
       //if the added column was deemed categorical, adds its GEO-EAS index and name of the category definition
       //to the list of pairs for metadata keeping.
       if( categorical ){
           //TODO: guard against cd being nullptr
           _categorical_attributes.append( QPair<uint,QString>( indexGEOEAS_new_variable , cd->getName() ) );
           //update the metadata file
           this->updateMetaDataFile();
       }
       //updates properties list so any changes appear in the project tree.
       updatePropertyCollection();
       //update the project tree in the main window.
       Application::instance()->refreshProjectTree();
    }
}

uint DataFile::getDataLineCount()
{
    return _data.size();
}

uint DataFile::getDataColumnCount()
{
    if( getDataLineCount() > 0 )
        return _data[0].size();
    else
        return 0;
}

bool DataFile::isNDV(double value)
{
    if( ! this->hasNoDataValue() )
        return false;
    else{
        double ndv = this->getNoDataValue().toDouble();
        return Util::almostEqual2sComplement( ndv, value, 1 );
    }
}

void DataFile::classify(uint column, UnivariateCategoryClassification *ucc, const QString name_for_new_column )
{
    //load the current data from the file system
    loadData();

    //for each data row...
    std::vector< std::vector <double> >::iterator it = _data.begin();
    for( ; it != _data.end(); ++it){
        //define the default value (for class not found)
        int noClassFoundValue = -1;
        if( hasNoDataValue() )
            //hopefully the file's NDV is integer
            noClassFoundValue = (int)getNoDataValue().toDouble();
        //...get the input value
        double value = (*it).at( column );
        //...get the category code corresponding to the value
        int categoryId = ucc->getCategory( value, noClassFoundValue );
        //...append the code to the current row.
        (*it).push_back( categoryId );
    }

    //create and add a new Attribute object the represents the new column
    uint newIndexGEOEAS = Util::getFieldNames( this->getPath() ).count() + 1;
    Attribute* at = new Attribute( name_for_new_column, newIndexGEOEAS, true );
    //adds the attribute's GEO-EAS index (with the name of the category definition file) to the metadata as a categorical attribute
    _categorical_attributes.append(
                QPair<uint,QString>(newIndexGEOEAS,
                                    ucc->getCategoryDefinition()->getName()) );
    at->setParent( this );
    this->addChild( at );

    //saves the file contents to file system
    this->writeToFS();

    //update the metadata file
    this->updateMetaDataFile();
}
