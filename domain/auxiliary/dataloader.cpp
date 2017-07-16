#include "dataloader.h"
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
    QStringList list;
    int n_vars = 0;
    int var_count = 0;
    QTextStream in(&_file);
    long bytesReadSofar = 0;

    for (int i = 0; !in.atEnd(); ++i)
    {
       //read file line by line
       QString line = in.readLine();

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
           list << line;
           ++var_count;
       } else if( _data_line_count >= _firstDataLineToRead &&
                  _data_line_count <= _lastDataLineToRead ) { //parse lines containing data (must be within the target interval)
           std::vector<double> data_line;
           //QStringList values = line.split(QRegularExpression("\\s+"), QString::SkipEmptyParts); //this is a bottleneck
           QStringList values = Util::fastSplit( line );
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
               _data.push_back( std::move( data_line ) );
               ++_data_line_count;
           }
       } else { //if the data line is not within the target interval
           ++_data_line_count; //just count it as parsed.
       }
    }
    _finished = true;
}
