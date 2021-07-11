#include "categorypdf.h"
#include "util.h"
#include "categorydefinition.h"
#include "application.h"
#include "project.h"
#include "objectgroup.h"

CategoryPDF::CategoryPDF(CategoryDefinition *cd, QString path) : IntDoublePairs ( path ),
    m_categoryDefinition( cd )
{
}

CategoryPDF::CategoryPDF(const QString categoryDefinitionName, QString path) : IntDoublePairs ( path ),
    m_categoryDefinition( nullptr ),
    m_categoryDefinitionNameForDelayedLoad( categoryDefinitionName )
{

}

CategoryDefinition *CategoryPDF::getCategoryDefinition()
{
    if( setCategoryDefinition() )
        return m_categoryDefinition;
    else
        return nullptr;
}

bool CategoryPDF::sumsToOne( double tolerance ) const
{
    return std::abs( sumProbs() - 1.0 ) < tolerance;
}

bool CategoryPDF::hasZeroOrLessProb() const
{
    for( const QPair<int, double>& idp : m_pairs )
        if( idp.second <= 0.0 )
            return true;
    return false;
}

bool CategoryPDF::hasNegativeProbabilities() const
{
    for( const QPair<int, double>& idp : m_pairs )
        if( idp.second < 0.0 )
            return true;
    return false;
}

int CategoryPDF::getFaciesFromCumulativeFrequency(double cumulativeProbability)
{
    CategoryDefinition* cd = getCategoryDefinition();
    assert( cd && "CategoryPDF::getFaciesFromCumulativeFrequency(): getCategoryDefinition() returned nullptr.");
    if( m_pairs.empty() )
        readFromFS();
    double cumProb = 0.0;
    for( const QPair<int, double>& idp : m_pairs ) {
        cumProb += idp.second;
        if( cumulativeProbability <= cumProb )
            return idp.first;
    }
    return -1;
}

double CategoryPDF::sumProbs() const
{
    double sum = 0.0;
    for( const QPair<int, double>& idp : m_pairs )
        sum += idp.second;
    return sum;
}

std::vector<double> CategoryPDF::getProbabilities() const
{
    std::vector<double> result;
    result.reserve( m_pairs.size() );
    for( const QPair<int, double>& idp : m_pairs )
        result.push_back( idp.second );
    return result;
}

void CategoryPDF::setProbabilities( const std::vector<double> &probabilities )
{
    uint index = 0;
    for( double prob : probabilities ){
        m_pairs[index].second = prob;
        ++index;
    }
}

void CategoryPDF::forceSumToOne()
{
    std::vector<double> probs = getProbabilities();
    double sum = std::accumulate(probs.begin(), probs.end(), 0.0);
    //divide all elements to their sum.
    std::transform(probs.begin(), probs.end(), probs.begin(), [sum](double &c){ return c/sum; });
    setProbabilities( probs );
}

QIcon CategoryPDF::getIcon()
{
    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI)
        return QIcon(":icons/catpdf16");
    else
        return QIcon(":icons32/catpdf32");
}

void CategoryPDF::save(QTextStream *txt_stream)
{
    QString usedCategoryDefinitionName;
    if( m_categoryDefinition )
        usedCategoryDefinitionName = m_categoryDefinition->getName();
    else
        usedCategoryDefinitionName = m_categoryDefinitionNameForDelayedLoad;

    if( usedCategoryDefinitionName.isEmpty() ){
        Application::instance()->logError(
                    "ERROR: CategoryPDF::save(): blank CategoryDefinition name.");
        usedCategoryDefinitionName = "FILE_NOT_FOUND";
    }

    (*txt_stream) << this->getFileType() << ":" << this->getFileName()
                                         << ',' << usedCategoryDefinitionName << '\n';
}

bool CategoryPDF::setCategoryDefinition()
{
    if( ! m_categoryDefinition ){
        m_categoryDefinition = (CategoryDefinition*)Application::instance()->
                getProject()->
                getResourcesGroup()->
                getChildByName( m_categoryDefinitionNameForDelayedLoad );
        if( ! m_categoryDefinition ){
            Application::instance()->logError(
                        "ERROR: CategoryPDF::setCategoryDefinition(): CategoryDefinition "
                        + m_categoryDefinitionNameForDelayedLoad + " not found in the project." );
            return false;
        } else
            return true;
    } else
        return true;
}
