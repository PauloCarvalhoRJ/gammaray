#include "faciestransitionmatrix.h"
#include "domain/categorydefinition.h"
#include "domain/application.h"
#include "domain/project.h"
#include "domain/objectgroup.h"
#include "util.h"
#include <QFile>
#include <QTextStream>

FaciesTransitionMatrix::FaciesTransitionMatrix(QString path,
                                               QString associatedCategoryDefinitionName ) :
    File( path ),
    m_associatedCategoryDefinitionName( associatedCategoryDefinitionName )
{

}

bool FaciesTransitionMatrix::isUsable()
{
    readFromFS();
    if( m_associatedCategoryDefinitionName.isEmpty() )
        return false;
    CategoryDefinition* cd = getAssociatedCategoryDefinition();
    if( ! cd )
        return false;
    cd->readFromFS();
    for( const QString& faciesName : m_columnHeadersFaciesNames )
        if( ! cd->categoryExistByName( faciesName ) ){
            Application::instance()->logError("FaciesTransitionMatrix::isUsable(): category name [" + faciesName
                                              + "] not found in " + m_associatedCategoryDefinitionName + ".");
            return false;
        }
    for( const QString& faciesName : m_lineHeadersFaciesNames )
        if( ! cd->categoryExistByName( faciesName ) ){
            Application::instance()->logError("FaciesTransitionMatrix::isUsable(): category name [" + faciesName
                                              + "] not found in " + m_associatedCategoryDefinitionName + ".");
            return false;
        }
    return true;
}

void FaciesTransitionMatrix::setInfo(QString associatedCategoryDefinitionName)
{
    m_associatedCategoryDefinitionName = associatedCategoryDefinitionName;
}

void FaciesTransitionMatrix::setInfoFromMetadataFile()
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

CategoryDefinition *FaciesTransitionMatrix::getAssociatedCategoryDefinition()
{
    CategoryDefinition* result = dynamic_cast<CategoryDefinition*>( Application::instance()->getProject()->getResourcesGroup()->getChildByName( m_associatedCategoryDefinitionName ) );
    if( ! result )
        Application::instance()->logError( "FaciesTransitionMatrix::getAssociatedCategoryDefinition(): object does not exist or an object of different type was found." );
    return result;
}

double FaciesTransitionMatrix::getTotal()
{
    double result = 0.0;
    for( const std::vector<double>& row : m_transitionProbabilities )
        for( const double& value : row )
            result += value;
    return result;
}

double FaciesTransitionMatrix::getSumOfRow( int rowIndex )
{
    double result = 0.0;
    int rowCount = 0;
    for( const std::vector<double>& row : m_transitionProbabilities ){
        if( rowIndex == rowCount ){
            for( const double& value : row )
                result += value;
            return result;
        }
        ++rowCount;
    }
    return result;
}

double FaciesTransitionMatrix::getTotalMinusSumOfRow(int rowIndex)
{
    return getTotal() - getSumOfRow( rowIndex );
}

double FaciesTransitionMatrix::getSumOfColumn(int columnIndex)
{
    double result = 0.0;
    for( const std::vector<double>& row : m_transitionProbabilities ){
        int columnCount = 0;
        for( const double& value : row ){
            if ( columnIndex == columnCount )
                result += value;
            ++columnCount;
        }
    }
    return result;
}

int FaciesTransitionMatrix::getColumnCount()
{
    return m_columnHeadersFaciesNames.size();
}

int FaciesTransitionMatrix::getRowCount()
{
    return m_lineHeadersFaciesNames.size();
}

QString FaciesTransitionMatrix::getColumnHeader(int columnIndex)
{
    return m_columnHeadersFaciesNames[columnIndex];
}

QString FaciesTransitionMatrix::getRowHeader(int rowIndex)
{
    return m_lineHeadersFaciesNames[rowIndex];
}

double FaciesTransitionMatrix::getValue(int rowIndex, int colIndex)
{
    return  m_transitionProbabilities[rowIndex][colIndex];
}

double FaciesTransitionMatrix::getValueMax()
{
    double max = 0.0;
    double value = m_transitionProbabilities[0][0];
    for( int i = 0; i < m_transitionProbabilities.size(); ++i )
        for( int j = 0; j < m_transitionProbabilities[i].size(); ++j ){
            value = m_transitionProbabilities[i][j];
            max = std::max( max, value );
        }
    return max;
}

