#include "histpltpar.h"

#include "../gslibparams/gslibparinputdata.h"
#include "../gslibparams/gslibparfile.h"
#include "../gslibparams/gslibparlimitsdouble.h"
#include "../gslibparams/gslibpardouble.h"
#include "../gslibparams/gslibparuint.h"
#include "../gslibparams/gslibparoption.h"
#include "../gslibparams/gslibparint.h"
#include "../gslibparams/gslibparstring.h"
#include "../gslibparams/gslibparrange.h"

HistpltPar::HistpltPar()
{
    _params.append( new GSLibParInputData() );
    _params.append( new GSLibParFile("PSOutput", "File por PostScript output") );
    _params.append( new GSLibParLimitsDouble ("AttrMinMax", "Attribute minimum and maximum") );
    _params.append( new GSLibParDouble ("FreqMax", "Frequency maximum (<0 for automatic)") );
    _params.append( new GSLibParUInt ("NumClasses", "Number of classes") );
    _params.append( new GSLibParOption ("ScaleOpt", "0=arithmetic, 1=log scaling") );
    _params.append( new GSLibParOption ("Hist", "0=frequency,  1=cumulative histogram") );
    _params.append( new GSLibParInt ("NumCumulQuantiles", "   number of cum. quantiles (<0 for all)") );
    _params.append( new GSLibParInt ("NumDecPlaces", "number of decimal places (<0 for auto.)") );
    _params.append( new GSLibParString ("PlotTitle", "Title") );
    _params.append( new GSLibParRange ("PosStats", "Positioning of stats (L to R: -1 to 1)", -1.0d, 1.0d) );
    _params.append( new GSLibParDouble ("RefValBoxPlot", "reference value for box plot" ));
}
