#include "variableremover.h"
#include "../datafile.h"
#include "util.h"
#include <QFile>
#include <QTextStream>

VariableRemover::VariableRemover(DataFile& dataFile, uint columnToDelete, QObject *parent) :
    QObject(parent),
    _finished(false),
    _dataFile(dataFile),
    _columnToDelete(columnToDelete)
{
}

void VariableRemover::doRemove()
{
    //get the current file path (this file)
    QString file_path = _dataFile.getPath();

    //create a new file for output
    QFile outputFile( QString(file_path).append(".new") );
    outputFile.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&outputFile);

    //open the current file for reading
    QFile inputFile( file_path );
    if ( inputFile.open(QIODevice::ReadOnly | QFile::Text ) ) {
       QTextStream in(&inputFile);
       uint line_index = 0;
       uint n_vars = 0;
       uint var_count = 0;
       long bytesPassedSoFar = 0;
        //for each line in the current file...
        while ( !in.atEnd() ){
            //...read its line
           QString line = in.readLine();
           //counts the bytes passed so far
           bytesPassedSoFar += line.size() + 1; //account for line break char (in Windows may have 2, though.)
            //update progress for each 100 lines to not impact performance much
           if( ! ( line_index % 100 ) ){
               // allows tracking progress of a file up to about 400GB
               emit progress( (int)(bytesPassedSoFar / 100) );
           }
           //simply copy the first line (title)
           if( line_index == 0 ){
               out << line << '\n';
           //first number of second line holds the variable count
           //writes an decreased number of variables.
           //TODO: try to keep the rest of the second line (not critical, but desirable)
           } else if( line_index == 1 ) {
               n_vars = Util::getFirstNumber( line );
               out << ( n_vars-1 ) << '\n';
           //simply copy the current variable names, except the one targeted for deletion
           } else if ( var_count < n_vars ) {
               if( var_count != _columnToDelete )
                 out << line << '\n';
               ++var_count;
           //treat the data lines until EOF
           } else {
               //tokenize the data record
               QStringList values = Util::fastSplit( line );
               //remove the value corresponding to the column to be removed
               values.removeAt( _columnToDelete );
               //output the remaining values
               out << values.join("\t") << '\n';
           } //if's and else's for each file line case (header, var. count, var. name and data line)
           //keep count of the source file lines
           ++line_index;
        } // for each line in destination file (this)
    }

    //close the current file
    inputFile.close();
    //close the newly created file without the target column
    outputFile.close();
    //deletes the current file
    inputFile.remove();
    //renames the new file, effectively replacing the destination file.
    outputFile.rename( QFile( file_path ).fileName() );
    _finished = true;
}
