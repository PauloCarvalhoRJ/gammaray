#include "calcscripting.h"
#include <exprtk.hpp>
#include "icalcpropertycollection.h"
#include "icalcproperty.h"

CalcScripting::CalcScripting(ICalcPropertyCollection * propertyCollection) :
	m_propertyCollection( propertyCollection ),
	m_registers( new double[propertyCollection->getCalcPropertyCount()] )
{
}

CalcScripting::~CalcScripting()
{
	delete m_registers;
}

bool CalcScripting::doCalc( const QString & script )
{
	//Define some types for brevity.
	typedef exprtk::symbol_table<double> symbol_table_t;
	typedef exprtk::expression<double> expression_t;
	typedef exprtk::parser<double> parser_t;

	//The registers to hold the spatial and topological coordinates.
	double _X_, _Y_, _Z_;
	int _I_, _J_, _K_;

	//Get the script text.
	std::string expression_string = script.toStdString();

	//Bind script variables to actual memory variables (the registers).
	symbol_table_t symbol_table;
	for( int i = 0; i < m_propertyCollection->getCalcPropertyCount(); ++i )
		//                                                    [variable name in script]                              [actual variable]
		//                         vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv   vvvvvvvvvvvvvv
		symbol_table.add_variable( m_propertyCollection->getCalcProperty(i)->getScriptCompatibleName().toStdString(), m_registers[i]);

	//Bind artificial variables to access spatial and topological coordinates.
	symbol_table.add_variable("_X_", _X_);
	symbol_table.add_variable("_Y_", _Z_);
	symbol_table.add_variable("_Z_", _Y_);
	symbol_table.add_variable("_I_", _I_);
	symbol_table.add_variable("_J_", _J_);
	symbol_table.add_variable("_K_", _K_);

	//Bind constant symbols (e.g. pi).
	symbol_table.add_constants();

	//Register the variable bind table.
	expression_t expression;
	expression.register_symbol_table(symbol_table);

	//Parse the script against the variable bind table.
	parser_t parser;
	if( ! parser.compile(expression_string, expression) ){
		m_lastError = QString( parser.error().c_str() );
		return false;
	}

	//Evaluate the script against all data records.
	for ( int iRecord = 0; iRecord < m_propertyCollection->getCalcRecordCount(); ++iRecord)
	{
		//Fetch values from the property collection to the registers.
		for( int iVar = 0; iVar < m_propertyCollection->getCalcPropertyCount(); ++iVar )
			m_registers[iVar] = m_propertyCollection->getCalcValue( iVar, iRecord );
		//Fetch the spatial and topological coordinates special variables
		m_propertyCollection->getSpatialAndTopologicalCoordinates( iRecord, _X_, _Y_, _Z_, _I_, _J_, _K_ );
		//Execute the script on the registers.
		expression.value();
		//Move the values from the registers to the property collection.
		for( int iVar = 0; iVar < m_propertyCollection->getCalcPropertyCount(); ++iVar )
			m_propertyCollection->setCalcValue( iVar, iRecord, m_registers[iVar] );
		//The spatial and topological coordinates are read-only.
	}
	return true;
}

void CalcScripting::trig_function()
{
//   typedef exprtk::symbol_table<double> symbol_table_t;
//   typedef exprtk::expression<double>     expression_t;
//   typedef exprtk::parser<double>             parser_t;

//   std::string expression_string = "clamp(-1.0,sin(2 * pi * x) + cos(x / 2 * pi),+1.0)";

//   double x;

//   symbol_table_t symbol_table;
//   symbol_table.add_variable("x",x);
//   symbol_table.add_constants();

//   expression_t expression;
//   expression.register_symbol_table(symbol_table);

//   parser_t parser;
//   parser.compile(expression_string,expression);

//   for (x = double(-5); x <= double(+5); x += double(0.1))
//   {
//      double y = expression.value();
//      printf("%19.15f\t%19.15f\n",x,y);
//   }
}
