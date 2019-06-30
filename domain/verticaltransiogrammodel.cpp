#include "verticaltransiogrammodel.h"
#include "domain/application.h"
#include "domain/project.h"
#include "domain/objectgroup.h"
#include "domain/categorydefinition.h"
#include "util.h"
#include "geostats/geostatsutils.h"

#include <cassert>
#include <QTextStream>
#include <QFile>
#include <tuple>

VerticalTransiogramModel::VerticalTransiogramModel(QString path,
                                                   QString associatedCategoryDefinitionName ) :
        File( path ),
        m_associatedCategoryDefinitionName( associatedCategoryDefinitionName )
{

}

void VerticalTransiogramModel::setInfo(QString associatedCategoryDefinitionName)
{
    m_associatedCategoryDefinitionName = associatedCategoryDefinitionName;
}

void VerticalTransiogramModel::setInfoFromMetadataFile()
{
    QString md_file_path( this->_path );
    QFile md_file( md_file_path.append(".md") );
    QString associatedCDname;
    if( md_file.exists() ){
        md_file.open( QFile::ReadOnly | QFile::Text );
        QTextStream in(&md_file);
        for (int i = 0; !in.atEnd(); ++i)
        {
           QString line = in.readLine();
           if( line.startsWith( "ASSOCIATED_CATEGORY_DEFINITION:" ) ){
               associatedCDname = line.split(":")[1];
           }
        }
        md_file.close();
    }
    this->setInfo( associatedCDname );
}

void VerticalTransiogramModel::addParameters(QString headFacies, QString tailFacies, VTransiogramParameters verticalTransiogramParameters)
{
    //get the index of the head facies
    int headFaciesIndex = getFaciesIndex( headFacies );

    //head facies is new
    if( headFaciesIndex < 0 ){
        //register the new facies and adjusts the matrix
        addFacies( headFacies );
        //get the new index
        headFaciesIndex = m_faciesNames.size() - 1;
    }

    //get the index of the tail facies
    int tailFaciesIndex = getFaciesIndex( tailFacies );

    //tail facies is new
    if( tailFaciesIndex < 0 ){
        //register the new facies and adjusts the matrix
        addFacies( tailFacies );
        //get the new index
        tailFaciesIndex = m_faciesNames.size() - 1;
    }

    //finally, set the parameters
    m_verticalTransiogramsMatrix[headFaciesIndex][tailFaciesIndex] = verticalTransiogramParameters;
}

void VerticalTransiogramModel::clearParameters()
{
    m_faciesNames.clear();
    m_verticalTransiogramsMatrix.clear();
    updateInternalFaciesCodeToIndexMap();
}

CategoryDefinition *VerticalTransiogramModel::getCategoryDefinition() const
{
    return dynamic_cast<CategoryDefinition*>( Application::instance()
                                                       ->getProject()
                                                       ->getResourcesGroup()
                                              ->getChildByName( m_associatedCategoryDefinitionName ) );
}

int VerticalTransiogramModel::getFaciesIndex(const QString faciesName) const
{
    //get the index of the head facies
    int faciesIndex = -1;
    std::vector<QString>::const_iterator it = std::find ( m_faciesNames.cbegin(), m_faciesNames.cend(), faciesName );
    if (it != m_faciesNames.end())
        faciesIndex = it - m_faciesNames.begin();
    return faciesIndex;
}

double VerticalTransiogramModel::getTransitionProbability(uint fromFaciesCode, uint toFaciesCode, double h) const
{
    try {

        uint fromFaciesIndex = m_faciesCodeToIndex.at( fromFaciesCode );
        uint toFaciesIndex   = m_faciesCodeToIndex.at(  toFaciesCode  );
        const std::vector< VTransiogramParameters >& transriogramsRow = m_verticalTransiogramsMatrix[ fromFaciesIndex ];
        const VTransiogramParameters& transiogram = transriogramsRow[ toFaciesIndex ];

        TransiogramType transiogramType = TransiogramType::CROSS_TRANSIOGRAM;
        if( fromFaciesCode == toFaciesCode )
            transiogramType = TransiogramType::AUTO_TRANSIOGRAM;

        return GeostatsUtils::getTransiogramProbability( transiogramType,
                                                         std::get<0>( transiogram ),
                                                         h,
                                                         std::get<1>( transiogram ),
                                                         std::get<2>( transiogram ) );

    } catch ( std::out_of_range& e ) {
        assert( false && "VerticalTransiogramModel::getTransitionProbability(): facies code not in m_faciesCodeToIndex. "
                         "Could not resolve transiogram rol/col index for the given facies code. "
                         "Perhaps a prior call to VerticalTransiogramModel::updateInternalFaciesCodeToIndexMap() is missing." );
    }
}