QColor FaciesTransitionMatrix::getColorOfCategoryInColumnHeader(int columnIndex)
{
    QColor result;
    CategoryDefinition* cd = getAssociatedCategoryDefinition();
    if( ! cd )
        return result;
    int catCode = cd->getCategoryCodeByName( getColumnHeader( columnIndex ) );
    if( catCode == -999 )
        return result;
    return cd->getCustomColor( cd->getCategoryIndex( catCode ) );
}

QColor FaciesTransitionMatrix::getColorOfCategoryInRowHeader(int rowIndex)
{
    QColor result;
    CategoryDefinition* cd = getAssociatedCategoryDefinition();
    if( ! cd )
        return result;
    int catCode = cd->getCategoryCodeByName( getRowHeader( rowIndex ) );
    if( catCode == -999 )
        return result;
    return cd->getCustomColor( cd->getCategoryIndex( catCode ) );
}

double FaciesTransitionMatrix::getUpwardTransitionProbability(int fromFaciesRowIndex, int toFaciesColIndex)
{
    return getValue( fromFaciesRowIndex, toFaciesColIndex ) / getSumOfRow( fromFaciesRowIndex );
}

double FaciesTransitionMatrix::getDownwardTransitionProbability(int fromFaciesColumnIndex, int toFaciesRowIndex)
{
    return getValue( toFaciesRowIndex, fromFaciesColumnIndex ) / getSumOfRow( fromFaciesColumnIndex );
}

double FaciesTransitionMatrix::getPostDepositionalEntropy(int faciesIndex, bool normalize)
{
    double sum = 0.0;
    for( int j = 0; j < m_columnHeadersFaciesNames.size(); ++j ){
        double prob = getUpwardTransitionProbability( faciesIndex, j );
        if( ! Util::almostEqual2sComplement( prob, 0.0, 1) )
            sum += prob * std::log2( prob );
    }
    sum = -sum;
    if( normalize )
        sum /= -std::log2( 1.0 / (m_columnHeadersFaciesNames.size()-1));
    return sum;
}

double FaciesTransitionMatrix::getPreDepositionalEntropy(int faciesIndex, bool normalize)
{
    double sum = 0.0;
    for( int j = 0; j < m_columnHeadersFaciesNames.size(); ++j ){
        double prob = getDownwardTransitionProbability( faciesIndex, j );
        if( ! Util::almostEqual2sComplement( prob, 0.0, 1) )
            sum += prob * std::log2( prob );
    }
    sum = -sum;
    if( normalize )
        sum /= -std::log2( 1.0 / (m_columnHeadersFaciesNames.size()-1));
    return sum;
}

double FaciesTransitionMatrix::getIndependentTrail(int fromFaciesRowIndex, int toFaciesColIndex)
{
    if( fromFaciesRowIndex == toFaciesColIndex )
        return 0.0;
    return getSumOfColumn( toFaciesColIndex ) / ( getTotal() - getSumOfRow( fromFaciesRowIndex ) );
}

double FaciesTransitionMatrix::getDifference(int fromFaciesRowIndex, int toFaciesColIndex)
{
    if( fromFaciesRowIndex == toFaciesColIndex )
        return 0.0;
    return getUpwardTransitionProbability(fromFaciesRowIndex, toFaciesColIndex) - getIndependentTrail(fromFaciesRowIndex, toFaciesColIndex);
}

double FaciesTransitionMatrix::getMaxAbsDifference()
{
    double max = 0.0;
    double value = std::abs( getDifference(0, 0) );
    for( int i = 0; i < m_transitionProbabilities.size(); ++i )
        for( int j = 0; j < m_transitionProbabilities[i].size(); ++j ){
            value = std::abs( getDifference(i, j) );
            max = std::max( max, value );
        }
    return max;
}

double FaciesTransitionMatrix::getExpectedFrequency(int fromFaciesRowIndex, int toFaciesColIndex)
{
    if( fromFaciesRowIndex == toFaciesColIndex )
        return 0.0;
    return getIndependentTrail(fromFaciesRowIndex, toFaciesColIndex) * getSumOfRow( fromFaciesRowIndex );
}

double FaciesTransitionMatrix::getMaxExpectedFrequency()
{
    double max = 0.0;
    double value = std::abs( getExpectedFrequency(0, 0) );
    for( int i = 0; i < m_transitionProbabilities.size(); ++i )
        for( int j = 0; j < m_transitionProbabilities[i].size(); ++j ){
            value = std::abs( getExpectedFrequency(i, j) );
            max = std::max( max, value );
        }
    return max;
}

