#include "util.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QLayoutItem>
#include <QLayout>
#include <QWidget>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QScreen>
#include <QApplication>
#include <cassert>
#include <stdint.h>
#include "exceptions/invalidgslibdatafileexception.h"
#include "domain/application.h"
#include "domain/cartesiangrid.h"
#include "domain/attribute.h"
#include "domain/project.h"
#include "domain/categorydefinition.h"
#include "domain/pointset.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparams/gslibparinputdata.h"
#include "gslib/gslib.h"
#include "displayplotdialog.h"
#include "distributioncolumnrolesdialog.h"
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QSettings>
#include <cmath>

/*static*/const QString Util::VARMAP_NDV("-999.00000");

Util::Util()
{
}

QStringList Util::getFieldNames(const QString gslib_data_file_path)
{
    QStringList list;
    QFile file( gslib_data_file_path );
    file.open( QFile::ReadOnly | QFile::Text );
    QTextStream in(&file);
    int n_vars = 0;
    int var_count = 0;
    for (int i = 0; !in.atEnd(); ++i)
    {
       //read file line by line
       QString line = in.readLine();
       //second line is the number of variables
       //TODO: second line may contain other information in grid files, so it will fail for such cases
       if( i == 1 ){
           n_vars = Util::getFirstNumber( line );
       } else if ( i > 1 && var_count < n_vars ){
           list << line;
           ++var_count;
           if( var_count == n_vars )
               break;
       }
    }
    file.close();
    return list;
}

std::pair<QStringList, QString> Util::parseTagsAndDescription( const QString gslib_param_file_template_line )
{
    QStringList tags;
    QString description;
    QRegularExpression re_tags("(<.*?>)");
    QRegularExpression re_description(".*>\\s*-(.*)");

    //finding the individual tags
    QRegularExpressionMatchIterator i = re_tags.globalMatch(gslib_param_file_template_line);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
             tags.append( match.captured(1) );
        }
    }

    //finding the description
    QRegularExpressionMatch match = re_description.match( gslib_param_file_template_line );
    if( match.hasMatch() ){
        description = match.captured(1);
    }

    return std::pair<QStringList, QString>(tags, description);
}

uint Util::getIndentation(const QString gslib_param_file_template_line)
{
    uint indent = 0;
    QRegularExpression re_name("(\\s*)<.*");
    //finding the leading spaces
    QRegularExpressionMatch match = re_name.match( gslib_param_file_template_line );
    if( match.hasMatch() ){
        indent = match.captured(1).length();
    }
    return indent;
}

QString Util::getNameFromTag(const QString tag)
{
    QString name;
    QRegularExpression re_name("<(.*?)[\\+\\s>]+.*");
    //finding the type name
    QRegularExpressionMatch match = re_name.match( tag );
    if( match.hasMatch() ){
        name = match.captured(1);
    }
    return name;
}

bool Util::hasPlusSign(const QString tag)
{
    QRegularExpression re_name("<.*\\+.*>");
    //finding the plus sign (if any)
    QRegularExpressionMatch match = re_name.match( tag );
    return match.hasMatch();
}

QString Util::getRefNameFromTag(const QString tag)
{
    QString name;
    QRegularExpression re_name("\\((.*)\\)");
    //finding the reference name
    QRegularExpressionMatch match = re_name.match( tag );
    if( match.hasMatch() ){
        name = match.captured(1);
    }
    return name;
}

std::vector< std::pair<QString, QString> > Util::getTagOptions(const QString tag)
{
    std::vector< std::pair<QString, QString> > options;
    QRegularExpression re_option("\\[(.*?):(.*?)\\]");
    //finding the individual options
    QRegularExpressionMatchIterator i = re_option.globalMatch( tag );
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
            options.push_back( std::pair<QString, QString>(match.captured(1), match.captured(2)) );
        }
    }
    return options;
}

QString Util::getReferenceName(const QString tag)
{
    QString ref_name;
    QRegularExpression re_name("\\[(\\w*)\\]");
    //finding the reference tag name
    QRegularExpressionMatch match = re_name.match( tag );
    if( match.hasMatch() ){
        ref_name = match.captured(1);
    }
    return ref_name;
}

