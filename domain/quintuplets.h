#ifndef QUINTUPLETS_H
#define QUINTUPLETS_H

#include "domain/file.h"

#include <QFile>
#include <QTextStream>

/** The generic template for a string-to-object convert function.
 *  See the avaliable specializations in the cpp source file.  Add more as needed for
 *  object types other than those already implemented.*/
namespace quintupletvalueconverter {
    template <typename T> T convert( QString value );
    template <typename T> QString toText( T value );
}

/** This abstract generic class represents a file that contains records with five values of any type
 *  Each record is written as fields in line files and each field is separated by a tab character.
 */
template <class T1, class T2, class T3, class T4, class T5>
class Quintuplets : public File
{
public:
    Quintuplets(QString path) : File( path ){}

    int getQuintupletCount() const { return m_quintuplets.count(); }

    T1 get1stValue( int quintuplet_index ){ return std::get<0>( m_quintuplets.at(quintuplet_index) ); }
    T2 get2ndValue( int quintuplet_index ){ return std::get<1>( m_quintuplets.at(quintuplet_index) ); }
    T3 get3rdValue( int quintuplet_index ){ return std::get<2>( m_quintuplets.at(quintuplet_index) ); }
    T4 get4thValue( int quintuplet_index ){ return std::get<3>( m_quintuplets.at(quintuplet_index) ); }
    T5 get5thValue( int quintuplet_index ){ return std::get<4>( m_quintuplets.at(quintuplet_index) ); }

    void addQuintuplet( T1 first, T2 second, T3 third, T4 fourth, T5 fifth ){
        m_quintuplets.append( std::tuple<T1,T2,T3,T4,T5>( first, second, third, fourth, fifth ) );
    }

    void clear(){ m_quintuplets.clear(); }

    /** Writes the value quintuplets as lines in a text file. */
    void saveQintuplets(){
        QFile file( _path );
        file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&file);
        typename QList< std::tuple<T1, T2, T3, T4, T5> >::iterator it = m_quintuplets.begin();
        for(; it != m_quintuplets.end(); ++it){
            out << quintupletvalueconverter::toText( std::get<0>(*it) ) << '\t' <<
                   quintupletvalueconverter::toText( std::get<1>(*it) ) << '\t' <<
                   quintupletvalueconverter::toText( std::get<2>(*it) ) << '\t' <<
                   quintupletvalueconverter::toText( std::get<3>(*it) ) << '\t' <<
                   quintupletvalueconverter::toText( std::get<4>(*it) ) << '\n';
        }
        file.close();
    }

    /** Reads the value quintuplets from the file. */
    void loadQuintuplets(){
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
           QStringList lineValues = line.split('\t');
           // a line can lack some columns, for example, category definition was changed from Triads to Quintuples sometime after version 5.3
           QStringList values = {"","","","",""};
           int control = 0;
           for( const QString& lineValue : lineValues ){
               values[control] = lineValue;
               ++control;
           }

           addQuintuplet( quintupletvalueconverter::convert<T1>( values[0] ),
                          quintupletvalueconverter::convert<T2>( values[1] ),
                          quintupletvalueconverter::convert<T3>( values[2] ),
                          quintupletvalueconverter::convert<T4>( values[3] ),
                          quintupletvalueconverter::convert<T5>( values[4] ));
        }
        file.close();
    }

protected:
    QList< std::tuple<T1, T2, T3, T4, T5> > m_quintuplets;

    // File interface
public:
    virtual void writeToFS(){ saveQintuplets(); }
    virtual void readFromFS(){ loadQuintuplets(); }
    virtual void clearLoadedContents() { clear(); }
    virtual long getContentsCount() { return getQuintupletCount(); }
};

#endif // QUINTUPLETS_H
