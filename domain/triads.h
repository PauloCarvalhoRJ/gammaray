#ifndef VALUETRIADS_H
#define VALUETRIADS_H

#include "file.h"
#include <QPair>
#include <QList>
#include <QFile>
#include <QTextStream>

/** The generic template for a string-to-object convert function.
 *  See the avaliable specializations in the cpp source file.  Add more as needed for
 *  object types other than those already implemented.*/
namespace triadvalueconverter {
    template <typename T> T convert( QString value );
}


/** This abstract generic class represents a file that contains triplets of types
 *  (can be of integer/string/integer, double/integer/string, integer/double/float, etc.).
 *  Each triad is written as fields in line files and each field is separated by a tab character.
 */
template <class T1, class T2, class T3>
class Triads : public File
{
public:
    Triads(QString path) : File( path ){}

    int getTripletCount(){ return m_triads.count(); }
    T1 get1stValue( int triplet_index ){ return m_triads.at(triplet_index).first; }
    T2 get2ndValue( int triplet_index ){ return m_triads.at(triplet_index).second.first; }
    T3 get3rdValue( int triplet_index ){ return m_triads.at(triplet_index).second.second; }
    void addTriplet( T1 first, T2 second, T3 third ){
        QPair<T2, T3> p23(second, third);
        m_triads.append( QPair< T1, QPair<T2, T3> >( first, p23 ) );
    }
    void clear(){ m_triads.clear(); }

    /** Writes the value triplets as lines in a text file. */
    void saveTriplets(){
        QFile file( _path );
        file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&file);
        typename QList< QPair< T1, QPair<T2, T3> > >::iterator it = m_triads.begin();
        for(; it != m_triads.end(); ++it){
            out << (*it).first << '\t' << (*it).second.first << '\t' << (*it).second.second << '\n';
        }
        file.close();
    }

    /** Reads the value triplets from the file. */
    void loadPairs(){
        //empties the triads list
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
           addTriplet( triadvalueconverter::convert<T1>( values[0] ),
                       triadvalueconverter::convert<T2>( values[1] ),
                       triadvalueconverter::convert<T3>( values[2] ));
        }
        file.close();
    }

protected:
    QList< QPair< T1, QPair< T2, T3 > > > m_triads;


};

#endif // VALUETRIADS_H