bool Util::almostEqual2sComplement(float A, float B, int maxUlps)
{
    // Make sure maxUlps is non-negative and small enough that the
    // default NAN won't compare as equal to anything.
    assert(maxUlps > 0 && maxUlps < 4 * 1024 * 1024);
    int aInt = *(int*)&A;
    // Make aInt lexicographically ordered as a twos-complement int
    if (aInt < 0)
        aInt = 0x80000000 - aInt;
    // Make bInt lexicographically ordered as a twos-complement int
    int bInt = *(int*)&B;
    if (bInt < 0)
        bInt = 0x80000000 - bInt;
    int intDiff = abs(aInt - bInt);
    if (intDiff <= maxUlps)
        return true;
    return false;
}

bool Util::almostEqual2sComplement(double A, double B, int maxUlps)
{
    // Make sure maxUlps is non-negative and small enough that the
    // default NAN won't compare as equal to anything.
    //<cassert>'s assert doesn't accept longs
    //assert(maxUlps > 0 && maxUlps < 2 * 1024 * 1024 * 1024 * 1024 * 1024);
    assert(maxUlps > 0 && maxUlps < 4 * 1024 * 1024);
    int64_t aLong = *reinterpret_cast<int64_t*>( &A ); //use the raw bytes from the double to make a long int value (type punning)
    // Make aLong lexicographically ordered as a twos-complement long
    if (aLong < 0)
        aLong = 0x8000000000000000 - aLong;
    // Make bLong lexicographically ordered as a twos-complement long
    int64_t bLong = *reinterpret_cast<int64_t*>( &B ); //use the raw bytes from the double to make a long int value (type punning)
    if (bLong < 0)
        bLong = 0x8000000000000000 - bLong;
    int64_t longDiff = (aLong - bLong) & 0x7FFFFFFFFFFFFFFF;
    if (longDiff <= maxUlps)
        return true;
    return false;
}

void Util::clearChildWidgets(QWidget *widget)
{
    QLayoutItem* item;
    while ( ( item = widget->layout()->takeAt( 0 ) ) )
    {
        delete item->widget();
        delete item;
    }
}

void Util::readFileSample(QPlainTextEdit *text_field, QString file_path)
{
    //read file content sample
    QFile file( file_path );
    file.open( QFile::ReadOnly | QFile::Text );
    QTextStream in(&file);
    //read up to 100 first lines
    for (int i = 0; !in.atEnd() && i < 100; ++i)
    {
       QString line = in.readLine();
       text_field->appendPlainText( line );
    }
    file.close();
    //send text cursor to home
    QTextCursor tmpCursor = text_field->textCursor();
    tmpCursor.movePosition(QTextCursor::Start);
    text_field->setTextCursor(tmpCursor);
}

void Util::addTrailingHiphens(const QString par_file_path)
{
    //open a new file for output
    QFile outputFile( QString(par_file_path).append(".new") );
    outputFile.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&outputFile);

    //open the current file for reading
    QFile inputFile( par_file_path );
    if ( inputFile.open(QIODevice::ReadOnly | QFile::Text ) ) {
       QTextStream in(&inputFile);
       bool in_header = true; //flags whether file read is still in header
       while ( !in.atEnd() ){
          QString line = in.readLine();
          if( in_header ){
              out << line << "\n"; //write header lines without change.
              if( line.trimmed().startsWith( "START OF PARAMETERS", Qt::CaseInsensitive ) )
                 in_header = false;
          } else {
              //replaces end line characters with spaces and appens the trailing hiphen
              line.replace('\n', ' ');
              line.replace('\r', ' ');
              line.append("     -   \n");
              //writes the modified line to output file
              out << line;
          }
       }
       inputFile.close();

       //closes the output file
       outputFile.close();

       //deletes current file
       inputFile.remove();

       //renames the new file
       outputFile.rename( QFile( par_file_path ).fileName() );
    }
}