uint VerticalTransiogramModel::getRowOrColCount() const
{
    return m_faciesNames.size();
}

QColor VerticalTransiogramModel::getColorOfCategory(uint index) const
{
    int faciesCode = getCodeOfCategory( index );
    CategoryDefinition* cd = getCategoryDefinition();
    int faciesIndex = cd->getCategoryIndex( faciesCode );
    if( faciesIndex < 0 )
        Application::instance()->logWarn("VerticalTransiogramModel::getColorOfCategory(): category index -1.  Returning default color.");
    return cd->getCustomColor( faciesIndex );
}

QString VerticalTransiogramModel::getNameOfCategory(uint index) const
{
    return m_faciesNames[index];
}

int VerticalTransiogramModel::getCodeOfCategory(uint index) const
{
    QString faciesName = getNameOfCategory( index );
    CategoryDefinition* cd = getCategoryDefinition();
    return cd->getCategoryCodeByName( faciesName );
}

double VerticalTransiogramModel::getLongestRange() const
{
    double result = -1.0;
    std::vector< std::vector< VTransiogramParameters > >::const_iterator itTransiogramsLines = m_verticalTransiogramsMatrix.cbegin();
    for(; itTransiogramsLines != m_verticalTransiogramsMatrix.cend(); ++itTransiogramsLines ){
        std::vector< VTransiogramParameters >::const_iterator itTransiograms = (*itTransiogramsLines).cbegin();
        for( ; itTransiograms != (*itTransiogramsLines).end() ; ++itTransiograms) {
            const VTransiogramParameters& vTransiogramParams = *itTransiograms;
            double range = std::get<INDEX_OF_RANGE_IN_TRANSIOGRAM_PARAMETERS_TUPLE>( vTransiogramParams ); //range is the second value in the VTransiogramParameters tuple
            if( range > result )
                result = range;
        }
    }
    if( result < 0 ){
        Application::instance()->logWarn( "VerticalTransiogramModel::getLongestRange(): no transiograms.  "
                                          "Perhaps you forgot a prior call to VerticalTransiogramModel::readFromFS().  "
                                          "Returning 10.0 by default." );
        return 10.0;
    }
    return result;
}

double VerticalTransiogramModel::getRange( uint iRow, uint iCol )
{
    return std::get<INDEX_OF_RANGE_IN_TRANSIOGRAM_PARAMETERS_TUPLE>( m_verticalTransiogramsMatrix[iRow][iCol] );
}

double VerticalTransiogramModel::getSill(uint iRow, uint iCol)
{
    return std::get<INDEX_OF_SILL_IN_TRANSIOGRAM_PARAMETERS_TUPLE>( m_verticalTransiogramsMatrix[iRow][iCol] );
}

QIcon VerticalTransiogramModel::getIcon()
{
    return QIcon(":icons32/transiogram32");
}

QString VerticalTransiogramModel::getTypeName()
{
    return "VERTICALTRANSIOGRAMMODEL";
}

void VerticalTransiogramModel::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
    //also saves the metadata file.
    this->updateMetaDataFile();
}

bool VerticalTransiogramModel::canHaveMetaData()
{
    return true;
}

QString VerticalTransiogramModel::getFileType()
{
    return getTypeName();
}

void VerticalTransiogramModel::updateMetaDataFile()
{
    QFile file( this->getMetaDataFilePath() );
    file.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&file);
    out << APP_NAME << " metadata file.  This file is generated automatically.  Do not edit this file.\n";
    out << "version=" << APP_VERSION << '\n';
    if( ! m_associatedCategoryDefinitionName.isEmpty() )
        out << "ASSOCIATED_CATEGORY_DEFINITION:" << m_associatedCategoryDefinitionName << '\n';
    file.close();
}

