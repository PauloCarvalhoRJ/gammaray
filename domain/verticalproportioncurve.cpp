#include "verticalproportioncurve.h"

#include <QFile>
#include <QTextStream>
#include <cassert>

#include "domain/application.h"
#include "domain/project.h"
#include "domain/objectgroup.h"
#include "domain/attribute.h"

VerticalProportionCurve::VerticalProportionCurve(QString path, QString associatedCategoryDefinitionName) :
    DataFile( path ),
    m_associatedCategoryDefinitionName( associatedCategoryDefinitionName )
{

}

void VerticalProportionCurve::addNewEntry(double relativeDepth)
{
    CategoryDefinition* cd = getAssociatedCategoryDefinition();
    assert( cd && cd->getCategoryCount() > 0 && "VerticalProportionCurve::addNewEntry(): "
                                                "Category definition is null or is empty/not loaded." );
    m_entries.push_back( VPCEntry( relativeDepth, cd ) );
}

int VerticalProportionCurve::getEntriesCount() const
{
    return m_entries.size();
}

int VerticalProportionCurve::getProportionsCount() const
{
    if( isEmpty() )
        return 0;
    else
        return m_entries[0].proportions.size();
}

void VerticalProportionCurve::setProportion(int entryIndex, int categoryCode, double proportion)
{
    CategoryDefinition* cd = getAssociatedCategoryDefinition();
    assert( cd && "VerticalProportionCurve::setProportion(): No CategoryDefinition was found.  "
                  "Be sure to set a name of an existing CategoryDefinition before calling this method" );

    int categoryIndex = cd->getCategoryIndex( categoryCode );

    m_entries[ entryIndex ].proportions[ categoryIndex ] = proportion;
}

bool VerticalProportionCurve::isEmpty() const
{
    return m_entries.empty();
}

bool VerticalProportionCurve::setAsMeanOf(const std::vector<VerticalProportionCurve> &curves)
{
    if( curves.empty() )
        //does nothing.
        return true;
    else if( curves.size() == 1 ){
        //simply copies the entries of the single other curve.
        m_entries = std::vector< VPCEntry >( curves[0].m_entries.begin(), curves[0].m_entries.end() );
        return true;
    } else {

        //check compatibility amongst the curves passed.
        const VerticalProportionCurve& firstVPC = curves[0];
        VPCIncompatibilityReason incompatibilityReason;
        for( int iCurve = 1; iCurve < curves.size(); ++iCurve ){
            const VerticalProportionCurve& vpc = curves[iCurve];
            if( ! firstVPC.isCompatibleWith( vpc, incompatibilityReason ) ){
                switch (incompatibilityReason) {
                case VPCIncompatibilityReason::DIFFERENT_ENTRY_COUNTS:
                    Application::instance()->logError( "VerticalProportionCurve::setAsMeanOf(): curve " +
                                                       QString::number( iCurve + 1 ) +
                                                       " showed a different entry count.");
                    break;
                case VPCIncompatibilityReason::NULL_CATEGORY_DEFINITION:
                    Application::instance()->logError( "VerticalProportionCurve::setAsMeanOf(): first curve or curve " +
                                                       QString::number( iCurve + 1 ) +
                                                       " has a null category definition.");
                    break;
                case VPCIncompatibilityReason::DIFFERENT_RELATIVE_DEPTHS:
                    Application::instance()->logError( "VerticalProportionCurve::setAsMeanOf():  curve " +
                                                       QString::number( iCurve + 1 ) +
                                                       " has different relative depths in its entries.");
                    break;
                case VPCIncompatibilityReason::DIFFERENT_PROPORTION_COUNTS:
                    Application::instance()->logError( "VerticalProportionCurve::setAsMeanOf():  curve " +
                                                       QString::number( iCurve + 1 ) +
                                                       " has different proportion values in its entries.");
                    break;
                case VPCIncompatibilityReason::DIFFERENT_CATEGORY_DEFINITIONS:
                    Application::instance()->logError( "VerticalProportionCurve::setAsMeanOf():  curve " +
                                                       QString::number( iCurve + 1 ) +
                                                       " refers to different category definitions.");
                    break;
                }
                return false;
            }
        }

        //make mean.  If execution reachs this point, then all curves are compatible.
        CategoryDefinition* cd = getAssociatedCategoryDefinition();

        //initialize the entries by the number of entries in the 1st VPC (assumes all VPCs are compatible).
        for( int iEntry = 0; iEntry < curves[0].m_entries.size(); ++iEntry ){
            const VPCEntry& vpcEntry = curves[0].m_entries[iEntry];
            m_entries.push_back( VPCEntry( curves[0].m_entries[iEntry].relativeDepth, cd ) );
        }

        //for each curve
        for( const VerticalProportionCurve& curve : curves ){
            //for each entry
            for( int iEntry = 0; iEntry < curve.m_entries.size(); ++iEntry ){
                const VPCEntry& vpcEntry = curve.m_entries[iEntry];
                //accumlate sum for the computation of the mean after the loop
                //over the curves finish
                std::transform( m_entries[iEntry].proportions.begin( ),
                                m_entries[iEntry].proportions.end( ),
                                vpcEntry.proportions.begin( ),
                                m_entries[iEntry].proportions.begin( ),
                                std::plus<double>( ));
            }
        }

        //compute the mean
        for( int iEntry = 0; iEntry < m_entries.size(); ++iEntry ){
            //divide all the sums of proportions for each entry by the number of curves.
            std::transform( m_entries[iEntry].proportions.begin(),
                            m_entries[iEntry].proportions.end(),
                            m_entries[iEntry].proportions.begin(),
                            std::bind( std::divides<double>(), std::placeholders::_1, curves.size() ) );
            //Rescale the values in the entry, so they sum up to 1.0.
            Util::unitize( m_entries[iEntry].proportions );
        }

        return true;

    } //if number of curves is > 2
}