uint Util::getFirstNumber(const QString line)
{
    uint result = 0;
    QRegularExpression re("(\\d+).*");
    QRegularExpressionMatch match = re.match( line );
    if( match.hasMatch() ){
        result = match.captured(1).toInt( );
    }
    return result;
}

QString Util::getValuesFromParFileLine(const QString line)
{
    QString result;
    QRegularExpression re("((?:(?:-[\\d.])?[^-]*)+)(?:-?.*)");
    QRegularExpressionMatch match = re.match( line );
    if( match.hasMatch() ){
        result = match.captured(1).trimmed();
    }
    return result;
}

void Util::fixVarmapBug(const QString varmap_grid_file_path)
{
    //open a new file for output
    QFile outputFile( QString(varmap_grid_file_path).append(".new") );
    outputFile.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&outputFile);
    //open the current file for reading
    QFile inputFile( varmap_grid_file_path );
    if ( inputFile.open(QIODevice::ReadOnly | QFile::Text ) ) {
       QTextStream in(&inputFile);
       while ( !in.atEnd() ){
          QString line = in.readLine();
          //replaces sequences of asterisks with the varmap standard no-data value
          line.replace(QRegularExpression("\\*+"), Util::VARMAP_NDV);
          //writes the fixed line to the new file
          out << line << '\n';
       }
       inputFile.close();
       //closes the output file
       outputFile.close();
       //deletes current file
       inputFile.remove();
       //renames the new file
       outputFile.rename( QFile( varmap_grid_file_path ).fileName() );
    }
}

void Util::renameGEOEASvariable(const QString file_path, const QString old_name, const QString new_name)
{
    //open a new file for output
    QFile outputFile( QString(file_path).append(".new") );
    outputFile.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&outputFile);

    //open the current file for reading
    QFile inputFile( file_path );
    int n_vars = 0;
    if ( inputFile.open(QIODevice::ReadOnly | QFile::Text ) ) {
       QTextStream in(&inputFile);
       int i_line = 1;
       int var_count = 0;
       while ( !in.atEnd() ){
          QString line = in.readLine();
          //first number of second line holds the variable count
          if( i_line == 2 ){
              n_vars = Util::getFirstNumber( line );
          //try a replacement only on lines corresponding to variable names.
          } else if ( i_line > 1 && var_count < n_vars ){
              if(line.trimmed() == old_name.trimmed())
                  line = new_name;
              else
                  line = line.trimmed();
              ++var_count;
          }
          out << line << '\n';
          ++i_line;
       }
       inputFile.close();

       //closes the output file
       outputFile.close();

       //deletes current file
       inputFile.remove();

       //renames the new file
       outputFile.rename( QFile( file_path ).fileName() );
    }
}

void Util::copyFile(const QString from_path, const QString to_path)
{
    //removes destination if it exists.
    QFile destination( to_path );
    if( destination.exists() )
        if( ! destination.remove() ){
            Application::instance()->logError("Util::copyFile: old file removal failed.");
            return;
        }
    //performs the copy
    QFile origin( from_path );
    if( ! origin.copy( to_path ) )
        Application::instance()->logError("Util::copyFile: file copy failed.");
}

void Util::copyFileToDir(const QString from_path, const QString path_to_directory)
{
    //get information on the original file
    QFileInfo info_origin( from_path );
    //get file name and extension of original file
    QString origin_file_name = info_origin.fileName();
    //get destination directory
    QDir dest_dir( path_to_directory );
    //make path by joining the destination directory path and the original file name
    QString to_path = dest_dir.absoluteFilePath( origin_file_name );
    //perfoms the copy
    Util::copyFile( from_path, to_path);
}