void VerticalTransiogramModel::writeToFS()
{
    //create a new file for output
    QFile outputFile( QString( this->getPath() ).append(".new") );
    outputFile.open( QFile::WriteOnly | QFile::Text );

    QTextStream out(&outputFile);

    //write the column headers
    for( const QString& headerFacies : m_faciesNames )
        out << '\t' << headerFacies ;
    out << '\n';

    //write the lines ( headers and transiogram parameters)
    std::vector< std::vector< VTransiogramParameters > >::const_iterator itTransiogramsLines = m_verticalTransiogramsMatrix.cbegin();
    for( const QString& headerFacies : m_faciesNames ){
        out << headerFacies ;
        std::vector< VTransiogramParameters >::const_iterator itTransiograms = (*itTransiogramsLines).cbegin();
        for( ; itTransiograms != (*itTransiogramsLines).end() ; ++itTransiograms)
            out << '\t' << static_cast<int>   (std::get<0>(*itTransiograms)) << ','  //structure type: spheric, exponential, gaussian, etc...
                        << static_cast<double>(std::get<1>(*itTransiograms)) << ','  //transiogram range:
                        << static_cast<double>(std::get<2>(*itTransiograms));        //transiogram sill:
        out << '\n';
        ++itTransiogramsLines;
    }

    // close output file
    outputFile.close();

    // deletes the current file
    QFile currentFile(this->getPath());
    currentFile.remove();
    // renames the .new file, effectively replacing the current file.
    outputFile.rename(this->getPath());
}

void VerticalTransiogramModel::readFromFS()
{
    //open the input file for reading
    QFile inputFile( getPath() );
    if ( inputFile.open(QIODevice::ReadOnly | QFile::Text ) ) {
        clearLoadedContents();
        QTextStream in(&inputFile);
        int lineNumber = 0;
        while ( !in.atEnd() ){
//            ++lineNumber;
//            // get the text file line
//            QString line = in.readLine();
//            // tokenize the line
//            std::vector<std::string> tokens = Util::tokenizeWithDoubleQuotes( line.toStdString(), false );
//            // the column headers
//            if( lineNumber == 1 ){
//                for( const std::string& token : tokens )
//                    m_faciesNames.push_back( QString( token.c_str() ) );
//            //the lines, each with a fascies name as their headers
//            } else {
//                int columnNumber = 0;
//                std::vector< VTransiogramParameters > lineOfTransiogramParameters;
//                for( const std::string& token : tokens ){
//                    ++columnNumber;
//                    if( columnNumber == 1){
//                        //sanity check: all facies names in the line headers must exist as column headers
//                        //              AND in the same order.
//                        {
//                            std::vector<QString>::iterator it = std::find ( m_faciesNames.begin(), m_faciesNames.end(), QString( token.c_str() ) );
//                            if (it != m_faciesNames.end()){
//                                int headFaciesIndex = it - m_faciesNames.begin();
//                                if( headFaciesIndex+1 != lineNumber ){
//                                    Application::instance()->logError("VerticalTransiogramModel::readFromFS(): facies " + QString(token.c_str()) +
//                                                                      " in the line header of line " + QString::number(lineNumber) + ""
//                                                                      " found at unexpected position with respect to its position as column header"
//                                                                      " of matrix file " + this->getPath() + ".");
//                                }
//                            } else {
//                                Application::instance()->logError("VerticalTransiogramModel::readFromFS(): facies " + QString(token.c_str()) +
//                                                                  " in the line header of line " + QString::number(lineNumber) + ""
//                                                                  " not found as a column header of matrix file " + this->getPath() + ".");
//                            }
//                        }
//                    }else{
//                        // get the string "<type>,<range>,<sill>"
//                        QString verticalTransiogramParameters = QString( token.c_str() );
//                        // get the tokens as separate itens
//                        QStringList verticalTransiogramParametersTokens = verticalTransiogramParameters.split(',');
//                        //convert the tokens to numeric values
//                        bool ok = false;
//                        VTransiogramStructureType iStructureType;
//                        VTransiogramRange fRange;
//                        VTransiogramSill fSill;
//                        for( int i = 0; i < verticalTransiogramParametersTokens.size(); ++i )
//                            switch ( i ) {
//                            case 0: iStructureType = static_cast<VTransiogramStructureType>( verticalTransiogramParametersTokens[i].toInt( &ok ) ); break;
//                            case 1: fRange = verticalTransiogramParametersTokens[i].toDouble( &ok ); break;
//                            case 2: fSill = verticalTransiogramParametersTokens[i].toDouble( &ok ); break;
//                            }
//                        if( ! ok )
//                            Application::instance()->logError("VerticalTransiogramModel::readFromFS(): conversion to number failed @ line " + QString::number(lineNumber) + ""
//                                                              " of file " + this->getPath() + ".");
//                        //add the newly built tuple of variograph parameyters to the line vector.
//                        lineOfTransiogramParameters.push_back( { iStructureType, fRange, fSill } );
//                    }
//                }
//                if( lineOfTransiogramParameters.size() != m_faciesNames.size() ){
//                    Application::instance()->logWarn("VerticalTransiogramModel::readFromFS(): number of vertical transiograms differs from the "
//                                                     "number of facies names in the header @ line " + QString::number(lineNumber) + ".");
//                }
//                //add the line of transiograms to the outer vector, forming a matrix of vertical transiogram models.
//                m_verticalTransiogramsMatrix.push_back( lineOfTransiogramParameters );
//            }
        }
        inputFile.close();
        updateInternalFaciesCodeToIndexMap();
    } else {
        Application::instance()->logError("VerticalTransiogramModel::readFromFS(): file " + getPath() + " not found or is not accessible.");
    }
}