double FaciesTransitionMatrix::getRank()
{
    int cov_matrix_rank = 0;
    spectral::array eigenvectors, eigenvalues;
    std::tie( eigenvectors, eigenvalues ) = spectral::eig( toSpectralArray() );
    {
        double smallestEigenValue = std::numeric_limits<double>::max();
        double largestEigenValue = 0; //since the cov table is positive definite, the smallest value possible is zero.
        for( int i = 0; i < eigenvalues.size(); ++i ){
            double eigenvalue = eigenvalues(i);
            //since the cov table is positive definite, there is no need to compute the absolute value.
            if( eigenvalue < smallestEigenValue )
                smallestEigenValue = eigenvalue;
            if( eigenvalue > largestEigenValue )
                largestEigenValue = eigenvalue;
            if( eigenvalue > eta )
                cov_matrix_rank = i;
        }
        ++cov_matrix_rank;
        if( condition_number > 10 ){
            is_cov_matrix_ill_conditioned = true;
            ++nIllConditioned;
        }
    }
}

QIcon FaciesTransitionMatrix::getIcon()
{
    return QIcon(":icons32/transmat32");
}

QString FaciesTransitionMatrix::getTypeName()
{
    return "FACIESTRANSITIONMATRIX";
}

void FaciesTransitionMatrix::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
    //also saves the metadata file.
    this->updateMetaDataFile();
}

bool FaciesTransitionMatrix::canHaveMetaData()
{
    return true;
}

QString FaciesTransitionMatrix::getFileType()
{
    return getTypeName();
}

void FaciesTransitionMatrix::updateMetaDataFile()
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

void FaciesTransitionMatrix::writeToFS()
{
    throw InvalidMethodException();
}

void FaciesTransitionMatrix::readFromFS()
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
                    m_columnHeadersFaciesNames.push_back( QString( token.c_str() ) );
            //the lines, each with a fascies name as their headers
            } else {
                int columnNumber = 0;
                std::vector< double > lineOfProbValues;
                for( const std::string& token : tokens ){
                    ++columnNumber;
                    if( columnNumber == 1)
                        m_lineHeadersFaciesNames.push_back( QString( token.c_str() ) );
                    else{
                        bool ok;
                        double probValue = QString( token.c_str() ).toDouble( &ok );
                        if( ! ok ){
                            Application::instance()->logWarn("FaciesTransitionMatrix::readFromFS(): Illegal probability value @ line " +
                                                              QString::number(lineNumber) + ": [" + QString( token.c_str() ) + "].  Assumed zero.");
                            probValue = 0.0;
                        }
                        lineOfProbValues.push_back( probValue );
                    }
                }
                if( lineOfProbValues.size() != m_columnHeadersFaciesNames.size() ){
                    Application::instance()->logWarn("FaciesTransitionMatrix::readFromFS(): number of probability values differs from the "
                                                     "number of facies names in the header @ line " + QString::number(lineNumber) + ".");
                }
                m_transitionProbabilities.push_back( lineOfProbValues );
            }
        }
        inputFile.close();
    } else {
        Application::instance()->logError("FaciesTransitionMatrix::readFromFS(): file " + getPath() + " not found or is not accessible.");
    }
}

void FaciesTransitionMatrix::clearLoadedContents()
{
    m_columnHeadersFaciesNames.clear();
    m_lineHeadersFaciesNames.clear();
    m_transitionProbabilities.clear();
}

bool FaciesTransitionMatrix::isDataFile()
{
    return false;
}

bool FaciesTransitionMatrix::isDistribution()
{
    return false;
}


void FaciesTransitionMatrix::deleteFromFS()
{
    File::deleteFromFS(); //delete the file itself.
    //also deletes the metadata file
    QFile file( this->getMetaDataFilePath() );
    file.remove(); //TODO: throw exception if remove() returns false (fails).  Also see QIODevice::errorString() to see error message.
}

spectral::array FaciesTransitionMatrix::toSpectralArray()
{
    //Make a spectral-compatible copy of this matrix.
    spectral::array A( m_transitionProbabilities.size(), m_transitionProbabilities[0].size(), 1, 0.0 );
    for( int i = 0; i < A.M(); ++i)
        for( int j = 0; j < A.N(); ++j)
            A(i,j) = m_transitionProbabilities[i][j];
    return A;
}