void Util::createGEOEAScheckerboardGrid(CartesianGrid *cg, QString path)
{
    //open file for writing
    QFile file( path );
    file.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&file);

    //write out the GEO-EAS grid geader
    out << "Grid file\n";
    out << "1\n";
    out << "checkerboard pattern\n";

    //flags to help in creating a checkerboard pattern.
    bool isNXeven = ( cg->getNX() % 2 ) == 0;
    bool isNYeven = ( cg->getNY() % 2 ) == 0;

    //loop to output the binary values
    ulong count = 0;
    for( uint ir = 0; ir < cg->getNReal(); ++ir)
        for( uint iz = 0; iz < cg->getNZ(); ++iz){
            for( uint iy = 0; iy < cg->getNY(); ++iy){
                for( uint ix = 0; ix < cg->getNX(); ++ix, ++count)
                    out << (count % 2) << '\n';
                if( isNXeven )
                    ++count;
            }
            if( isNYeven )
                ++count;
        }

    //close file
    file.close();
}

bool Util::viewGrid(Attribute *variable, QWidget* parent = 0, bool modal, CategoryDefinition *cd)
{
    //get input data file
    //the parent component of an attribute is a file
    //assumes the file is a Cartesian Grid, since the user is calling pixelplt
    CartesianGrid* input_data_file = (CartesianGrid*)variable->getContainingFile();

    //loads data in file, because it's necessary.
    input_data_file->loadData();

    //get the variable index in parent data file
    uint var_index = input_data_file->getFieldGEOEASIndex( variable->getName() );

    //make plot/window title
    QString title = variable->getContainingFile()->getName();
    title.append("/");
    title.append(variable->getName());
    title.append(" grid");

    //Construct an object composition based on the parameter file template for the pixelplt program.
    GSLibParameterFile gpf( "pixelplt" );

    //Set default values so we need to change less parameters and let
    //the user change the others as one may see fit.
    gpf.setDefaultValues();

    //get the max and min of the selected variable
    double data_min = input_data_file->min( var_index-1 );
    double data_max = input_data_file->max( var_index-1 );

    //----------------set the minimum required pixelplt paramaters-----------------------
    //input data parameters: data file, trimming limits and var,weight indexes
    GSLibParInputData* par0;
    par0 = gpf.getParameter<GSLibParInputData*>(0);
    par0->_file_with_data._path = input_data_file->getPath();
    par0->_var_wgt_pairs.first()->_var_index = var_index;
    par0->_trimming_limits._min = data_min - fabs( data_min/100.0 );
    par0->_trimming_limits._max = data_max + fabs( data_max/100.0 );

    //postscript file
    gpf.getParameter<GSLibParFile*>(1)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

    //grid definition parameters
    GSLibParGrid* par3 = gpf.getParameter<GSLibParGrid*>(3);
    par3->_specs_x->getParameter<GSLibParUInt*>(0)->_value = input_data_file->getNX(); //nx
    par3->_specs_x->getParameter<GSLibParDouble*>(1)->_value = input_data_file->getX0(); //min x
    par3->_specs_x->getParameter<GSLibParDouble*>(2)->_value = input_data_file->getDX(); //cell size x
    par3->_specs_y->getParameter<GSLibParUInt*>(0)->_value = input_data_file->getNY(); //ny
    par3->_specs_y->getParameter<GSLibParDouble*>(1)->_value = input_data_file->getY0(); //min y
    par3->_specs_y->getParameter<GSLibParDouble*>(2)->_value = input_data_file->getDY(); //cell size y
    par3->_specs_z->getParameter<GSLibParUInt*>(0)->_value = input_data_file->getNZ(); //nz
    par3->_specs_z->getParameter<GSLibParDouble*>(1)->_value = input_data_file->getZ0(); //min z
    par3->_specs_z->getParameter<GSLibParDouble*>(2)->_value = input_data_file->getDZ(); //cell size z

    //plot title
    gpf.getParameter<GSLibParString*>(6)->_value = title;

    //horizontal axis (normally X) label
    gpf.getParameter<GSLibParString*>(7)->_value = "longitude";

    //vertical axis (normally Y) label
    gpf.getParameter<GSLibParString*>(8)->_value = "latitude";

    //min, max, resolution for color scale
    GSLibParMultiValuedFixed* par12 = gpf.getParameter<GSLibParMultiValuedFixed*>(12);
    par12->getParameter<GSLibParDouble*>(0)->_value = data_min;
    par12->getParameter<GSLibParDouble*>(1)->_value = data_max;
    par12->getParameter<GSLibParDouble*>(2)->_value = (data_max-data_min)/10.0;

    //enable categorical display, if a category definition is passed
    if( cd ){
        //make sure the category definition info is loaded from the file
        cd->loadTriplets();
        //set category mode on
        GSLibParOption* par11 = gpf.getParameter<GSLibParOption*>(11);
        par11->_selected_value = 1;
        //set the number of categories
        GSLibParUInt* par13 = gpf.getParameter<GSLibParUInt*>(13);
        par13->_value = cd->getCategoryCount();
        //set the category display parameters
        GSLibParRepeat* par14 = gpf.getParameter<GSLibParRepeat*>(14);
        par14->setCount( par13->_value );
        for( uint iCat = 0; iCat < par13->_value; ++iCat){
            //The category code, color code and name are in a row of three paramaters
            GSLibParMultiValuedFixed *parMV = par14->getParameter<GSLibParMultiValuedFixed*>( iCat, 0 );
            //set the category code
            GSLibParUInt* par14_0 = parMV->getParameter<GSLibParUInt*>( 0 );
            par14_0->_value = cd->getCategoryCode( iCat );
            //set the category color
            GSLibParColor* par14_1 = parMV->getParameter<GSLibParColor*>( 1 );
            par14_1->_color_code = cd->getColorCode( iCat );
            //set the category name
            GSLibParString* par14_2 = parMV->getParameter<GSLibParString*>( 2 );
            par14_2->_value = cd->getCategoryName( iCat );
        }
    }

    //----------------------------------------------------------------------------------

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpf.save( par_file_path );

    //run pixelplt program
    Application::instance()->logInfo("Starting pixelplt program...");
    GSLib::instance()->runProgram( "pixelplt", par_file_path );

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(gpf.getParameter<GSLibParFile*>(1)->_path, title, gpf, parent);
    if( modal ){
        int response = dpd->exec();
        return response == QDialog::Accepted;
    }
    dpd->show();
    return false;
}

