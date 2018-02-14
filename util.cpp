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
#include <QFrame>
#include <cassert>
#include <stdint.h>
#include "exceptions/invalidgslibdatafileexception.h"
#include "domain/application.h"
#include "domain/cartesiangrid.h"
#include "domain/attribute.h"
#include "domain/project.h"
#include "domain/categorydefinition.h"
#include "domain/pointset.h"
#include "domain/variogrammodel.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparams/gslibparinputdata.h"
#include "gslib/gslib.h"
#include "dialogs/displayplotdialog.h"
#include "dialogs/distributioncolumnrolesdialog.h"
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QSettings>
#include <cmath>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkImageFFT.h>
#include <vtkImageRFFT.h>

//includes for getPhysicalRAMusage()
#ifdef Q_OS_WIN
  #include <windows.h>
  #include <psapi.h>
#include <QProgressDialog>
#endif
#ifdef Q_OS_LINUX
  #include <stdlib.h>
  #include <stdio.h>
  #include <string.h>
#include <QProgressDialog>
#endif
#ifdef Q_OS_MAC
  #include <mach/mach.h>
#endif

//TODO: define constants for other GSLib program NDVs and checke whether this is being
//      actually used.
/*static*/const QString Util::VARMAP_NDV("-999.00000");

/*static*/const long double Util::PI( 3.141592653589793238L );

/*static*/const long double Util::PI_OVER_180( Util::PI / 180.0L );

//TODO: move this to geostatsutils.h, or transfer its PI_OVER_180 constant here
#define C_180_OVER_PI (180.0 / 3.14159265)

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

QString Util::copyFileToDir(const QString from_path, const QString path_to_directory)
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
    //returns the new complete path to the copied file
    return to_path;
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

    QProgressDialog progressDialog;
    progressDialog.setRange(0,0);
    progressDialog.show();
    progressDialog.setLabelText("Creating grid...");

    //loop to output the binary values
    ulong count = 0;
    int counter = 0;
    for( uint ir = 0; ir < cg->getNReal(); ++ir)
        for( uint iz = 0; iz < cg->getNZ(); ++iz){
            for( uint iy = 0; iy < cg->getNY(); ++iy, ++counter){
                for( uint ix = 0; ix < cg->getNX(); ++ix, ++count)
                    out << (count % 2) << '\n';
                if( isNXeven )
                    ++count;
                if( ! ( counter % 1000) )
                    QCoreApplication::processEvents(); //let Qt repaint widgets
            }
            if( isNYeven )
                ++count;
        }

    //close file
    file.close();
}

void Util::createGEOEASGrid(const QString columnNameForRealPart,
                            const QString columnNameForImaginaryPart,
                            std::vector<std::complex<double> > &array, QString path)
{
    //open file for writing
    QFile file( path );
    file.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&file);

    //determine the number of columns
    int nColumns = 0;
    if( ! columnNameForRealPart.isEmpty() )
        nColumns++;
    if( ! columnNameForImaginaryPart.isEmpty() )
        nColumns++;
    if( nColumns == 0)
        return;

    //write out the GEO-EAS grid header
    out << "Grid file\n";
    out << nColumns << '\n';
    if( ! columnNameForRealPart.isEmpty() )
        out << columnNameForRealPart << '\n';
    if( ! columnNameForImaginaryPart.isEmpty() )
        out << columnNameForImaginaryPart << '\n';

    QProgressDialog progressDialog;
    progressDialog.setRange(0,0);
    progressDialog.show();
    progressDialog.setLabelText("Creating grid...");

    //loop to output the values
    std::vector< std::complex<double> >::iterator it = array.begin();
    int counter = 0;
    for( ; it != array.end(); ++it, ++counter ){
        if( ! columnNameForRealPart.isEmpty() ){
            out << (*it).real() ;
            if( ! columnNameForImaginaryPart.isEmpty() )
                out << '\t';
        }
        if( ! columnNameForImaginaryPart.isEmpty() )
            out << (*it).imag() ;
        out << '\n';
        if( ! ( counter % 1000) )
            QCoreApplication::processEvents(); //let Qt repaint widgets
    }

    //close file
    file.close();
}