bool VerticalProportionCurve::isCompatibleWith(const VerticalProportionCurve &otherVPC,
                                               VPCIncompatibilityReason &reason) const
{
    CategoryDefinition* myCD = getAssociatedCategoryDefinition();
    CategoryDefinition* otherCD = otherVPC.getAssociatedCategoryDefinition();

    if( !myCD || !otherCD ){
        reason = VPCIncompatibilityReason::NULL_CATEGORY_DEFINITION;
        return false;
    }

    if( myCD != otherCD ){
        reason = VPCIncompatibilityReason::DIFFERENT_CATEGORY_DEFINITIONS;
        return false;
    }

    if( getEntriesCount() != otherVPC.getEntriesCount() ){
        reason = VPCIncompatibilityReason::DIFFERENT_ENTRY_COUNTS;
        return false;
    }

    if( getProportionsCount() != otherVPC.getProportionsCount() ){
        reason = VPCIncompatibilityReason::DIFFERENT_PROPORTION_COUNTS;
        return false;
    }

    bool sameRelativeDepths = true;
    for( int iEntry = 0; iEntry < getEntriesCount();  ++iEntry ){
        if( ! Util::almostEqual2sComplement( getRelativeDepth( iEntry ),
                                             otherVPC.getRelativeDepth( iEntry ),
                                             1 ) ){
            reason = VPCIncompatibilityReason::DIFFERENT_RELATIVE_DEPTHS;
            return false;
        }
    }

    return true;
}

double VerticalProportionCurve::getRelativeDepth( uint entryIndex ) const
{
    if( isEmpty() || entryIndex >= m_entries.size() )
        return std::numeric_limits<double>::quiet_NaN();
    else
        return m_entries[ entryIndex ].relativeDepth;
}

CategoryDefinition *VerticalProportionCurve::getAssociatedCategoryDefinition() const
{
    CategoryDefinition* result = dynamic_cast<CategoryDefinition*>( Application::instance()->
                                                                               getProject()->
                                                                        getResourcesGroup()->
                                                                     getChildByName( m_associatedCategoryDefinitionName ) );
    if( ! result )
        Application::instance()->logError( "VerticalProportionCurve::getAssociatedCategoryDefinition(): "
                                           "object does not exist or an object of different type was found." );
    return result;
}