bool Util::viewPointSet(Attribute *variable, QWidget *parent, bool modal)
{
    //get input data file
    //the parent component of an attribute is a file
    //assumes the file is a Point Set, since the user is calling locmap
    PointSet* input_data_file = (PointSet*)variable->getContainingFile();

    //loads data in file, because it's necessary.
    input_data_file->loadData();

    //get the variable index in parent data file
    uint var_index = input_data_file->getFieldGEOEASIndex( variable->getName() );

    //make plot/window title
    QString title = variable->getContainingFile()->getName();
    title.append("/");
    title.append(variable->getName());
    title.append(" map");

    //Construct an object composition based on the parameter file template for the locmap program.
    GSLibParameterFile gpf( "locmap" );

    //Set default values so we need to change less parameters and let
    //the user change the others as one may see fit.
    gpf.setDefaultValues();

    //get the max and min of the selected variable
    double data_min = input_data_file->min( var_index-1 );
    double data_max = input_data_file->max( var_index-1 );

    //----------------set the minimum required locmap paramaters-----------------------
    //input file
    gpf.getParameter<GSLibParFile*>(0)->_path = input_data_file->getPath();

    //X, Y and variable
    GSLibParMultiValuedFixed* par1 = gpf.getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = input_data_file->getXindex();
    par1->getParameter<GSLibParUInt*>(1)->_value = input_data_file->getYindex();
    par1->getParameter<GSLibParUInt*>(2)->_value = var_index;

    //trimming limits
    GSLibParMultiValuedFixed* par2 = gpf.getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParDouble*>(0)->_value = data_min - fabs( data_min/100.0 );
    par2->getParameter<GSLibParDouble*>(1)->_value = data_max + fabs( data_max/100.0 );

    //postscript file
    gpf.getParameter<GSLibParFile*>(3)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

    //X limits
    GSLibParMultiValuedFixed* par4 = gpf.getParameter<GSLibParMultiValuedFixed*>(4);
    par4->getParameter<GSLibParDouble*>(0)->_value = input_data_file->min( input_data_file->getXindex()-1 );
    par4->getParameter<GSLibParDouble*>(1)->_value = input_data_file->max( input_data_file->getXindex()-1 );

    //Y limits
    GSLibParMultiValuedFixed* par5 = gpf.getParameter<GSLibParMultiValuedFixed*>(5);
    par5->getParameter<GSLibParDouble*>(0)->_value = input_data_file->min( input_data_file->getYindex()-1 );
    par5->getParameter<GSLibParDouble*>(1)->_value = input_data_file->max( input_data_file->getYindex()-1 );

    //color scale details
    GSLibParMultiValuedFixed* par10 = gpf.getParameter<GSLibParMultiValuedFixed*>(10);
    par10->getParameter<GSLibParDouble*>(0)->_value = data_min;
    par10->getParameter<GSLibParDouble*>(1)->_value = data_max;
    par10->getParameter<GSLibParDouble*>(2)->_value = (data_max-data_min)/10.0; //color scale ticks in 10 steps

    //plot title
    gpf.getParameter<GSLibParString*>(12)->_value = title;
    //----------------------------------------------------------------------------------

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpf.save( par_file_path );

    //run locmap program
    Application::instance()->logInfo("Starting locmap program...");
    GSLib::instance()->runProgram( "locmap", par_file_path );

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(gpf.getParameter<GSLibParFile*>(3)->_path, title, gpf, parent);
    if( modal ){
        int response = dpd->exec();
        return response == QDialog::Accepted;
    }
    dpd->show();
    return false;
}

