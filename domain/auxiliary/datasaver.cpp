#include "datasaver.h"
#include <QTextStream>
#include <sstream>    // std::stringstream
#include <iomanip>      // std::setprecision

DataSaver::DataSaver(std::vector<std::vector<double> > &data, QTextStream &out, QObject *parent) :
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
    std::vector< std::vector<double> >::iterator itDataLine = _data.begin();
    for(; itDataLine != _data.end(); ++itDataLine){
        //updates the progress
        if( ! ( linesSavedSoFar % 1000 ) ){ //update progress for each 1000 lines to not impact performance much
            emit progress( (int)(linesSavedSoFar) );
        }
        //for each data column
        std::vector<double>::iterator itDataColumn = (*itDataLine).begin();
        _out << *itDataColumn;
        ++itDataColumn;
        for(; itDataColumn != (*itDataLine).end(); ++itDataColumn){
            //making sure the values are written in GSLib-like precision
            std::stringstream ss;
            ss << std::setprecision( 12 /*std::numeric_limits<double>::max_digits10*/ );
            ss << *itDataColumn;
            _out << '\t' << ss.str().c_str();
        }
        _out << endl; //this must be QTextStream's endl, not std::endl
        ++linesSavedSoFar;
    }
    _finished = true;
}
