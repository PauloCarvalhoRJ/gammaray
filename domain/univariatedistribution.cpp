#include "univariatedistribution.h"
#include "util.h"

#include "domain/application.h"

UnivariateDistribution::UnivariateDistribution(const QString path) :
    Distribution( path ),
    m_data( path )
{
}

QIcon UnivariateDistribution::getIcon()
{
    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
        return QIcon(":icons/unidist16");
    else
        return QIcon(":icons32/unidist32");
}

double UnivariateDistribution::getValueFromCumulativeFrequency(double cumulativeProbability) const
{
    //result is NaN by default
    double result = std::numeric_limits<double>::quiet_NaN();

    //get the columns for the Z and P values of the distribution
    int zValueIndex = getTheColumnWithValueRole()-1;
    int pValueIndex = getTheColumnWithProbabilityRole()-1;

    //sanity checks
    if( m_data.getDataTable().empty() ){
        Application::instance()->logError("UnivariateDistribution::getValueFromCumulativeFrequency(): distribution data not loaded. "
                                          "Make sure there is a prior call to UnivariateDistribution::readFromFS().");
        return result;
    }
    if( zValueIndex < 0 ){
        Application::instance()->logError("UnivariateDistribution::getValueFromCumulativeFrequency(): no field or more than one field with Z-Value role.");
        return result;
    }
    if( pValueIndex < 0 ){
        Application::instance()->logError("UnivariateDistribution::getValueFromCumulativeFrequency(): no field or more than one field with P-Value role.");
        return result;
    }

    //traverse the distribution values table to determine the Z-value corresponding
    //to the given cumulative probability
    double previousZValue = 0.0;
    double previousPValue = 0.0;
    double previousCumulativeP = 0.0;
    double cumulativeP = 0.0;
    for( int i = 0; i < m_data.getDataTable().size(); ++i ){
        const std::vector<double>& record = m_data.getDataTable()[i];
        double zValue = record[ zValueIndex ];
        double pValue = record[ pValueIndex ];
        cumulativeP += pValue;
        if( i > 0 ){ //1st point is the start of the distribution, that is, we don't have a ramp yet.
            if( cumulativeProbability < cumulativeP ){
                //the client P passed may be lower than the starting P of the distribution.
                //using a P lower than the min P result in extrapolation, with likely wrong Z-values.
                double cumulativePtoUse = std::max( cumulativeProbability, previousCumulativeP );
                //the resulting z value is a linear interpolation between two cumulative probabilities and two z-values.
                result = Util::linearInterpolation( cumulativePtoUse, previousCumulativeP, cumulativeP, previousZValue, zValue );
                if( result < previousZValue || result > zValue )
                    Application::instance()->logWarn( "UnivariateDistribution::getValueFromCumulativeFrequency(): extrapolation occured. "
                                                      "Returned Z value is likely wrong.  Check passed cumulative probability value." );
                return result;
            }
        }
        previousZValue = zValue;
        previousPValue = pValue;
        previousCumulativeP = cumulativeP;
    }

    return result;
}

void UnivariateDistribution::readFromFS()
{
    m_data.loadData();
}

File *UnivariateDistribution::duplicatePhysicalFiles(const QString new_file_name)
{
    QString duplicateFilePath = duplicateDataAndMetaDataFiles( new_file_name );
    return new UnivariateDistribution( duplicateFilePath );
}