void Util::viewXPlot(Attribute *xVariable, Attribute *yVariable, QWidget *parent, Attribute *zVariable)
{
    //get the selected attributes
    Attribute* var1 = xVariable;
    Attribute* var2 = yVariable;
    Attribute* var3 = zVariable;

    //assumes their parent are a data file and they refer to the same one.
    DataFile* data_file = static_cast<DataFile*>( var1->getContainingFile() );
    //loads data in file, because it's necessary to compute some stats.
    data_file->loadData();
    //get the variables indexes in the parent data file
    uint var1_index = data_file->getFieldGEOEASIndex( var1->getName() );
    uint var2_index = data_file->getFieldGEOEASIndex( var2->getName() );
    uint var3_index = 0;
    if( var3 ){
        var3_index = data_file->getFieldGEOEASIndex( var3->getName() );
    }
    //make plot/window title
    QString title = "Crossplot ";
    title.append(data_file->getName());
    title.append(": ");
    title.append(var1->getName());
    title.append(" x ");
    title.append(var2->getName());
    if( var3 ){
        title.append(" x ");
        title.append(var3->getName());
    }
    //Construct an object composition based on the parameter file template for the scatplot program.
    GSLibParameterFile gpf( "scatplt" );
    //Set default values so we need to change less parameters and let
    //the user change the others as one may see fit.
    gpf.setDefaultValues();

    //----------------set the minimum required scatplot paramaters---------------------

    //input data parameters: data file, trimming limits and var1,var2,weight and 3rd var indexes
    GSLibParInputData* par0;
    par0 = gpf.getParameter<GSLibParInputData*>(0);
    par0->_file_with_data._path = data_file->getPath();
    par0->_var_wgt_pairs.first()->_var_index = var1_index;
    par0->_var_wgt_pairs.last()->_var_index = var2_index;
    if( var3 ){
        //scatplt expects the third variable index as the fourth value
        //instead of the usual weight for the second variable.  figures...
        par0->_var_wgt_pairs.last()->_wgt_index = var3_index;
    }

    //path to postscript file
    gpf.getParameter<GSLibParFile*>(1)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

    //min,max and scale type for X variable
    GSLibParMultiValuedFixed* par2 = gpf.getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParDouble*>(0)->_value = data_file->min( var1_index-1 );
    par2->getParameter<GSLibParDouble*>(1)->_value = data_file->max( var1_index-1 );

    //min,max and scale type for Y variable
    GSLibParMultiValuedFixed* par3 = gpf.getParameter<GSLibParMultiValuedFixed*>(3);
    par3->getParameter<GSLibParDouble*>(0)->_value = data_file->min( var2_index-1 );
    par3->getParameter<GSLibParDouble*>(1)->_value = data_file->max( var2_index-1 );

    if( var3 ){
        //min,max and scale type for 3rd variable (grayscale)
        GSLibParLimitsDouble* par6 = gpf.getParameter<GSLibParLimitsDouble*>(6);
        par6->_min = data_file->min( var3_index-1 );
        par6->_max = data_file->max( var3_index-1 );
    }

    gpf.getParameter<GSLibParString*>(7)->_value = title;
    //---------------------------------------------------------------------------------

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpf.save( par_file_path );

    //run scatplt program
    Application::instance()->logInfo("Starting scatplt program...");
    GSLib::instance()->runProgram( "scatplt", par_file_path );

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(gpf.getParameter<GSLibParFile*>(1)->_path, title, gpf, parent);
    dpd->show(); //show() makes dialog modalless
}

