#include "dataloader.h"
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include "util.h"
#include "../application.h"


DataLoader::DataLoader(QFile &file,
                       std::vector<std::vector<double> > &data,
                       uint &data_line_count,
                       ulong firstDataLineToRead,
                       ulong lastDataLineToRead,
                       QObject *parent) :
    QObject(parent),
    _file(file),
    _data(data),
    _data_line_count(data_line_count),
    _finished(false),
    _firstDataLineToRead( firstDataLineToRead ),
    _lastDataLineToRead( lastDataLineToRead )
{
}

void DataLoader::doLoad() { /* do what you need and emit progress signal */
	//QStringList list;
    int n_vars = 0;
    int var_count = 0;
    QTextStream in(&_file);
    long bytesReadSofar = 0;
	QStringList valuesAsString;

	//Get data file size in bytes.
	QFileInfo fileInfo( _file );
	uint64_t fileSize = fileInfo.size();

	uint iDataLineParsed = 0;

	uint nPushesBack = 0;

	uint nPopsBack = 0;

	for (int i = 0; !in.atEnd(); ++i)
    {
       //read file line by line
       QString line = in.readLine();

	   //Clear the list with the tokenized data values as string
	   valuesAsString.clear();

	   //updates the progress
	   bytesReadSofar += line.size() + 1; //account for line break char (in Windows may have 2, though.)

	   if( ! ( i % 100 ) ){ //update progress for each 100 lines to not impact performance much
		   // allows tracking progress of a file up to about 400GB
		   emit progress( (int)(bytesReadSofar / 100) );
	   }

	   //TODO: second line may contain other information in grid files, so it will fail for such cases.
	   if( i == 0 ){} //first line is ignored
	   else if( i == 1 ){ //second line is the number of variables
		   n_vars = Util::getFirstNumber( line );
	   } else if ( i > 1 && var_count < n_vars ){ //the variables names
		   //list << line;
		   ++var_count;
	   } else if( _data_line_count >= _firstDataLineToRead &&
				  _data_line_count <= _lastDataLineToRead ) { //parse lines containing data (must be within the target interval)
		   //read line from the GSLib data file
		   Util::fastSplit( line, valuesAsString );
		   //if we are at the first data line to read...
		   if( _data_line_count == _firstDataLineToRead ){
			   //...estimate the number of lines in the file
			   uint nLinesExpectation = fileSize / ( line.size() + 2 ) + 200; //+2 -> line breaks in Windows; +200 -> make room for possible line length variation
			   //the number of data lines to store should be smaller or equal than the file line window
			   if( nLinesExpectation > ( _lastDataLineToRead - _firstDataLineToRead ) )
				   nLinesExpectation =  _lastDataLineToRead - _firstDataLineToRead;
			   //now we can reserve capacity in the _data array to avoid minimize push_back calls and vector re-allocations/copies
			   _data = std::vector<std::vector<double> >( 1.1 * nLinesExpectation, std::vector<double>( n_vars, -424242.0 ) );
		   }
		   if( valuesAsString.size() != n_vars ){
			   Application::instance()->logError( QString("ERROR: wrong number of values in line ").append(QString::number(i)) );
			   Application::instance()->logError( QString("       expected: ").append(QString::number(n_vars)).append(", found:").append(QString::number(valuesAsString.size())) );
			   for( QStringList::Iterator it = valuesAsString.begin(); it != valuesAsString.end(); ++it ){
				   Application::instance()->logInfo((*it));
			   }
		   } else {
			   //read each value along the line
			   QStringList::Iterator it = valuesAsString.begin();
			   for( int j = 0; it != valuesAsString.end(); ++it, ++j ){
				   //parse the double value
				   bool ok = true;
				   double value = (*it).toDouble( &ok );
				   if( !ok ){
					   Application::instance()->logError( QString("DataLoader::doLoad(): error in data file (line ").append(QString::number(i)).append("): cannot convert ").append( *it ).append(" to double.") );
				   }
				   //making sure there is room for the new data.
				   if( iDataLineParsed == _data.size() ){
					   ++nPushesBack;
					   _data.push_back( std::vector<double>( n_vars, -424242.0 ) );
				   }
				   //store the value in the data array.
				   _data[iDataLineParsed][j] = value;
			   }
			   ++_data_line_count;
			   ++iDataLineParsed;
		   }
	   } else { //if the data line is not within the target interval (window in data file)
		   ++_data_line_count; //just count it as parsed.
	   }
    }

	//remove possibly excess of data in pre-allocation of _data
	while( iDataLineParsed < _data.size() ){
		++nPopsBack;
		_data.pop_back();
	}

	if( nPushesBack )
		Application::instance()->logInfo( QString("DataLoader::doLoad(): data array adjustment resulted in ").append(QString::number(nPushesBack)).append(" push(es)-back.") );

	if( nPopsBack )
		Application::instance()->logInfo( QString("DataLoader::doLoad(): data array adjustment resulted in ").append(QString::number(nPopsBack)).append(" pop(s)-back.") );

    _finished = true;
}