void VerticalTransiogramModel::clearLoadedContents()
{
    m_faciesNames.clear();
    m_verticalTransiogramsMatrix.clear();
    updateInternalFaciesCodeToIndexMap();
}

bool VerticalTransiogramModel::isDataFile()
{
    return false;
}

bool VerticalTransiogramModel::isDistribution()
{
    return false;
}

void VerticalTransiogramModel::deleteFromFS()
{
    File::deleteFromFS(); //delete the file itself.
    //also deletes the metadata file
    QFile file( this->getMetaDataFilePath() );
    file.remove(); //TODO: throw exception if remove() returns false (fails).  Also see QIODevice::errorString() to see error message.
}

void VerticalTransiogramModel::addFacies(QString faciesName)
{
    //add a new line of default transiogram parameters to the matrix
    {
        std::vector<VTransiogramParameters> newLine;
        for( int i = 0; i < m_faciesNames.size();  ++i ){
            newLine.emplace_back( static_cast<VTransiogramStructureType>( 1 ),
                                  static_cast<VTransiogramRange>( 0.0 ),
                                  static_cast<VTransiogramSill>( 0.0 ) );
        }
        m_verticalTransiogramsMatrix.push_back( newLine );
    }

    //Register the facies name.
    //NOTE: do not move this code to before the previous block.
    m_faciesNames.push_back( faciesName );

    //append a new element with default transiogram parameters to each line, in effect adding a new column to the matrix
    {
        std::vector< std::vector < VTransiogramParameters > >::iterator itLines = m_verticalTransiogramsMatrix.begin();
        for( ; itLines != m_verticalTransiogramsMatrix.end(); ++itLines ){
            std::vector < VTransiogramParameters >& line = (*itLines);
            line.emplace_back( static_cast<VTransiogramStructureType>( 1 ),
                               static_cast<VTransiogramRange>( 0.0 ),
                               static_cast<VTransiogramSill>( 0.0 ) );
        }
    }

    updateInternalFaciesCodeToIndexMap();
}

void VerticalTransiogramModel::updateInternalFaciesCodeToIndexMap()
{
    CategoryDefinition* cd = getCategoryDefinition();
    m_faciesCodeToIndex.clear();
    for( uint i = 0; i < cd->getCategoryCount(); ++i ){
        uint faciesIndex = getFaciesIndex( cd->getCategoryName( i ) );
        uint faciesCode = cd->getCategoryCode( i );
        if( faciesIndex >= 0)
            m_faciesCodeToIndex.insert( { faciesCode, faciesIndex } );
    }
}