void Util::createGEOEASGrid(const QString columnName, std::vector<double> &values, QString path)
{
    //open file for writing
    QFile file( path );
    file.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&file);

    //write out the GEO-EAS grid header
    out << "Grid file\n";
    out << "1\n";
    out << columnName << '\n';

    QProgressDialog progressDialog;
    progressDialog.setRange(0,0);
    progressDialog.show();
    progressDialog.setLabelText("Creating grid...");

    //loop to output the values
    std::vector< double >::iterator it = values.begin();
    int counter = 0;
    for( ; it != values.end(); ++it, ++counter ){
        out << (*it) << '\n';
        if( ! ( counter % 1000) )
            QCoreApplication::processEvents(); //let Qt repaint widgets
    }

    //close file
    file.close();
}

void Util::createGEOEASGridFile(const QString gridDescription,
                                std::vector<QString> columnNames,
                                std::vector<std::vector<double> > &array,
                                QString path)
{
    //open file for writing
    QFile file( path );
    file.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&file);

    //determine the number of columns
    int nColumns = columnNames.size();

    //write out the GEO-EAS grid header
    out << gridDescription << '\n';
    out << nColumns << '\n';
    std::vector<QString>::iterator itColNames = columnNames.begin();
    for(; itColNames != columnNames.end(); ++itColNames){
        out << *itColNames << '\n';
    }

    QProgressDialog progressDialog;
    progressDialog.setRange(0,0);
    progressDialog.show();
    progressDialog.setLabelText("Creating grid...");

    //loop to output the values
    std::vector< std::vector<double> >::iterator it = array.begin();
    int counter = 0;
    for( ; it != array.end(); ++it, ++counter ){
        std::vector<double> dataLine = *it;
        std::vector<double>::iterator itData = dataLine.begin();
        for(; itData != dataLine.end(); ++itData){
            out << *itData << '\t';  //TODO: it would be nice to not leave a useless trailing tab char
        }
        out << '\n';
        if( ! ( counter % 1000) )
            QCoreApplication::processEvents(); //let Qt repaint widgets
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
    Util::assureNonZeroWindow( par12->getParameter<GSLibParDouble*>(0)->_value,
                               par12->getParameter<GSLibParDouble*>(1)->_value,
                               data_min, data_max);
    par12->getParameter<GSLibParDouble*>(2)->_value = (par12->getParameter<GSLibParDouble*>(1)->_value -
                                                       par12->getParameter<GSLibParDouble*>(0)->_value)/10.0;

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
    Util::assureNonZeroWindow( par4->getParameter<GSLibParDouble*>(0)->_value,
                               par4->getParameter<GSLibParDouble*>(1)->_value,
                               input_data_file->min( input_data_file->getXindex()-1 ),
                               input_data_file->max( input_data_file->getXindex()-1 ));

    //Y limits
    GSLibParMultiValuedFixed* par5 = gpf.getParameter<GSLibParMultiValuedFixed*>(5);
    Util::assureNonZeroWindow( par5->getParameter<GSLibParDouble*>(0)->_value,
                               par5->getParameter<GSLibParDouble*>(1)->_value,
                               input_data_file->min( input_data_file->getYindex()-1 ),
                               input_data_file->max( input_data_file->getYindex()-1 ));

    //color scale details
    GSLibParMultiValuedFixed* par10 = gpf.getParameter<GSLibParMultiValuedFixed*>(10);
    Util::assureNonZeroWindow( par10->getParameter<GSLibParDouble*>(0)->_value,
                               par10->getParameter<GSLibParDouble*>(1)->_value,
                               data_min, data_max);
    par10->getParameter<GSLibParDouble*>(2)->_value = (par10->getParameter<GSLibParDouble*>(1)->_value -
                                                       par10->getParameter<GSLibParDouble*>(0)->_value)/10.0; //color scale ticks in 10 steps

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

QString Util::getGSLibColorName(uint color_code)
{
    switch( color_code ){
    case 1: return "red";
    case 2: return "orange";
    case 3: return "yellow";
    case 4: return "light green";
    case 5: return "green";
    case 6: return "light blue";
    case 7: return "dark blue";
    case 8: return "violet";
    case 9: return "white";
    case 10: return "black";
    case 11: return "purple";
    case 12: return "brown";
    case 13: return "pink";
    case 14: return "intermediate green";
    case 15: return "gray";
    case 16: return "gray 10%";
    case 17: return "gray 20%";
    case 18: return "gray 30%";
    case 19: return "gray 40%";
    case 20: return "gray 50%";
    case 21: return "gray 60%";
    case 22: return "gray 70%";
    case 23: return "gray 80%";
    case 24: return "gray 90%";
    default: return "invalid color code";
    }
}

void Util::importSettingsFromPreviousVersion()
{
    //get the settings of this application
    //Current application name and version are globbaly set in the main() funtion in main.cpp.
    QSettings currentSettings;
    //The list of previous versions (order from latest to oldest version is advised)
    QStringList previousVersions;
    previousVersions << "3.6.1" << "3.6" << "3.5" << "3.2" << "3.0" << "2.7.2" << "2.7.1" << "2.7" << "2.5.1"
                     << "2.5" << "2.4" << "2.3" << "2.2" << "2.1" << "2.0" << "1.7.1" << "1.7" << "1.6" << "1.5"
                     << "1.4" << "1.3.1" << "1.3" << "1.2.1" << "1.2" << "1.1.0" << "1.0.1" << "1.0";
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

QFrame *Util::createHorizontalLine()
{
    QFrame *line;
    line = new QFrame( );
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setLineWidth(1);
    return line;
}

QFrame *Util::createVerticalLine()
{
    QFrame *line;
    line = new QFrame( );
    line->setFrameShape(QFrame::VLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setLineWidth(1);
    return line;
}

bool Util::isLMC(VariogramModel *vmVar1, VariogramModel *vmVar2, VariogramModel *crossVariogram)
{
    bool result = true;

    if( !vmVar1 || !vmVar2 || !crossVariogram ){
        Application::instance()->logError("Util::isLMC(): one of the variogram models is null.  Returning false.");
        return false;
    }

    VariogramModel* vmodels[] = { vmVar1, vmVar2, crossVariogram };

    //Do not allow power model.
    for( uint j = 0; j < 3; ++j)
        for( uint i = 0; i < vmodels[j]->getNst(); ++i){
            if( vmodels[j]->getIt( i ) == VariogramStructureType::POWER_LAW ){
                result = false;
                Application::instance()->logError( vmodels[j]->getName() + " has a power model, which is not allowed." );
            }
        }

    //all the models must have the same structures
    bool sameNumberOfStructures = true;
    for( uint j = 0; j < 2; ++j)
        for( uint i = j+1; i < 3; ++i)
            if( vmodels[j]->getNst() != vmodels[i]->getNst() ){
                sameNumberOfStructures = false;
                result = false;
                Application::instance()->logError( vmodels[j]->getName() + " has a different number of structures than " +
                                                   vmodels[i]->getName() + "." );
            } else {
                for( uint k = 0; k < vmodels[j]->getNst(); ++k){
                    if( vmodels[j]->getIt( k ) != vmodels[i]->getIt( k ) ){
                        result = false;
                        Application::instance()->logError( vmodels[j]->getName() + " has a different set of structures than " +
                                                           vmodels[i]->getName() + "." );
                    }
                    if( vmodels[j]->getAzimuth( k ) != vmodels[i]->getAzimuth( k ) ||
                        vmodels[j]->getDip( k ) != vmodels[i]->getDip( k ) ||
                        vmodels[j]->getRoll( k ) != vmodels[i]->getRoll( k )){
                        result = false;
                        Application::instance()->logError( vmodels[j]->getName() + " has a different set of angles in structure " +
                                                           QString::number(k+1) + " than " + vmodels[i]->getName() + "." );
                    }
                    if( vmodels[j]->get_a_hMax( k ) != vmodels[i]->get_a_hMax( k ) ||
                        vmodels[j]->get_a_hMin( k ) != vmodels[i]->get_a_hMin( k ) ||
                        vmodels[j]->get_a_vert( k ) != vmodels[i]->get_a_vert( k )){
                        result = false;
                        Application::instance()->logError( vmodels[j]->getName() + " has a different set of ranges in structure " +
                                                           QString::number(k+1) + " than " + vmodels[i]->getName() + "." );
                    }
                }
            }

    //the contributions of the autovariograms must be positive
    for( uint j = 0; j < 2; ++j){
        if( vmodels[j]->getNugget() <= 0.0 ){
            result = false;
            Application::instance()->logError( vmodels[j]->getName() + " autovariogram has a non-positive nugget effect." );
        }
        for( uint k = 0; k < vmodels[j]->getNst(); ++k){
            if( vmodels[j]->getCC( k ) <= 0.0 ){
                result = false;
                Application::instance()->logError( vmodels[j]->getName() + " autovariogram has a non-positive contribution." );
            }
        }
    }

    //the product of the contributions of the autovariograms must be grater than the square of the contributions of the
    //cross variogram.
    if( sameNumberOfStructures ){
        if( vmodels[0]->getNugget() * vmodels[1]->getNugget() <=
            vmodels[2]->getNugget() * vmodels[2]->getNugget() ){
            result = false;
            Application::instance()->logError( "The product of the nugget effects (" + QString::number(vmodels[0]->getNugget()) + " and " +
                    QString::number(vmodels[1]->getNugget()) + ") of the autovariograms must be " +
                    "grater than the square of the nugget effect (" + QString::number(vmodels[2]->getNugget()) +
                    ") of the cross variogram." );
        }
        for( uint k = 0; k < vmodels[0]->getNst(); ++k){
            if( vmodels[0]->getCC( k ) * vmodels[1]->getCC( k ) <=
                vmodels[2]->getCC( k ) * vmodels[2]->getCC( k ) ){
                result = false;
                Application::instance()->logError( "The product of the contributions (" + QString::number(vmodels[0]->getCC( k )) + " and " +
                        QString::number(vmodels[1]->getCC( k )) + ") of the autovariograms in structure " + QString::number(k+1) + " must be " +
                        "grater than the square of the contribution (" + QString::number(vmodels[2]->getCC( k )) +
                        ") of the cross variogram." );
            }
        }
    }

    return result;
}

void Util::saveText(const QString filePath, const QStringList lines)
{
    //open a new file for output
    QFile outputFile( filePath );
    outputFile.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&outputFile);

    QStringList::const_iterator it = lines.begin();
    for ( ; it != lines.end(); ++it ){
        QString line = *it;
        out << line << "\n";
    }

    //closes the output file
    outputFile.close();
}

void Util::fft1D(int lx, std::vector< std::complex<double> > &cx, int startingElement, FFTComputationMode isig )
{
    int i, j, l, m, istep;
    std::complex<double> carg, /*cexp,*/ cw, ctemp;
    double pii, sc;
    pii = 4.*std::atan(1.); //c++ has pi defined as constant

    int iisig = 0;
    switch( isig ){
        case FFTComputationMode::DIRECT: iisig = 0; break;
        case FFTComputationMode::REVERSE: iisig = 1;
    }

    j = 1;
    sc = std::sqrt(1./lx);
    for( i = 1; i <= lx; ++i){
        if(i <= j){
            int pi = startingElement + i;
            int pj = startingElement + j;
            ctemp = cx[pj-1]*sc;
            cx[pj-1] = cx[pi-1]*sc;
            cx[pi-1] = ctemp;
        }
        m = lx/2;
        while (m >= 1 && j > m){
            j = j-m;
            m = m/2;
        }
        j = j + m;
    }

    l = 1;

    std::complex<double> im(0, 1); //Fortran's cmplx(0.,1.) == i

    while ( l < lx ){
        istep = 2*l;
        for( m = 1; m <= l; ++m ){
            carg = im *(pii*iisig*(m-1))/(double)(l);
            cw = std::exp(carg);
            for( i = m; i+l < lx; i = i + istep ){
                int pi = startingElement + i;
                if( pi+l-1 < 0 || pi+l-1 >= (int)cx.size() || pi-1 < 0 || pi-1 >= (int)cx.size()){
                    Application::instance()->logError("Util::fft1D: Index out of bounds.  Computation not done.");
                    continue;
                }
                ctemp = (cw*cx[pi+l-1]);
                cx[pi+l-1] = (cx[pi-1]-ctemp);
                cx[pi-1] = (cx[pi-1]+ctemp);
            }
        }
        l = istep;
    }

    /////////////////original code in Fortran/////////////////////////////
 /* !----------------------------------------------------------------------
    ! 1-D Fast Fourier Transform (FFT) by Jon Claerbout (1985)
    !----------------------------------------------------------------------
    ! Input and output parameters:
    !
    !   LX=   dimension of the data array (integer)
    !   CX=   data array (complex, dim=LX) with input values in real part
    !   ISIG= integer flag = 0 for forward FFT and = 1 for inverse FFT
    !
    ! Note that this soubroutine deletes the original data in the CX array
    ! and after this subroutine the data array needs to be swapped around
    ! the origin (see Press et al. Numerical recipes, for example).
    !
    ! Original reference: Claerbout, J., 1976. Fundamentals of geophysical
    ! data processing. With applications to petroleum prospecting: McGraw-
    ! Hill Book Co.
    !----------------------------------------------------------------------
    ! Markku Pirttijärvi, 2003

      subroutine fork(lx,cx,isig)

        implicit none
        integer :: lx,isig,i,j,l,m,istep
        complex :: cx(lx),carg,cexp,cw,ctemp
        real :: pii,sc
        pii= 4.*atan(1.)

        j= 1
        sc= sqrt(1./lx)
        do i= 1,lx
          if(i <= j) then
            ctemp= cx(j)*sc
            cx(j)= cx(i)*sc
            cx(i)= ctemp
          end if
          m= lx/2
          do while (m >= 1 .and. j > m)
            j= j-m
            m= m/2
          end do
          j= j+m
        end do
        l= 1
        do while (l < lx)
          istep= 2*l
          do m= 1,l
            carg= cmplx(0.,1.)*(pii*isig*(m-1))/l
            cw= cexp(carg)
            do i= m,lx,istep
              ctemp= cw*cx(i+l)
              cx(i+l)= cx(i)-ctemp
              cx(i)= cx(i)+ctemp
            end do
          end do
          l= istep
        end do
        return
      end subroutine fork
*/
}

void Util::fft1DPPP(int dir, long m, std::vector<std::complex<double> > &x, long startingElement)
{
    long i, i1, i2,j, k, l, l1, l2, n;
    std::complex <double> t1, u, c;

    /*Calculate the number of points */
    n = 1;
    for(i = 0; i < m; i++)
        n <<= 1;

    /* Do the bit reversal */
    i2 = n >> 1;
    j = 0;

    for (i = 0; i < n-1 ; i++)
    {
        if (i < j)
            std::swap(x[i+startingElement],
                      x[j+startingElement]);

        k = i2;

        while (k <= j)
        {
            j -= k;
            k >>= 1;
        }

        j += k;
    }

    /* Compute the FFT */
    c.real(-1.0);
    c.imag(0.0);
    l2 = 1;
    for (l = 0; l < m; l++)
    {
        l1 = l2;
        l2 <<= 1;
        u.real(1.0);
        u.imag(0.0);

        for (j = 0; j < l1; j++)
        {
            for (i = j; i < n; i += l2)
            {
                i1 = i + l1;
                t1 = u * x[i1+startingElement];
                x[i1+startingElement] = x[i+startingElement] - t1;
                x[i+startingElement] += t1;
            }

            u = u * c;
        }

        c.imag(std::sqrt((1.0 - c.real()) / 2.0));
        if (dir == 1)
            c.imag(-c.imag());
        c.real(std::sqrt((1.0 + c.real()) / 2.0));
    }

    /* Scaling for forward transform */
    if (dir == 1)
    {
        for (i = 0; i < n; i++)
            x[i+startingElement] /= n;
    }
    return;
}


void Util::fft2D(int n1, int n2, std::vector< std::complex<double> > &cp, FFTComputationMode isig)
{
    int i1,i2;
    std::vector< std::complex<double> > cw( n2 );

    for( i2 = 0; i2 < n2; ++i2){
        fft1D(n1, cp, i2*n1, isig); //cp[i2*n1] is supposed to mean cp[0][i2] (Fortran: cp(1,i2))
        //fft1DPPP( 1, (long)std::log2(n2), cp, i2*n1);
    }

    for( i1 = 0; i1 < n1; ++i1){
        for( i2 = 0; i2 < n2; ++i2) {
            cw[i2] = cp[i1+i2*n1];       //cp[i1+i2*n1] is supposed to mean cp[i1][i2] (Fortran: cp(i1,i2))
        }
        fft1D(n2, cw, 0, isig);
        //fft1DPPP( 1, (long)std::log2(n2), cw, 0);
        for( i2 = 0; i2 < n2; ++i2){
            cp[i1+i2*n1] = cw[i2];       //cp[i1+i2*n1] is supposed to mean cp[i1][i2] (Fortran: cp(i1,i2))
        }
    }
    //////////////////////original code in Fortran//////////////////////
/*  !----------------------------------------------------------------------
    ! Computation of 2-D Fast Fourier Transform (FFT)
    !---------------------------------------------------------------------
    ! Input and output parameters:
    !
    ! N1=   dimension of the data array in x direction (integer)
    ! N2=   dimension of the data array in y direction (integer)
    ! CP=   data array (complex, dim=(N1,N2)) with input values in real part
    ! ISIG= computation flag (int.) = 0 for forward and =1 for inverse FFT
    !
    ! Calls external subroutine FORK (for 1D FFT) to do the actual work.
    !---------------------------------------------------------------------
    ! M.Pirttijärvi, October, 2003

      subroutine ft2d(n1,n2,cp,isig)
        implicit none
        integer :: n1,n2,isig,i1,i2
        complex :: cp(n1,n2),cw(n2)
        external fork

        do i2= 1,n2
          call fork(n1,cp(1,i2),isig)
        end do

        do i1= 1,n1
          do i2= 1,n2
            cw(i2)= cp(i1,i2)
          end do
          call fork(n2,cw,isig)
          do i2= 1,n2
            cp(i1,i2)= cw(i2)
          end do
        end do

        return
      end subroutine ft2d
*/
}

QStringList Util::fastSplit(const QString lineGEOEAS)
{
    QStringList result;
    char token[100]; //100 characters is more than enough for a double in a text file.
    int iTokenChar = 0;
    int nchar = lineGEOEAS.length();
    char currentChar;

    //for each char of the line
    for( int i = 0; i < nchar; ++i){
        currentChar = lineGEOEAS[i].toLatin1(); //assumes no fancy unicode chars are in GEO-EAS data sets
        switch(currentChar){
            case '-': case '.': case '0': case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9': case 'E': case 'e': case '+':
                token[ iTokenChar++ ] = currentChar;
                break;
            default:  //found a separator char (could be anything other than valid number characters)
                token[ iTokenChar ] = 0; //append null char
                if( iTokenChar > 0 ) //if token is not empty
                    result.push_back( token ); //adds the token to the string list
                iTokenChar = 0; //resets the token char counter
        }
    }

    //it is possible that the last token finishes the line
    if( iTokenChar > 0 ){ //if token is not empty
        token[ iTokenChar ] = 0; //append null char
        result.push_back( token ); //adds the token to the string list
    }

    return result;
}

void Util::fft3D(int nI, int nJ, int nK, std::vector<std::complex<double> > &values,
                 FFTComputationMode isig,
                 FFTImageType itype )
{
    //create a vtk image from the input value array
    ////// index_shift = ( index + nINDEX/2) % nINDEX), if in reverse FFT mode,
    ////// shifts the lower frequencies components to the corners of the image for compatibility with RFFT algorithm/////
    vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
    image->SetDimensions( nI, nJ, nK );
    image->AllocateScalars( VTK_DOUBLE, 2 ); // two components: real and imaginary parts.
    for(unsigned int k = 0; k < (unsigned int)nK; ++k) {
        int k_shift = (k + nK/2) % nK;
        if( isig == FFTComputationMode::DIRECT ) k_shift = k;
        for(unsigned int j = 0; j < (unsigned int)nJ; ++j){
            int j_shift = (j + nJ/2) % nJ;
            if( isig == FFTComputationMode::DIRECT ) j_shift = j;
            for(unsigned int i = 0; i < (unsigned int)nI; ++i){
                int i_shift = (i + nI/2) % nI;
                if( isig == FFTComputationMode::DIRECT ) i_shift = i;
                std::complex<double> value = values[i_shift + j_shift*nI + k_shift*nJ*nI];
                double* cell = static_cast<double*>(image->GetScalarPointer(i,j,k));
                if( isig == FFTComputationMode::REVERSE && itype == FFTImageType::POLAR_FORM )
                    value = std::polar( value.real(), value.imag() );
                cell[0] = value.real();
                cell[1] = value.imag();
            }
        }
    }
    image->Modified();

    vtkSmartPointer<vtkImageFourierFilter> fftFilter;
    if( isig == FFTComputationMode::DIRECT )
        // Compute the FFT of the image
        fftFilter = vtkSmartPointer<vtkImageFFT>::New();
    else // FFTComputationMode::REVERSE
        // Compute the reverse FFT of the image
        fftFilter = vtkSmartPointer<vtkImageRFFT>::New();
    //fftFilter->SetInputConnection( image->GetData() );
    fftFilter->SetInputData( image );
    fftFilter->Update();

    //return the result image in frequency/real domain (polar/rectangular form)
    ////// index_shift = ( index + nINDEX/2) % nINDEX), if in forward FFT mode,
    ////// shifts the lower frequencies components to the center of the image for ease of interpretation/////
    vtkSmartPointer<vtkImageData> imageOutput = fftFilter->GetOutput();
    for(unsigned int k = 0; k < (unsigned int)nK; ++k) {
        int k_shift = (k + nK/2) % nK;
        if( isig == FFTComputationMode::REVERSE ) k_shift = k;
        for(unsigned int j = 0; j < (unsigned int)nJ; ++j){
            int j_shift = (j + nJ/2) % nJ;
            if( isig == FFTComputationMode::REVERSE ) j_shift = j;
            for(unsigned int i = 0; i < (unsigned int)nI; ++i){
                int i_shift = (i + nI/2) % nI;
                if( isig == FFTComputationMode::REVERSE ) i_shift = i;
                std::complex<double> value;
                double* cell = static_cast<double*>(imageOutput->GetScalarPointer(i,j,k));
                if( isig == FFTComputationMode::DIRECT && itype == FFTImageType::POLAR_FORM ){
                    std::complex<double> tmp( cell[0], cell[1] );
                    value.real( std::abs( tmp ) );
                    value.imag( std::arg( tmp ) );
                } else {
                    value.real( cell[0] );
                    value.imag( cell[1] );
                }
                values[i_shift + j_shift*nI + k_shift*nJ*nI] = value;
            }
        }
    }
}

double Util::getDip( double dx, double dy, double dz, int xstep, int ystep, int zstep )
{
    double xlag = xstep * dx;
    double ylag = ystep * dy;
    double xylag = sqrt( xlag*xlag + ylag*ylag );
    double zlag = zstep * dz;
    //refer to gam program instructions for cell steps equivalency to angles.
    double dip = 0.0; //dip defaults to zero
    if( xstep == 0 && ystep == 0) //dip along z axis
    {
        if( zstep < 0 ) dip = 90.0; else dip = -90.0;
    }
    else
        dip = -std::atan( zlag / xylag ) * C_180_OVER_PI;
    return dip;
}

double Util::getAzimuth(double dx, double dy, int xstep, int ystep)
{
    //refer to gam program instructions for cell steps equivalency to angles.
    double azimuth = 0.0; //azimuth defaults to zero
    if( xstep == 0 ) //azimuth along x axis
    {
        if( ystep < 0 ) azimuth = 180.0; else azimuth = 0.0;
    }
    else if( ystep == 0 ) //azimuth along y axis
    {
        if( xstep < 0 ) azimuth = 270.0; else azimuth = 90.0;
    }
    else if( xstep > 0 && ystep > 0 ) //azimuth in 1st quadrant
        azimuth = atan( xstep*dx / ystep*dy ) * C_180_OVER_PI;
    else if( xstep > 0 && ystep < 0 ) //azimuth in 2nd quadrant
        azimuth = 180.0 + atan( xstep*dx / ystep*dy ) * C_180_OVER_PI;
    else if( xstep < 0 && ystep < 0 ) //azimuth in 3rd quadrant
        azimuth = 180.0 + atan( xstep*dx / ystep*dy ) * C_180_OVER_PI;
    else if( xstep < 0 && ystep > 0 ) //azimuth in 4th quadrant
        azimuth = 360.0 + atan( xstep*dx / ystep*dy ) * C_180_OVER_PI;
    return azimuth;
}

double Util::dB(double value, double refLevel, double epsilon)
{
    double absValue = std::abs<double>( value );
    double valueToUse = value;
    if( absValue < epsilon ){
        if( value < 0.0 )
            valueToUse = -epsilon;
        else
            valueToUse = epsilon;
    }
    return DECIBEL_SCALE_FACTOR * std::log10<double>( valueToUse / refLevel ).real();
}

QString Util::humanReadable(double value)
{
    //buffer string for formatting the output (QString's sptrintf doesn't honor field size)
    char buffer[50];
    //define base unit to change suffix (could be 1024 for ISO bytes (iB), for instance)
    double unit = 1000.0d;
    //return the plain value if it doesn't require a multiplier suffix (small values)
    if (value <= unit){
        std::sprintf(buffer, "%.1f", value);
        return QString( buffer );
    }
    //compute the order of magnitude (approx. power of 1000) of the value
    int exp = (int) (std::log10<double>(value).real() / std::log10<double>(unit).real());
    //string that is a list of available multiplier suffixes
    QString suffixes = "pnum kMGTPE";
    //select the suffix
    char suffix = suffixes.at( 5+exp-1 ).toLatin1(); //-5 because pico would result in a -5 index.
    //format output, dividing the value by the power of 1000 found
    std::sprintf(buffer, "%.1f%c", value / std::pow<double, int>(unit, exp), suffix);
    return QString( buffer );
}

bool Util::isWithinBBox(double x, double y, double minX, double minY, double maxX, double maxY)
{
    if( x < minX ) return false;
    if( x > maxX ) return false;
    if( y < minY ) return false;
    if( y > maxY ) return false;
    return true;
}

void Util::assureNonZeroWindow(double &outMin,
                               double &outMax,
                               double inMin,
                               double inMax,
                               double minWindowPercent)
{
    if( Util::almostEqual2sComplement( inMin, inMax, 1 ) ){
        outMin = inMin - fabs( inMin * minWindowPercent );
        outMax = inMax + fabs( inMax * minWindowPercent );
    } else {
        outMin = inMin;
        outMax = inMax;
    }
}

bool Util::viewHistogram(Attribute *at, QWidget *parent, bool modal)
{
    //get input data file
    //the parent component of an attribute is a file
    DataFile* input_data_file = (DataFile*)at->getContainingFile();

    //load file data.
    input_data_file->loadData();

    //get the variable index in parent data file
    uint var_index = input_data_file->getFieldGEOEASIndex( at->getName() );

    //make plot/window title
    QString title = at->getContainingFile()->getName();
    title.append("/");
    title.append(at->getName());
    title.append(" histogram");

    //Construct an object composition based on the parameter file template for the hisplt program.
    GSLibParameterFile gpf( "histplt" );

    //Set default values so we need to change less parameters and let
    //the user change the others as one may see fit.
    gpf.setDefaultValues();

    //get the maximum and minimun of selected variable, excluding no-data value
    double data_min = input_data_file->min( var_index-1 );
    double data_max = input_data_file->max( var_index-1 );

    //----------------set the minimum required histplt paramaters-----------------------
    //input parameters (input file, variable and trimming limits)
    GSLibParInputData* par0;
    par0 = gpf.getParameter<GSLibParInputData*>(0);
    par0->_file_with_data._path = input_data_file->getPath();
    par0->_var_wgt_pairs.first()->_var_index = var_index;
    par0->_trimming_limits._min = data_min - fabs( data_min/100.0 );
    par0->_trimming_limits._max = data_max + fabs( data_max/100.0 );

    //postscript file
    gpf.getParameter<GSLibParFile*>(1)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

    //if min == max, then the default (0,-1) to get min/max from data causes the plot to fail
    if( Util::almostEqual2sComplement( data_min, data_max, 1 ) ){
        //set actual limits
        GSLibParMultiValuedFixed* par2;
        par2 = gpf.getParameter<GSLibParMultiValuedFixed*>(2);
        Util::assureNonZeroWindow( par2->getParameter<GSLibParDouble*>(0)->_value,
                                   par2->getParameter<GSLibParDouble*>(1)->_value,
                                   data_min,
                                   data_max);
    }

    //plot title
    gpf.getParameter<GSLibParString*>(9)->_value = title;

    //reference value for the box plot
    gpf.getParameter<GSLibParDouble*>(11)->_value = input_data_file->mean( var_index-1 );
    //----------------------------------------------------------------------------------

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpf.save( par_file_path );

    //run histplt program
    Application::instance()->logInfo("Starting histplt program...");
    GSLib::instance()->runProgram( "histplt", par_file_path );

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(gpf.getParameter<GSLibParFile*>(1)->_path, title, gpf, parent);
    if( modal ){
        int response = dpd->exec();
        return response == QDialog::Accepted;
    }
    dpd->show();
    return false;
}

bool Util::programWasCalledWithCommandLineArgument(QString argument)
{
    QStringList arguments = QCoreApplication::arguments();
    return arguments.contains(argument);
}

//This .cpp internal function is used by Util::getRAMusage()
int parseLine(char* line){
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
}

std::int64_t Util::getPhysicalRAMusage()
{
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    //SIZE_T virtualMemUsedByMe = pmc.PrivateUsage;
    SIZE_T physMemUsedByMe = pmc.WorkingSetSize;
    return (std::int64_t)physMemUsedByMe;
#endif
#ifdef Q_OS_LINUX
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];
    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return (std::int64_t)result * 1024; //value in kB
#endif
#ifdef Q_OS_MAC
    //TODO: untested code.
    struct task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

    if (KERN_SUCCESS != task_info(mach_task_self(),
                                  TASK_BASIC_INFO, (task_info_t)&t_info,
                                  &t_info_count))
    {
        return (std::int64_t)-1;
    } else {
        return (std::int64_t)t_info.resident_size;
    }
#else
    return (std::int64_t)-1;
#endif
}

QString Util::getFileName(QString path)
{
    QFileInfo fileinfo( path );
    return fileinfo.fileName();
}
