#ifndef VALUEPAIRS_H
#define VALUEPAIRS_H

#include "file.h"
#include <QPair>
#include <QList>
#include <QFile>
#include <QTextStream>

/** The generic template for a string-to-number convert function.
 *  See the avaliable specializations in the cpp source file.  Add more as needed for
 *  numeric types other than int, float and double.*/
namespace valueconverter {
    template <typename T> T convert( QString value );
}

/** This abstract generic class represents a file that contains pairs of values
 *  (can be of integer/integer, double/float, integer/float, etc.).
 *  Each pair is written as line files and each value is separated by a tab character.
 */
template <class T1, class T2>
class ValuePairs : public File
{
public:
    ValuePairs(QString path) : File( path ){}

    int getPairCount(){ return m_pairs.count(); }
    T1 get1stValue( int pair_index ){ return m_pairs.at(pair_index).first; }
    T2 get2ndValue( int pair_index ){ return m_pairs.at(pair_index).second; }
    void addPair( T1 first, T2 second ){ m_pairs.append( QPair<T1,T2>( first, second ) ); }
    void clear(){ m_pairs.clear(); }

    /** Writes the value pairs as lines in a text file. */
    void savePairs(){
        QFile file( _path );
        file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&file);
        typename QList< QPair< T1, T2 > >::iterator it = m_pairs.begin();
        for(; it != m_pairs.end(); ++it){
            out << (*it).first << '\t' << (*it).second << '\n';
        }
        file.close();
    }

    /** Reads the value pairs from the file. */
    void loadPairs(){
        //empties the pairs list
        clear();
        //read file contents
        QFile file( _path );
        file.open( QFile::ReadOnly | QFile::Text );
        QTextStream in(&file);
        //for each file line
        while ( !in.atEnd() )
        {
           QString line = in.readLine();
           QStringList values = line.split('\t');
           addPair( valueconverter::convert<T1>( values[0] ),
                    valueconverter::convert<T2>( values[1] ));
        }
        file.close();
    }

protected:
    QList< QPair< T1, T2 > > m_pairs;


    // File interface
public:
    virtual void writeToFS(){ savePairs(); }
    virtual void readFromFS(){ loadPairs(); }
    virtual void clearLoadedContents() { clear(); }
    virtual long getContentsCount() { return getPairCount(); }

};



#endif // VALUEPAIRS_H
