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
#include <QFileInfo>
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
    std::vector<QString>::const_iterator it = std::find ( m_faciesNames.cbegin(), m_faciesNames.cend(), faciesName );
    if (it != m_faciesNames.cend()){
        return it - m_faciesNames.cbegin();
    }
    return -1;
}

int VerticalTransiogramModel::getFaciesCount() const
{
    return m_faciesNames.size();
}

double VerticalTransiogramModel::getTransitionProbability(uint fromFaciesCode, uint toFaciesCode, double h) const
{
    //it is possible that not all facies have transiograms modeled.  In this case, there is zero probability
    //of transition.
    if( m_faciesCodeToIndex.find( fromFaciesCode ) == m_faciesCodeToIndex.end() ||
        m_faciesCodeToIndex.find( toFaciesCode ) == m_faciesCodeToIndex.end() )
        return 0.0;

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
    //-----------try again after loading category data from file--------
    if( faciesIndex < 0 )
        cd->loadQuintuplets();
    faciesIndex = cd->getCategoryIndex( faciesCode );
    //----------------------------------------------------
    if( faciesIndex < 0 ){
        Application::instance()->logError("VerticalTransiogramModel::getColorOfCategory(): category index == -1"
                                          " returned for facies code == (" + QString::number(faciesCode) + ").  Returning default color.");
        return QColor();
    }
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
    int code = cd->getCategoryCodeByName( faciesName );
    //-----------try again after loading category data from file--------
    if( code == -999 )
        cd->loadQuintuplets();
    code = cd->getCategoryCodeByName( faciesName );
    //----------------------------------------------------
    if( code == -999 ){
        Application::instance()->logError("VerticalTransiogramModel::getCodeOfCategory(): category code == -999"
                                          " returned for facies name == (" + faciesName + ").");
    }
    return code;
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

void VerticalTransiogramModel::setRange(uint iRow, uint iCol, double range)
{
    //the non-const std::get<> returns a reference to the element.
    std::get<INDEX_OF_RANGE_IN_TRANSIOGRAM_PARAMETERS_TUPLE>( m_verticalTransiogramsMatrix[iRow][iCol] ) = range;
}

double VerticalTransiogramModel::getSill(uint iRow, uint iCol)
{
    return std::get<INDEX_OF_SILL_IN_TRANSIOGRAM_PARAMETERS_TUPLE>( m_verticalTransiogramsMatrix[iRow][iCol] );
}

void VerticalTransiogramModel::setSill(uint iRow, uint iCol, double sill)
{
    //the non-const std::get<> returns a reference to the element.
    std::get<INDEX_OF_SILL_IN_TRANSIOGRAM_PARAMETERS_TUPLE>( m_verticalTransiogramsMatrix[iRow][iCol] ) = sill;
}

bool VerticalTransiogramModel::isCompatibleWith(const VerticalTransiogramModel *otherVTM) const
{
    CategoryDefinition* thisCD = getCategoryDefinition();
    CategoryDefinition* otherCD = otherVTM->getCategoryDefinition();
    if( ! otherCD || ( thisCD != otherCD ) ){
        return false;
    }
    int nFacies = m_faciesNames.size();
    int nFaciesOther = otherVTM->getFaciesCount();
    if( nFacies != nFaciesOther ){
        return false;
    }
    for( int iFacies = 0; iFacies < nFacies; ++iFacies ){
        if( m_faciesNames[iFacies] != otherVTM->m_faciesNames[iFacies] )
            return false;
    }
    return true;
}

int VerticalTransiogramModel::getCategoryMatrixIndex(const QString categoryName) const
{
    return getFaciesIndex( categoryName );
}

void VerticalTransiogramModel::makeAsSameModel(const VerticalTransiogramModel &otherVTM)
{
    m_associatedCategoryDefinitionName = otherVTM.m_associatedCategoryDefinitionName;
    m_faciesNames = otherVTM.m_faciesNames;
    m_verticalTransiogramsMatrix = otherVTM.m_verticalTransiogramsMatrix;
    m_faciesCodeToIndex = otherVTM.m_faciesCodeToIndex;
}

void VerticalTransiogramModel::unitizeRowwiseSills()
{
    uint transiogramMatrixDimension = getRowOrColCount();
    for( uint iRow = 0; iRow < transiogramMatrixDimension; ++iRow ){
        double sumSill = 0.0;
        for( uint iCol = 0; iCol < transiogramMatrixDimension; ++iCol )
            sumSill += getSill( iRow, iCol );
        double ratio = 1.0 / sumSill;
        for( uint iCol = 0; iCol < transiogramMatrixDimension; ++iCol )
            setSill( iRow, iCol, getSill( iRow, iCol) * ratio );
    }
}

QIcon VerticalTransiogramModel::getIcon()
{
    return QIcon(":icons32/transiogram32");
}

QString VerticalTransiogramModel::getTypeName() const
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

QString VerticalTransiogramModel::getFileType() const
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
        out << '\t' << Util::putDoubleQuotesIfThereIsWhiteSpace( headerFacies ) ;
    out << '\n';

    //write the lines ( headers and transiogram parameters)
    std::vector< std::vector< VTransiogramParameters > >::const_iterator itTransiogramsLines = m_verticalTransiogramsMatrix.cbegin();
    for( const QString& headerFacies : m_faciesNames ){
        out << Util::putDoubleQuotesIfThereIsWhiteSpace( headerFacies ) ;
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
            ++lineNumber;
            // get the text file line
            QString line = in.readLine();
            // tokenize the line
            std::vector<std::string> tokens = Util::tokenizeWithDoubleQuotes( line.toStdString(), false );
            // the column headers
            if( lineNumber == 1 ){
                for( const std::string& token : tokens )
                    m_faciesNames.push_back( QString( token.c_str() ) );
            //the lines, each with a fascies name as their headers
            } else {
                int columnNumber = 0;
                std::vector< VTransiogramParameters > lineOfTransiogramParameters;
                for( const std::string& token : tokens ){
                    ++columnNumber;
                    if( columnNumber == 1){
                        //sanity check: all facies names in the line headers must exist as column headers
                        //              AND in the same order.
                        {
                            std::vector<QString>::iterator it = std::find ( m_faciesNames.begin(), m_faciesNames.end(), QString( token.c_str() ) );
                            if (it != m_faciesNames.end()){
                                int headFaciesIndex = it - m_faciesNames.begin();
                                if( headFaciesIndex+1 != lineNumber-1 ){ //2nd line in file is the 1st line after the line with column headers
                                    Application::instance()->logError("VerticalTransiogramModel::readFromFS(): facies " + QString(token.c_str()) +
                                                                      " in the line header of line " + QString::number(lineNumber) + ""
                                                                      " found at unexpected position (" + QString::number(headFaciesIndex+1) +
                                                                      ") with respect to its position as column header"
                                                                      " of matrix file " + this->getPath() + ".");
                                }
                            } else {
                                Application::instance()->logError("VerticalTransiogramModel::readFromFS(): facies " + QString(token.c_str()) +
                                                                  " in the line header of line " + QString::number(lineNumber) + ""
                                                                  " not found as a column header of matrix file " + this->getPath() + ".");
                            }
                        }
                    }else{
                        // get the string "<type>,<range>,<sill>"
                        QString verticalTransiogramParameters = QString( token.c_str() );
                        // get the tokens as separate itens
                        QStringList verticalTransiogramParametersTokens = verticalTransiogramParameters.split(',');
                        //convert the tokens to numeric values
                        bool ok = false;
                        VTransiogramStructureType iStructureType;
                        VTransiogramRange fRange;
                        VTransiogramSill fSill;
                        for( int i = 0; i < verticalTransiogramParametersTokens.size(); ++i )
                            switch ( i ) {
                            case 0: iStructureType = static_cast<VTransiogramStructureType>( verticalTransiogramParametersTokens[i].toInt( &ok ) ); break;
                            case 1: fRange = verticalTransiogramParametersTokens[i].toDouble( &ok ); break;
                            case 2: fSill = verticalTransiogramParametersTokens[i].toDouble( &ok ); break;
                            }
                        if( ! ok )
                            Application::instance()->logError("VerticalTransiogramModel::readFromFS(): conversion to number failed @ line " + QString::number(lineNumber) + ""
                                                              " of file " + this->getPath() + ".");
                        if( verticalTransiogramParametersTokens.size() != 3 )
                            Application::instance()->logError("VerticalTransiogramModel::readFromFS(): number of parameters is not 3 @ line " + QString::number(lineNumber) + ""
                                                              " of file " + this->getPath() + ".");
                        //add the newly built tuple of variograph parameyters to the line vector.
                        lineOfTransiogramParameters.push_back( { iStructureType, fRange, fSill } );
                    }
                }
                if( lineOfTransiogramParameters.size() != m_faciesNames.size() ){
                    Application::instance()->logWarn("VerticalTransiogramModel::readFromFS(): number of vertical transiograms differs from the "
                                                     "number of facies names in the header @ line " + QString::number(lineNumber) + ".");
                }
                //add the line of transiograms to the outer vector, forming a matrix of vertical transiogram models.
                m_verticalTransiogramsMatrix.push_back( lineOfTransiogramParameters );
            }
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

File *VerticalTransiogramModel::duplicatePhysicalFiles( const QString new_file_name )
{
    QString duplicateFilePath = duplicateDataAndMetaDataFiles( new_file_name );

    return new VerticalTransiogramModel( duplicateFilePath, m_associatedCategoryDefinitionName );
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
    assert( cd && "VerticalTransiogramModel::updateInternalFaciesCodeToIndexMap(): null category definition."
                  " Metadata file may be refering to a non-existent category definition file.");
    m_faciesCodeToIndex.clear();
    cd->loadQuintuplets();
    for( uint i = 0; i < cd->getCategoryCount(); ++i ){
        int faciesIndex = getFaciesIndex( cd->getCategoryName( i ) );
        int faciesCode = cd->getCategoryCode( i );
        assert( faciesCode >= 0 && "VerticalTransiogramModel::updateInternalFaciesCodeToIndexMap(): facies code not found." );
        if( faciesIndex >= 0 )
            m_faciesCodeToIndex.insert( { faciesCode, faciesIndex } );
    }
}