qint64 Util::getDirectorySize(const QString path)
{
    quint64 sizex = 0;
    QFileInfo str_info( path );
    if (str_info.isDir())
    {
        QDir dir(path);
        QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs |  QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot);
        for (int i = 0; i < list.size(); ++i){
            QFileInfo fileInfo = list.at(i);
            if(fileInfo.isDir()){
                    sizex += getDirectorySize(fileInfo.absoluteFilePath());
            }else
                sizex += fileInfo.size();
        }
    }
    return sizex;
}

void Util::clearDirectory(const QString path)
{
    QDir dir(path);
    dir.setNameFilters(QStringList() << "*.*");
    dir.setFilter(QDir::Files);
    foreach(QString dirFile, dir.entryList()) //foreach is a Qt macro
    {
        dir.remove(dirFile);
    }
}

QString Util::renameFile(const QString path, const QString newName)
{
    QFileInfo original( path );
    QString newPath = original.canonicalPath() + QDir::separator() + newName;
    QFile::rename( path, newPath );
    return newPath;
}

void Util::importUnivariateDistribution(Attribute *at, const QString path_from, QWidget *dialogs_owner)
{
    //propose a name for the file
    QString proposed_name( at->getName() );
    proposed_name = proposed_name.append("_SMOOTH_DISTR");

    //open file rename dialog
    bool ok;
    QString new_dist_model_name = QInputDialog::getText(dialogs_owner, "Name the new file",
                                             "New univariate distribution model file name:", QLineEdit::Normal,
                                             proposed_name, &ok);
    if (ok && !new_dist_model_name.isEmpty()){

        QString tmpPathOfDistr = path_from;

        //asks the user to set the roles for each of the distribution file columns.
        DistributionColumnRolesDialog dcrd( tmpPathOfDistr, dialogs_owner );
        dcrd.adjustSize();
        int result = dcrd.exec(); //show modally
        if( result == QDialog::Accepted )
            Application::instance()->getProject()->importUnivariateDistribution( tmpPathOfDistr,
                                                                                new_dist_model_name.append(".dst"),
                                                                                dcrd.getRoles() );
        else
            Application::instance()->getProject()->importUnivariateDistribution( tmpPathOfDistr,
                                                                                new_dist_model_name.append(".dst"),
                                                                                QMap<uint, Roles::DistributionColumnRole>() ); //empty list
    }
}

void Util::makeGSLibColorsList(QList<QColor> &colors)
{
    colors << Qt::red << QColor(255,165,0) << Qt::yellow << Qt::green << QColor( Qt::green ).darker();
    colors << Qt::cyan << Qt::blue <<  QColor(238,130,238) << Qt::white << Qt::black;
    colors << QColor(128,0,128) << QColor(165,42,42) << QColor(255,20,147) << QColor(50,205,50);
    colors << Qt::gray << QColor(26,26,26) << QColor(51,51,51) << QColor(77,77,77) << QColor(102,102,102);
    colors << QColor(128,128,128) << QColor(154,154,154) << QColor(179,179,179) << QColor(205,205,205) << QColor(230,230,230);
}

