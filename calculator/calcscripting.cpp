#include "calcscripting.h"
#include <exprtk.hpp>
#include "icalcpropertycollection.h"
#include "icalcproperty.h"
#include <cmath>

//This controls the use of custom script functions which are static.
CalcScripting* s_calcEngineUser = nullptr;
int s_currentIteraction;

// The custom neigh("var_name", dI, dJ, dK) script function
template <typename T>
struct neigh : public exprtk::igeneric_function<T>
{
	typedef typename exprtk::igeneric_function<T>::parameter_list_t	parameter_list_t;
	neigh() : exprtk::igeneric_function<T>("STTT") {} //S=string, T=scalar, V=vector, Z=no parameters, ?=any type, *=asterisk operator, |=param. sequ. delimiter.
	inline T operator()(parameter_list_t parameters)
	{
		//define some types to shorten code
		typedef typename exprtk::igeneric_function<T>::generic_type generic_type;
		typedef typename generic_type::scalar_view scalar_t;
		typedef typename generic_type::string_view string_t;

		//Get the variable name parameter value
		//TODO: throw exception if parameter count is different from 4 or types are different than string,int,int,int
		string_t tmpVarName(parameters[0]);
		std::string varName;
		varName.reserve(100); //this speeds up things a bit
        for( unsigned int i = 0; i < tmpVarName.size(); ++i )
			varName.push_back( tmpVarName[i] );

		//get the numerical parameters
		int dI = scalar_t(parameters[1])();
		int dJ = scalar_t(parameters[2])();
		int dK = scalar_t(parameters[3])();

		//the static variables that work as cache to avoid repetitive calls
		//to ICalcPropertyCollection::getCalcPropertyIndex() that may be slow
		static std::string varNameFromPreviousCall = "";
		static int propIndexFromPreviousCall = -9999;

		//get the property collection object
		ICalcPropertyCollection* propCol = s_calcEngineUser->getPropertyCollection();

		//This is to speed up the resolution of property index a bit
		int propIndex;
		if( varNameFromPreviousCall != varName ){
            propIndex = propCol->getCalcPropertyIndexByScriptCompatibleName( varName );
			propIndexFromPreviousCall = propIndex;
			varNameFromPreviousCall = varName;
		}else{
			propIndex = propIndexFromPreviousCall;
		}

        if( propIndex < 0 )
            return std::numeric_limits<double>::quiet_NaN();

        //finally actually retrieve the neighbor value
        return propCol->getNeighborValue( s_currentIteraction, propIndex, dI, dJ, dK );
	}
};

//The custom isNaN() script function
//returns 0 for false and 1 otherwise.
template <typename T>
inline T isNaN(T value)
{
   return std::isnan( value );
}

CalcScripting::CalcScripting(ICalcPropertyCollection * propertyCollection) :
	m_propertyCollection( propertyCollection ),
	m_registers( new double[propertyCollection->getCalcPropertyCount()] ),
	m_isBlocked( false )
{
	if( s_calcEngineUser )
		m_isBlocked = true;
	else
		s_calcEngineUser = this;
}

CalcScripting::~CalcScripting()
{
	if( ! m_isBlocked )
		s_calcEngineUser = nullptr;
	delete m_registers;
}

/** LOCAL FUNCTION: Converts an absolute char postion into line number an column number in the expression text. */
void getLineAndColumnFromPosition( const QString& expression, int position, int& line, int& col ){
    line = 1;
    col = 1;
    for( int i = 0; i < expression.size(); ++i ){
        if( i == position )
            return;
        QChar c = expression.at(i);
        if( c == '\n'){
            ++line;
            col = 1;
        }
        ++col;
    }
}

bool CalcScripting::doCalc( const QString & script )
{
	if( m_isBlocked ){
		m_lastError = QString( "Another instance of the calculator engine is running or active.  Maybe another calculation is going on in another Calculator Dialog." );
		return false;
	}

	//Define some types for brevity.
	typedef exprtk::symbol_table<double> symbol_table_t;
	typedef exprtk::expression<double> expression_t;
	typedef exprtk::parser<double> parser_t;
    typedef exprtk::parser_error::type error_t;

	//The registers to hold the spatial and topological coordinates.
	double _X_, _Y_, _Z_;
	double _I_, _J_, _K_; //set as double to be compatible with the ExprTk API (add_variable() template should be extended to support int)
	int _iI_, _iJ_, _iK_;

	//Get the script text.
	std::string expression_string = script.toStdString();

	//Bind script variables to actual memory variables (the registers).
	symbol_table_t symbol_table;
	for( int i = 0; i < m_propertyCollection->getCalcPropertyCount(); ++i )
		//                                                    [variable name in script]                              [actual variable]
		//                         vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv   vvvvvvvvvvvvvv
		symbol_table.add_variable( m_propertyCollection->getCalcProperty(i)->getScriptCompatibleName().toStdString(), m_registers[i]);

	//Bind artificial variables to access spatial and topological coordinates.
	symbol_table.add_variable("X_", _X_);
	symbol_table.add_variable("Y_", _Y_);
	symbol_table.add_variable("Z_", _Z_);
	symbol_table.add_variable("I_", _I_);
	symbol_table.add_variable("J_", _J_);
	symbol_table.add_variable("K_", _K_);

	//Bind the neigh() function
	neigh<double> neigh;
	symbol_table.add_function("neigh", neigh);

	//Bind the isNaN() function
	symbol_table.add_function("isNan", isNaN);

	//Bind constant symbols (e.g. pi).
	symbol_table.add_constants();

    //Bind vector functions like avg(), sort(), etc...
    exprtk::rtl::vecops::package<double> vecops_package;
    symbol_table.add_package ( vecops_package );

	//Register the variable bind table.
	expression_t expression;
	expression.register_symbol_table(symbol_table);

	//Parse the script against the variable bind table.
	parser_t parser;
	if( ! parser.compile(expression_string, expression) ){
        m_lastError = QString( parser.error().c_str() ) + "<br><br>\n\nError details:<br>\n";
        //retrive compilation error details
        for (std::size_t i = 0; i < parser.error_count(); ++i){
           error_t error = parser.get_error(i);
           QString tmp;
           int lin, col;
           getLineAndColumnFromPosition( script, error.token.position, lin, col);
           tmp.sprintf("%2d) %14s @ line=%3d, col=%3d: %s; <BR>\n",
                  (int)i+1,
                  exprtk::parser_error::to_str(error.mode).c_str(),
                  lin,
                  col,
                  error.diagnostic.c_str());
           m_lastError += tmp + '\n';
        }
        return false;
	}

	//Evaluate the script against all data records.
	for ( s_currentIteraction = 0; s_currentIteraction < m_propertyCollection->getCalcRecordCount(); ++s_currentIteraction)
	{
		//Fetch values from the property collection to the registers.
		for( int iVar = 0; iVar < m_propertyCollection->getCalcPropertyCount(); ++iVar )
			m_registers[iVar] = m_propertyCollection->getCalcValue( iVar, s_currentIteraction );
		//Fetch the spatial and topological coordinates special variables
		m_propertyCollection->getSpatialAndTopologicalCoordinates( s_currentIteraction, _X_, _Y_, _Z_, _iI_, _iJ_, _iK_ );
		_I_ = _iI_;
		_J_ = _iJ_;
		_K_ = _iK_;
		//Execute the script on the registers.
		expression.value();
		//Move the values from the registers to the property collection.
		for( int iVar = 0; iVar < m_propertyCollection->getCalcPropertyCount(); ++iVar )
			m_propertyCollection->setCalcValue( iVar, s_currentIteraction, m_registers[iVar] );
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
