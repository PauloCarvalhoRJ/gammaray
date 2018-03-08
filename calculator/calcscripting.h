#ifndef CALCSCRIPTING_H
#define CALCSCRIPTING_H

#include <QString>

//ATTENTION: Compile in a 64-bit tool set since exprtk.hpp is too large resulting in
//           "too many section" excpetion with MinGW 32-bit.  Also it is necessary
//           to enable the -Wa,-mbig-obj for MinGW or /bigobj for MSVC.
//           Another option is to use another scripting engine such as Python, Lua or muParser.

class ICalcPropertyCollection;

/**
 * @brief The Scripting class encapsulates the scripting engine (currently the ExprTK header library).
 */
class CalcScripting
{
public:

	/**
	 * @param propertyCollection The collection of properties to run calculations on. Cannot be null because the m_registers array is created with its number of properties.
	 */
	CalcScripting( ICalcPropertyCollection* propertyCollection );

	virtual ~CalcScripting();

	/** Executes the passed script against the property collection passed in the constructor.
	 * Returns false if it fails, then client code should call getLastError() to give feedback to the user.
	 */
	bool doCalc(const QString& script);

	QString getLastError(){ return m_lastError; }

	/** Returns the property collection. */
	ICalcPropertyCollection* getPropertyCollection( ){ return m_propertyCollection; }

private:
	/** Used for testing purposes. */
    void trig_function();

	ICalcPropertyCollection* m_propertyCollection;

	/** The registers are double variables that are bound to the script engine.
	 * The array index matches the index of the ICalcProperty objects in their parent ICalcPropertyCollection.
	 */
	double *m_registers;

	/** Stores the last error message in case doCalc() fails. */
	QString m_lastError;

	/** Sets whether doCalc() is allowed to be called. */
	bool m_isBlocked;
};

#endif // CALCSCRIPTING_H