QIcon Util::makeGSLibColorIcon(uint color_code)
{
    //make list of GSLib colors
    QList<QColor> colors;
    Util::makeGSLibColorsList( colors );

    //make and return the icon.
    QPixmap pixmap(16,16);
    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI )
        pixmap = QPixmap(32, 32);
    pixmap.fill( colors.at( color_code - 1 ) );
    return QIcon( pixmap );
}

QColor Util::getGSLibColor(uint color_code)
{
    //make list of GSLib colors
    QList<QColor> colors;
    Util::makeGSLibColorsList( colors );

    return colors.at( color_code - 1 );
}

void Util::importSettingsFromPreviousVersion()
{
    //get the settings of this application
    //Current application name and version are globbaly set in the main() funtion in main.cpp.
    QSettings currentSettings;
    //The list of previous versions (order from latest to oldest version is advised)
    QStringList previousVersions;
    previousVersions << "1.5" << "1.4" << "1.3.1" << "1.3" << "1.2.1" << "1.2" << "1.1.0" << "1.0.1" << "1.0";
    //Iterate through the list of previous versions
    QList<QString>::iterator itVersion = previousVersions.begin();
    for(; itVersion != previousVersions.end(); ++itVersion){
        //get the settings of a previous version application
        QSettings previousSettings(APP_NAME, QString(APP_NAME) + " " + (*itVersion));
        //the screen splitter setting signals the presence of a previous version setting
        //copy settings only if there are previous version settings and no settings for this version
        if( previousSettings.contains("cmsplitter") && ! currentSettings.contains("cmsplitter") ){
            QStringList keys = previousSettings.allKeys();
            QList<QString>::iterator it = keys.begin();
            //Copy all keys/values from the previous version settings
            for(; it != keys.end(); ++it){
               currentSettings.setValue( (*it), previousSettings.value( (*it) ) );
            }
            //terminate import operation upon finding a previous version
            return;
        }
    }
}

DisplayResolution Util::getDisplayResolutionClass()
{
    QScreen *screen0 = QApplication::screens().at(0);
    qreal rDPI = (qreal)screen0->logicalDotsPerInch();
    if( rDPI < 160 ) //96dpi is about SVGA in a 15-inch screen.
        return DisplayResolution::NORMAL_DPI;
    else
        return DisplayResolution::HIGH_DPI;
}

QString Util::getLastBrowsedDirectory()
{
    QSettings settings;
    return settings.value( "LastBrowsedDir" ).toString();
}

void Util::saveLastBrowsedDirectory(QString dir_path )
{
    QSettings settings;
    settings.setValue( "LastBrowsedDir", dir_path );
}

void Util::saveLastBrowsedDirectoryOfFile(QString file_path)
{
    Util::saveLastBrowsedDirectory( QFileInfo( file_path ).dir().absolutePath() );
}

QString Util::getProgramInstallDir()
{
#ifdef Q_OS_WIN
    return QString( getenv("PROGRAMFILES") );
#else
    return QString("/usr");
#endif
}

uint Util::getHeaderLineCount( QString file_path )
{
    QFile file( file_path );
    file.open( QFile::ReadOnly | QFile::Text );
    QTextStream in(&file);
    int n_vars = 0;
    int var_count = 0;

    for (int i = 0; !in.atEnd(); ++i)
    {
       //read file line by line
       QString line = in.readLine();

       if( i == 0 ){} //first line is ignored
       else if( i == 1 ){ //second line is the number of variables
           n_vars = Util::getFirstNumber( line );
       } else if ( i > 1 && var_count < n_vars ){ //the variables names
           ++var_count;
       } else { //begin lines containing data
           file.close();
           return i;
       }
    }
    //it is not supposed to reach the end of file.
    Application::instance()->logWarn("WARNING: Util::getHeaderLineCount(): unexpected reach EOF.");
    file.close();
    return 0;
}

QString Util::getGEOEAScomment(QString file_path)
{
    QFile file( file_path );
    file.open( QFile::ReadOnly | QFile::Text );
    QTextStream in(&file);
    //the comment is the first file line (no need for loops, etc.)
    QString result = in.readLine();
    file.close();
    return result;
}
