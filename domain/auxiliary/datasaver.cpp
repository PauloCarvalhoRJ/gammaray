#include "datasaver.h"
#include "domain/application.h"
#include <QTextStream>
#include <sstream>    // std::stringstream
#include <iomanip>      // std::setprecision

DataSaver::DataSaver(std::vector<std::vector<double> > &data, std::ostringstream &out, QObject *parent) :
    QObject(parent),
    _finished( false ),
    _data(data),
    _out(out)
{
}

void DataSaver::doSave()
{
    int linesSavedSoFar = 0;
    //for each data line
    std::vector< std::vector<double> >::const_iterator itDataLine = _data.cbegin();
    for(; itDataLine != _data.cend(); ++itDataLine){
        //updates the progress
        if( ! ( linesSavedSoFar % 1000 ) ){ //update progress for each 1000 lines to not impact performance much
            emit progress( (int)(linesSavedSoFar) );
        }
        //sanity check
        if( (*itDataLine).empty() ){
            Application::instance()->logWarn("DataSaver::doSave(): Empty data record at data line " +
                                             QString::number( linesSavedSoFar ) +
                                             ". Ignoring, but this signals ill-written code that changes or creates data in "
                                             "the DataFile::_data member.");
            ++linesSavedSoFar;
            continue;
        }
        std::vector<double>::const_iterator itDataColumn = (*itDataLine).cbegin();
        //output the value in the first column
        _out << *itDataColumn;
        ++itDataColumn;
        //for each data column, from 2nd column and on.
        for(; itDataColumn != (*itDataLine).cend(); ++itDataColumn){
            _out << '\t' << *itDataColumn;
        }
        _out << std::endl; //mind the difference between QTextStream's endl and std::endl
        ++linesSavedSoFar;
    }
    _finished = true;
}
