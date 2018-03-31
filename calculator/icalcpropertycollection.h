#ifndef ICALCPROPERTYCOLLECTION_H
#define ICALCPROPERTYCOLLECTION_H

#include <QString>

class ICalcProperty;

/**
 * The ICalcPropertyCollection interface should be implemented by classes to be usable in the Calculator.
 * ICalcPropertyCollection represents tabular data, that is, properties as columns with equal number of values each.
 */
class ICalcPropertyCollection
{
public:
    ICalcPropertyCollection();
    virtual ~ICalcPropertyCollection(){}

	/** Returns the name of the collection. */
	virtual QString getCalcPropertyCollectionName() = 0;

	/** Returns the number of properties in this collection. */
	virtual int getCalcPropertyCount() = 0;

	/** Returns one property given its index. */
	virtual ICalcProperty* getCalcProperty( int index ) = 0;

	/** Returns the number of data values in the properties (assumes all
	 * properties have the same number of values). */
	virtual int getCalcRecordCount() = 0;

	/** Returns the value of the given variable (table column) in the given record (table line). */
	virtual double getCalcValue( int iVar, int iRecord ) = 0;

	/** Sets the value of the given variable (table column) in the given record (table line). */
	virtual void setCalcValue( int iVar, int iRecord, double value ) = 0;

	/** Called when a computation will commence.  This might prompt implementations to fetch
	 *  data from file, network, etc.. */
	virtual void computationWillStart() = 0;

	/** Called when a computation has completed.  This might prompt implementations to save
	 *  changes to file, for example. */
	virtual void computationCompleted() = 0;

	/** Returns (via output parameters) the spatial coordinates (x,y,z) and the topological
	 * coordinates (i,j,k) corresponding to the record index (data sample index).
	 */
	virtual void getSpatialAndTopologicalCoordinates( int iRecord, double& x, double& y, double& z, int& i, int& j, int& k ) = 0;

    /** Returns a property's index given its name.
     * Implementations should return -1 if the property is not found.
     */
	virtual int getCalcPropertyIndex( const std::string& name ) = 0;

	/**
	 * Returns a neighbouring value.  This only makes sense with property collections that have topology.
     * Non-topological implementations (e.g. point sets) or calls to non-existing neighbours (e.g. at edges)
     *  should return std::numeric_limits<double>::quiet_NaN();.
	 * @param iRecord Number of data line.
	 * @param iVar Index of the variable to retrieve value.
	 * @param dI Relative topological position, e.g. -1.
	 * @param dJ Relative topological position.
	 * @param dK Relative topological position.
	 */
	virtual double getNeighborValue( int iRecord, int iVar, int dI, int dJ, int dK ) = 0;

    /** Returns a property's index given its script-compatible name (with illegal characters replaced
     * by underscores). Returns -1 if the property is not found.
     */
    int getCalcPropertyIndexByScriptCompatibleName( const std::string& name );
};

#endif // ICALCPROPERTYCOLLECTION_H