std::vector<double> VerticalProportionCurve::getNthProportions(uint proportionIndex) const
{
    std::vector<double> result( m_entries.size() );

    int iEntry = -1;
    for( const VPCEntry& entry : m_entries )
        result[++iEntry] = entry.proportions[ proportionIndex ];

    return result;
}

std::vector<double> VerticalProportionCurve::getNthCumulativeProportions(uint proportionIndex) const
{
    std::vector<double> result( m_entries.size() );

    int iEntry = -1;
    for( const VPCEntry& entry : m_entries ){
        double cumulativeProportion = 0.0;
        for( int iProportion = 0; iProportion <= proportionIndex; ++iProportion)
            cumulativeProportion += entry.proportions[ iProportion ];
        result[++iEntry] = cumulativeProportion;
    }

    return result;
}

void VerticalProportionCurve::print() const
{
    for( const VPCEntry& entry : m_entries ){
        for( int iProportion = 0; iProportion < entry.proportions.size(); ++iProportion)
            std::cout << entry.proportions[ iProportion ] << '\t';
        std::cout << std::endl;
    }
}

void VerticalProportionCurve::setInfoFromMetadataFile()
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

void VerticalProportionCurve::setInfo(QString associatedCategoryDefinitionName)
{
    m_associatedCategoryDefinitionName = associatedCategoryDefinitionName;
    //updates the sub-tree of child objects.
    updateChildObjectsCollection();
}

QString VerticalProportionCurve::getProportionVariableName( uint proportionIndex ) const
{
    //First column (0 index, 1 GeoEAS index) is the relative depth.
    //First proportion is the second column.
    Attribute* at = getAttributeFromGEOEASIndex( proportionIndex + 2 );
    assert( at && "VerticalProportionCurve::getProportionVariableName(): no such attribute.  Check for "
                  "indexes out of range or a missing prior call to VerticalProportionCurve::readFromFS()." );
    return at->getName();
}

double VerticalProportionCurve::getProportionAt(uint proportionIndex, double relativeDepth) const
{
    //traverse all entries in this VPC.
    for( uint iEntry = 1; iEntry < m_entries.size(); ++iEntry ){
        VPCEntry entry0 = m_entries[ iEntry-1 ];
        VPCEntry entry1 = m_entries[  iEntry  ];
        //if the queried relative depth falls between two entries...
        if( relativeDepth >= entry0.relativeDepth && relativeDepth <= entry1.relativeDepth ){
            //...interpolates the proportion between them.
            return Util::linearInterpolation( relativeDepth,
                                              entry0.relativeDepth,
                                              entry1.relativeDepth,
                                              entry0.proportions[ proportionIndex ],
                                              entry1.proportions[ proportionIndex ]
                                              );
        }
    }
    return std::numeric_limits<double>::quiet_NaN();
}

QIcon VerticalProportionCurve::getIcon()
{
    return QIcon(":icons32/vpc32");
}

QString VerticalProportionCurve::getTypeName() const
{
    return "VERTICALPROPORTIONCURVE";
}

void VerticalProportionCurve::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
    //also saves the metadata file.
    this->updateMetaDataFile();
}

bool VerticalProportionCurve::canHaveMetaData()
{
    return true;
}

QString VerticalProportionCurve::getFileType() const
{
    return getTypeName();
}

void VerticalProportionCurve::updateMetaDataFile()
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

bool VerticalProportionCurve::isDataFile()
{
    //this class only reuses DataFile's infrastructure to read/save data,
    //as it is not a data file representing a spatial object.
    return false;
}

bool VerticalProportionCurve::isDistribution()
{
    return  false;
}

void VerticalProportionCurve::deleteFromFS()
{
    DataFile::deleteFromFS(); //delete the file itself.
    //also deletes the metadata file
    QFile file( this->getMetaDataFilePath() );
    file.remove(); //TODO: throw exception if remove() returns false (fails).  Also see QIODevice::errorString() to see error message.
}

File *VerticalProportionCurve::duplicatePhysicalFiles(const QString new_file_name)
{
    QString duplicateFilePath = duplicateDataAndMetaDataFiles( new_file_name );
    VerticalProportionCurve* vpcNew = new VerticalProportionCurve( duplicateFilePath, m_associatedCategoryDefinitionName );
    vpcNew->updateChildObjectsCollection();
    return vpcNew;
}

bool VerticalProportionCurve::isWeight(Attribute *at)
{
    //Vertical Proportion Curves are not supposed to have attributes, but, as it reuses DataFile,
    //the framework may call this anyway.
    return false;
}

Attribute *VerticalProportionCurve::getVariableOfWeight(Attribute *weight)
{
    Q_UNUSED( weight )
    assert( false && "VerticalProportionCurve::getVariableOfWeight(): VerticalProportionCurve is a data file, "
                     "but calling getVariableOfWeight() on it makes no sense." );
}

void VerticalProportionCurve::deleteVariable(uint columnToDelete)
{
    Q_UNUSED( columnToDelete )
    assert( false && "VerticalProportionCurve::deleteVariable(): Columns of a VerticalProportionCurve are fixed and cannot"
                     " be removed." );
}

bool VerticalProportionCurve::isRegular() {
    assert( false && "VerticalProportionCurve::isRegular(): a VerticalProportionCurve is a data set, but it is not"
                     " a spatial object." );
}

double VerticalProportionCurve::getDataSpatialLocation(uint line, CartesianCoord whichCoord)
{
    Q_UNUSED( line )
    Q_UNUSED( whichCoord )
    assert( false && "VerticalProportionCurve::getDataSpatialLocation(): a VerticalProportionCurve is not a spatial object." );
}

void VerticalProportionCurve::getDataSpatialLocation(uint line, double &x, double &y, double &z)
{
    Q_UNUSED( line )
    Q_UNUSED( x )
    Q_UNUSED( y )
    Q_UNUSED( z )
    assert( false && "VerticalProportionCurve::getDataSpatialLocation(): a VerticalProportionCurve is not a spatial object." );
}

bool VerticalProportionCurve::isTridimensional()
{
    assert( false && "VerticalProportionCurve::isTridimensional(): a VerticalProportionCurve is not a spatial object." );
}

void VerticalProportionCurve::writeToFS()
{
    //populate the _data member so we can reuse DataFile's writeToFS()
    for( const VPCEntry& entry : m_entries ){
        std::vector<double> record;
        record.push_back( entry.relativeDepth );
        for( double proportion : entry.proportions )
            record.push_back( proportion );
        _data.push_back( record );
    }

    //save the proportions
    DataFile::writeToFS();

    //after saving, we can discard the data frame, which is only necessary
    //to reuse DataFile's IO functionalities.
    _data.clear();
}

void VerticalProportionCurve::readFromFS()
{
    CategoryDefinition* cd = getAssociatedCategoryDefinition();
    if( ! cd ){
        Application::instance()->logError("VerticalProportionCurve::readFromFS(): CategoryDefinition is null."
                                          " Operation failed.");
        return;
    }

    //read the data in file into the DataFile::_data table.
    DataFile::readFromFS();

    //discard current entries (if any).
    m_entries.clear();

    //makes sure the associated category definition file is loaded.
    cd->readFromFS();

    //re-create entries from the data table.
    int colCount = getDataColumnCount();
    for( int iRow = 0; iRow < getDataLineCount(); ++iRow ){
        double relativeDepth = data( iRow, 0 );
        VPCEntry entry( relativeDepth, cd );
        for( int iCol = 1; iCol < colCount; ++iCol )
            entry.proportions[iCol-1] = data( iRow, iCol );
        m_entries.push_back( entry );
    }

    //The data table is no longer needed.
    _data.clear();
}

void VerticalProportionCurve::getSpatialAndTopologicalCoordinates(int iRecord, double &x, double &y, double &z, int &i, int &j, int &k)
{
    assert( false && "VerticalProportionCurve::getSpatialAndTopologicalCoordinates(): a VerticalProportionCurve is not a spatial object." );
}

double VerticalProportionCurve::getNeighborValue(int iRecord, int iVar, int dI, int dJ, int dK)
{
    assert( false && "VerticalProportionCurve::getNeighborValue(): a VerticalProportionCurve is not a spatial object." );
}
