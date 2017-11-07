#include "gslibparameterfile.h"
#include <QListIterator>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QRegularExpression>
#include "../../domain/application.h"
#include "../../domain/project.h"
#include "../../domain/variogrammodel.h"
#include "../../util.h"
#include "gslibparamtypes.h"
#include "../igslibparameterfinder.h"

GSLibParameterFile::GSLibParameterFile(const QString program_name)
{

    //saves a copy of the program name
    this->_program_name = program_name;

    //make the complete parameter template file
    QString template_path = Application::instance()->getProject()->getTemplatePath( program_name );

    //last <repeat> is initially nullptr, of course.
    _last_repeat = nullptr;

    QFile inputFile( template_path );
    if( ! inputFile.exists() ){
        QString msg("ERROR: parameter file template ");
        msg.append( template_path );
        msg.append(" not found.");
        Application::instance()->logError( msg );
    }

    //make component objects according to the parameter file template for the given program
    if ( inputFile.open(QIODevice::ReadOnly | QFile::Text ) ) {
       QTextStream in(&inputFile);
       bool in_header = true; //flags whether file read is still in header
       while ( !in.atEnd() ){
          QString line = in.readLine();
          if( in_header ){
              this->_header.append( line ).append("\n");
              if( line.trimmed().startsWith( "START OF PARAMETERS", Qt::CaseInsensitive ) )
                 in_header = false;
          } else {
              this->parseTemplateLine( line );
          }
       }
       inputFile.close();
    }
}

GSLibParameterFile::GSLibParameterFile()
{
   _header = "";
   _params.clear();
   _program_name = "";
   _last_repeat = nullptr;
}

GSLibParameterFile::~GSLibParameterFile()
{
    //TODO: I don't remember whether QList automatically deletes its objects upon its own
    //deallocation
}

void GSLibParameterFile::setDefaultValues()
{
    if( this->_program_name == "histplt" ){
        this->setDefaultValuesForHistplt();
    } else if ( this->_program_name == "locmap" ){
        this->setDefaultValuesForLocmap();
    } else if ( this->_program_name == "scatplt" ){
        this->setDefaultValuesForScatplt();
    } else if ( this->_program_name == "pixelplt" ){
        this->setDefaultValuesForPixelplt();
    } else if ( this->_program_name == "gamv" ){
        this->setDefaultValuesForGamv();
    } else if ( this->_program_name == "vargplt" ){
        this->setDefaultValuesForVargplt();
    } else if ( this->_program_name == "varmap" ){
        this->setDefaultValuesForVarmap();
    } else if ( this->_program_name == "gam" ){
        this->setDefaultValuesForGam();
    } else if ( this->_program_name == "declus" ){
        this->setDefaultValuesForDeclus();
    } else if ( this->_program_name == "nscoremv" ){
        this->setDefaultValuesForNScoreMV();
    } else if ( this->_program_name == "vmodel" ){
        this->setDefaultValuesForVmodel();
    } else if ( this->_program_name == "gammabar" ){
        Application::instance()->logWarn( "GSLibParameterFile::setDefaultValues(): not implemented for gammabar." );
    } else if ( this->_program_name == "nscore" ){
        Application::instance()->logWarn( "GSLibParameterFile::setDefaultValues(): not implemented for nscore." );
    } else if ( this->_program_name == "addcoord" ){
        Application::instance()->logWarn( "GSLibParameterFile::setDefaultValues(): not implemented for addcoord." );
    } else if ( this->_program_name == "histsmth" ){
        this->setDefaultValuesForHistsmth();
    } else if ( this->_program_name == "scatsmth" ){
        this->setDefaultValuesForScatsmth();
    } else if ( this->_program_name == "bivplt" ){
        this->setDefaultValuesForBivplt();
    } else if ( this->_program_name == "probplt" ){
        this->setDefaultValuesForProbplt();
    } else if ( this->_program_name == "qpplt" ){
        this->setDefaultValuesForQpplt();
    } else if ( this->_program_name == "kt3d" ){
        this->setDefaultValuesForKt3d();
    } else if ( this->_program_name == "ik3d" ){
        this->setDefaultValuesForIk3d();
    } else if ( this->_program_name == "postik" ){
        this->setDefaultValuesForPostik();
    } else if ( this->_program_name == "cokb3d" ){
        this->setDefaultValuesForCokb3d();
    } else if ( this->_program_name == "histpltsim" ){
        this->setDefaultValuesForHistpltsim();
    } else {
        QString msg("ERROR in setDefaultValues(): unsupported GSLib program: ");
        msg.append( this->_program_name );
        Application::instance()->logError( msg );
    }
}

void GSLibParameterFile::setValuesFromParFile(const QString path)
{
    //open the parameter file for reading
    QFile file( path );
    file.open( QFile::ReadOnly | QFile::Text );
    QTextStream in(&file);

    //skips the header of the parameter file
    bool in_header = true; //flags whether file read is still in header
    while ( !in.atEnd() ){
       QString line = in.readLine();
       if( in_header ){
           if( line.trimmed().startsWith( "START OF PARAMETERS", Qt::CaseInsensitive ) )
              break;
       }
    }

    //for each parameter in the collection
    //and for each parameter file text line
    QList<GSLibParType*>::iterator it = _params.begin();
    for(; it != _params.end() && !in.atEnd(); ++it)
    {
        GSLibParType* par = *it;
        if( ! par->isRepeat() ){ //Parameters of type repeat do not exist as a line in the parameter file
            QString line = in.readLine();
            this->parseParFileLine( line, par );
        }
        else //parameters of type repeat are represented by several lines depending on the referenced parameter
             // that holds the count number (e.g. number of variograms and the variogram definition lines)
        {
            //the parameter is surely a repeat parameter object
            GSLibParRepeat* rpar = (GSLibParRepeat*) par;
            //retrieve the parameter used to set the count.
            GSLibParType* count_par = this->findParameterByName( rpar->_ref_par_name );
            //get the parameter with the count for the repeat parameter object
            uint replicate_count = 0;
            if( count_par->getTypeName() == "uint" ){
                replicate_count = ((GSLibParUInt*)count_par)->_value;
            }else
                Application::instance()->logError("GSLibParameterFile::setValuesFromParFile(): parameters of type \"" + count_par->getTypeName() + "\" not supported as storing a count.");
            //sets the parameters replication to the number given by the referenced parameter
            rpar->setCount( replicate_count );
            //get the total number of lines that must be read to completely set the parameters' values
            //in the repeat object
            uint number_of_lines = rpar->getTotalParameterCount();
            //create a list with the required parameter file lines
            QStringList lines_to_look_ahead;
            //read the required lines
            for( uint i = 0; i < number_of_lines; ++i )
            {
                lines_to_look_ahead.append( in.readLine() );
            }
            //sets the values of all parameters inside the repeat parameter object
            parseLinesForRepeatObject( lines_to_look_ahead, rpar );
        }
    }

    //tests whether there was a mismatch between file reading and parmeter collection
    if( it != _params.end() || !in.atEnd() ){
        Application::instance()->logWarn("GSLibParameterFile::setValuesFromParFile(): there was a mismatch between parameter collection and parameter lines in the .par file.");
        Application::instance()->logWarn("                                            check possible unnecessary line breaks in the file.");
    }

    //close the parameter file.
    file.close();
}


void GSLibParameterFile::save(const QString path)
{
    QFile file( path );
    file.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&file);
    out << QString("              Parameters for ").append( this->_program_name ) << "\n";
    out << QString("              ******************************** ") << "\n";
    out << "\n";
    out << QString("START OF PARAMETERS:") << "\n";
    QListIterator<GSLibParType*> i( this->_params );
    while (i.hasNext())
        i.next()->save( &out );
    file.close();

    //pixelplt does not work when the .par file lacks the trailing hiphens
    if( this->_program_name.compare("pixelplt") == 0 )
        Util::addTrailingHiphens( path );
}

QString GSLibParameterFile::getProgramName()
{
    return this->_program_name;
}

int GSLibParameterFile::getParameterCount()
{
    return this->_params.size();
}

GSLibParType *GSLibParameterFile::findParameterByName(const QString par_name)
{
    for( int i = 0; i < this->_params.size(); ++i )
        //if the parameter is already a child of the parameter collection
        if( this->_params.at( i )->isNamed( par_name ) )
            return this->_params.at( i );
        //it may be found inside one of the multi-valued parameters
        else if( this->_params.at( i )->isCollection()  ){
            IGSLibParameterFinder* finder = this->_params.at( i )->getFinder();
            if( finder ){
                GSLibParType* result = finder->findParameterByName( par_name );
                //a failed serach does not mean error.  The sought parameter may be in another collection
                if( result )
                    return result;
            } else {
                Application::instance()->logWarn( QString("GSLibParameterFile::findParameterByName(): collection parameters of type \"")\
                                                  .append(this->_params.at( i )->getTypeName())\
                                                  .append("\" are not providing a parameter finder.") );
            }
        }
    Application::instance()->logError( QString("GSLibParameterFile::findParameterByName(): parameter [").append(par_name).append("] not found.") );
    return nullptr;
}

bool GSLibParameterFile::isEmpty()
{
    return _params.empty();
}

bool GSLibParameterFile::isUndefined()
{
    return this->isEmpty();
}

void GSLibParameterFile::setVariogramModel(VariogramModel *vm)
{
    if( _program_name == "gammabar" )
    {
        //Variogram models are stored as parameter files for the vmodel GSLib program,
        //ergo, to read the values, just create a parameter file object for vmodel
        GSLibParameterFile par_vmodel( "vmodel" );
        par_vmodel.setValuesFromParFile( vm->getPath() );
        GSLibParMultiValuedFixed *vm_par3 = par_vmodel.getParameter<GSLibParMultiValuedFixed*>(3);
        GSLibParRepeat *vm_par4 = par_vmodel.getParameter<GSLibParRepeat*>(4); //repeat nst-times

        GSLibParMultiValuedFixed *par2 = getParameter<GSLibParMultiValuedFixed*>(2);
        par2->getParameter<GSLibParUInt*>(0)->_value = vm_par3->getParameter<GSLibParUInt*>(0)->_value; //nst
        par2->getParameter<GSLibParDouble*>(1)->_value = vm_par3->getParameter<GSLibParDouble*>(1)->_value;

        GSLibParRepeat *par3 = getParameter<GSLibParRepeat*>(3); //repeat nst-times
        par3->setCount( par2->getParameter<GSLibParUInt*>(0)->_value );

        for( uint inst = 0; inst < par2->getParameter<GSLibParUInt*>(0)->_value; ++inst)
        {
            GSLibParMultiValuedFixed *par3_0 = par3->getParameter<GSLibParMultiValuedFixed*>(inst, 0);
            GSLibParMultiValuedFixed *vm_par4_0 = vm_par4->getParameter<GSLibParMultiValuedFixed*>(inst, 0);
            par3_0->getParameter<GSLibParOption*>(0)->_selected_value = vm_par4_0->getParameter<GSLibParOption*>(0)->_selected_value;
            par3_0->getParameter<GSLibParDouble*>(1)->_value = vm_par4_0->getParameter<GSLibParDouble*>(1)->_value;
            par3_0->getParameter<GSLibParDouble*>(2)->_value = vm_par4_0->getParameter<GSLibParDouble*>(2)->_value;
            par3_0->getParameter<GSLibParDouble*>(3)->_value = vm_par4_0->getParameter<GSLibParDouble*>(3)->_value;
            par3_0->getParameter<GSLibParDouble*>(4)->_value = vm_par4_0->getParameter<GSLibParDouble*>(4)->_value;
            GSLibParMultiValuedFixed *par3_1 = par3->getParameter<GSLibParMultiValuedFixed*>(inst, 1);
            GSLibParMultiValuedFixed *vm_par4_1 = vm_par4->getParameter<GSLibParMultiValuedFixed*>(inst, 1);
            par3_1->getParameter<GSLibParDouble*>(0)->_value = vm_par4_1->getParameter<GSLibParDouble*>(0)->_value;
            par3_1->getParameter<GSLibParDouble*>(1)->_value = vm_par4_1->getParameter<GSLibParDouble*>(1)->_value;
            par3_1->getParameter<GSLibParDouble*>(2)->_value = vm_par4_1->getParameter<GSLibParDouble*>(2)->_value;
        }
    }
    else
    {
        QString msg("GSLibParameterFile::setVariogramModel(): unsupported GSLib program: ");
        msg.append( this->_program_name );
        Application::instance()->logError( msg );
    }
}

void GSLibParameterFile::saveVariogramModel(const QString vmodel_par_path)
{
    if( _program_name == "kt3d" )
    {
        //make a default vmodel parameters file
        GSLibParameterFile gpf_vmodel( "vmodel" );
        gpf_vmodel.setDefaultValues();

        //copy the variogram model parameters of this kt3d parameter set to it
        gpf_vmodel.copyVariogramModel( this );

        //save the vmodel par file
        gpf_vmodel.save( vmodel_par_path );
    }
    else
    {
        QString msg("GSLibParameterFile::saveVariogramModel(): unsupported GSLib program: ");
        msg.append( this->_program_name );
        Application::instance()->logError( msg );
    }
}

void GSLibParameterFile::copyVariogramModel(GSLibParameterFile *gpf_from)
{
    if( _program_name == "vmodel" && gpf_from->getProgramName() == "kt3d" )
    {
        //get the variogram number of structures and nugget effect variance contribution
        GSLibParMultiValuedFixed *from_par20 = gpf_from->getParameter<GSLibParMultiValuedFixed*>(20);
        GSLibParMultiValuedFixed *my_par3 = getParameter<GSLibParMultiValuedFixed*>(3);
        my_par3->getParameter<GSLibParUInt*>(0)->_value = from_par20->getParameter<GSLibParUInt*>(0)->_value; //nst
        my_par3->getParameter<GSLibParDouble*>(1)->_value = from_par20->getParameter<GSLibParDouble*>(1)->_value;

        //make the necessary copies of variogram structures
        GSLibParRepeat *my_par4 = getParameter<GSLibParRepeat*>(4); //repeat nst-times
        my_par4->setCount( my_par3->getParameter<GSLibParUInt*>(0)->_value );

        //set each variogram structure parameters
        GSLibParRepeat *from_par21 = gpf_from->getParameter<GSLibParRepeat*>(21); //repeated nst-times
        for( uint ist = 0; ist < my_par3->getParameter<GSLibParUInt*>(0)->_value; ++ist)
        {
            GSLibParMultiValuedFixed *from_par21_0 = from_par21->getParameter<GSLibParMultiValuedFixed*>(ist, 0);
            GSLibParMultiValuedFixed *my_par4_0 = my_par4->getParameter<GSLibParMultiValuedFixed*>(ist, 0);
            my_par4_0->getParameter<GSLibParOption*>(0)->_selected_value = from_par21_0->getParameter<GSLibParOption*>(0)->_selected_value;
            my_par4_0->getParameter<GSLibParDouble*>(1)->_value = from_par21_0->getParameter<GSLibParDouble*>(1)->_value;
            my_par4_0->getParameter<GSLibParDouble*>(2)->_value = from_par21_0->getParameter<GSLibParDouble*>(2)->_value;
            my_par4_0->getParameter<GSLibParDouble*>(3)->_value = from_par21_0->getParameter<GSLibParDouble*>(3)->_value;
            my_par4_0->getParameter<GSLibParDouble*>(4)->_value = from_par21_0->getParameter<GSLibParDouble*>(4)->_value;
            GSLibParMultiValuedFixed *from_par21_1 = from_par21->getParameter<GSLibParMultiValuedFixed*>(ist, 1);
            GSLibParMultiValuedFixed *my_par4_1 = my_par4->getParameter<GSLibParMultiValuedFixed*>(ist, 1);
            my_par4_1->getParameter<GSLibParDouble*>(0)->_value = from_par21_1->getParameter<GSLibParDouble*>(0)->_value;
            my_par4_1->getParameter<GSLibParDouble*>(1)->_value = from_par21_1->getParameter<GSLibParDouble*>(1)->_value;
            my_par4_1->getParameter<GSLibParDouble*>(2)->_value = from_par21_1->getParameter<GSLibParDouble*>(2)->_value;
        }
    } else {
        QString msg("GSLibParameterFile::copyVariogramModel(): variogram model copy from a ");
        msg.append( gpf_from->getProgramName() );
        msg.append( " parameter set to a " );
        msg.append( this->_program_name );
        msg.append( " is not currently supported." );
        Application::instance()->logError( msg );
    }
}

void GSLibParameterFile::setGridParameters(CartesianGrid *cg)
{
    QList<GSLibParType*>::iterator it = _params.begin();
    for(; it != _params.end(); ++it){
        GSLibParType* par = *it;
        if( par->getTypeName() == "grid" ){
            GSLibParGrid* parGrid = (GSLibParGrid*)par;
            parGrid->setFromCG( cg );
        }
    }
}

bool GSLibParameterFile::parseType( uint line_indentation, QString tag, QList<GSLibParType*>* params, QString tag_description ){

    QString type_name = Util::getNameFromTag( tag );
    QString var_name = Util::getRefNameFromTag( tag ); //parameters are normally anonymous
    bool hasPlusSign = Util::hasPlusSign( tag ); //the plus (+) sign in a tag denotes a variable length multivalued paramater.

    //create a list of objects representing all parameter types available
    //takes the opportunity to assign the variable name and description to all potential types at once
    QList<GSLibParType*> param_types;
    param_types.append( new GSLibParDouble(var_name,"",tag_description) );
    param_types.append( new GSLibParFile(var_name,"",tag_description) );
    param_types.append( new GSLibParInputData() );
    param_types.append( new GSLibParInt(var_name,"",tag_description) );
    param_types.append( new GSLibParLimitsDouble(var_name,"",tag_description) );
    param_types.append( new GSLibParOption(var_name,"",tag_description) );
    param_types.append( new GSLibParRange(var_name,"",tag_description) );
    param_types.append( new GSLibParString(var_name,"",tag_description) );
    param_types.append( new GSLibParUInt(var_name,"",tag_description) );
    param_types.append( new GSLibParVarWeight(var_name,"",tag_description) );
    param_types.append( new GSLibParGrid(var_name,"",tag_description) );
    param_types.append( new GSLibParColor(var_name,"",tag_description) );
    param_types.append( new GSLibParRepeat() );
    param_types.append( new GSLibParVModel(var_name, "", tag_description) );
    //test all available types
    bool type_recognized = false;
    for ( int i = 0; i < param_types.size(); ++i)
    {
        //parameter type found: add to the object's structure
        if( type_name.compare( param_types[i]->getTypeName() ) == 0){
            //parses the tag options for those types that have them.
            if( type_name == "option" || type_name == "range" || type_name == "repeat" ){
                std::vector< std::pair<QString, QString> > tagOptions = Util::getTagOptions( tag );
                QString reference_par_name = Util::getReferenceName( tag );
                if( type_name == "option" ){
                    GSLibParOption* popt = (GSLibParOption*)param_types[i];
                    for(std::vector< std::pair<QString, QString> >::iterator it = tagOptions.begin(); it != tagOptions.end(); ++it) {
                        std::pair<QString, QString> option = *it;
                        popt->addOption( option.first.toUInt(), option.second );
                    }
                }else if( type_name == "range" ){
                    GSLibParRange* prng = (GSLibParRange*)param_types[i];
                    std::pair<QString, QString> min = tagOptions.front();
                    std::pair<QString, QString> max = tagOptions.back();
                    prng->_min = min.first.toDouble();
                    prng->_min_label = min.second;
                    prng->_max = max.first.toDouble();
                    prng->_max_label = max.second;
                }else if( type_name == "repeat" ){
                    GSLibParRepeat* prep = (GSLibParRepeat*)param_types[i];
                    _last_repeat = prep; //save pointer to the last <repeat> for next subordinate tags
                    prep->_ref_par_name = reference_par_name;
                }
            }
            if( line_indentation > 0 ){
                //if line indentation is not zero, it means that the paramter is under
                // a structure (likely to be a <repeat> tag, so it is added to last processed structure
                // instead of being directly added to the parameter file object
                if( ! _last_repeat )
                    Application::instance()->logError( "ERROR: indented tag without structure (e.g. <repeat> tag)." );
                else{
                    if( ! hasPlusSign ){ //paramater is not variably multivalued
                        if( _last_repeat == param_types[i] )
                            Application::instance()->logError( "ERROR: a <repeat> tag being added as a child parameter of itself.  This is likely caused by a bug or unsupported nesting syntax in the template parameter file." );
                        else
                            _last_repeat->_original_parameters.append( param_types[i] );
                    }
                    else
                        this->addAsMultiValued( &(_last_repeat->_original_parameters), param_types[i] );
                }
            }else{
                //reset last <repeat> if indentation gets back to zero, unless the parameter object to be added
                //is itself a <repeat>
                if( ! param_types[i]->isRepeat() )
                    _last_repeat = nullptr;
                if( ! hasPlusSign ) //paramater is not variably multivalued
                    params->append( param_types[i] );
                else
                    this->addAsMultiValued( params, param_types[i] );
            }
            type_recognized = true;
        }
        //parameter type not found: discard
        else
            delete param_types[i];
    }
    return type_recognized;
}

void GSLibParameterFile::parseParFileLine(const QString line, GSLibParType *par)
{
    if( par->getTypeName() == "file" )
    {
        GSLibParFile* gpar = (GSLibParFile*)par;
        gpar->_path = Util::getValuesFromParFileLine( line );
    }
    else if( par->getTypeName() == "uint" )
    {
        GSLibParUInt* gpar = (GSLibParUInt*)par;
        gpar->_value = Util::getValuesFromParFileLine( line ).toUInt();
    }
    else if( par->getTypeName() == "double" )
    {
        GSLibParDouble* gpar = (GSLibParDouble*)par;
        gpar->_value = Util::getValuesFromParFileLine( line ).toDouble();
    }
    else if( par->getTypeName() == "option" )
    {
        GSLibParOption* gpar = (GSLibParOption*)par;
        gpar->_selected_value = Util::getValuesFromParFileLine( line ).toUInt();
    }
    else if( par->getTypeName() == "string" )
    {
        GSLibParString* gpar = (GSLibParString*)par;
        gpar->_value = Util::getValuesFromParFileLine( line );
    }
    else if( par->getTypeName() == "color" )
    {
        GSLibParColor* gpar = (GSLibParColor*)par;
        gpar->_color_code = Util::getValuesFromParFileLine( line ).toUInt();
    }
    else if( par->getTypeName() == "multivaluedfixed" )
    {
        this->parseParFileLineWithMultipleValues( Util::getValuesFromParFileLine( line ), par );
    }
    else
        Application::instance()->logError("GSLibParameterFile::parseParFileLine(): parameters of type \"" + par->getTypeName() + "\" not supported.");
}

void GSLibParameterFile::parseParFileLineWithMultipleValues(const QString line, GSLibParType *par)
{
    //get a list with each separated value
    //if not used trimmed, the trailing spaces cause split() to return
    //an extra empty element
    QStringList tokens = line.trimmed().split( QRegularExpression("\\s+") );
    //populates the types that are a parameter collection
    if( par->getTypeName() == "multivaluedfixed" )
    {
        //in this type of parameter collection, each atomic parameter is known beforehand
        GSLibParMultiValuedFixed* gpar = (GSLibParMultiValuedFixed*)par;
        //set values to the collection
        QStringListIterator it( tokens );
        QList<GSLibParType*>::iterator it_par = gpar->_parameters.begin();
        while (it.hasNext()){
            //guard against parameter collection overrun
            if( it_par == gpar->_parameters.end() )
            {
                Application::instance()->logError("GSLibParameterFile::parseParFileLineWithMultipleValues(): parameter file line token count and parameter object count mismatch.");
                break;
            }
            //get next token in parameter file line
            QString token = it.next();
            //recursive call to reuse code that parses a single value.
            this->parseParFileLine( token, *(it_par) );
            //move to next parameter object
            ++it_par;
        }
    }
    else{
        Application::instance()->logError("GSLibParameterFile::parseParFileLineWithMultipleValues(): parameters of type \"" + par->getTypeName() + "\" not supported.");
        return;
    }
}

void GSLibParameterFile::parseLinesForRepeatObject(QStringList &lines, GSLibParRepeat *repeat_par)
{
    //first fill in the original parameters
    QList<GSLibParType*>::iterator it = repeat_par->_original_parameters.begin();
    QStringListIterator it_lines( lines );
    uint processed_lines = 0;
    for(; it != repeat_par->_original_parameters.end() && it_lines.hasNext(); ++it, ++processed_lines)
    {
        GSLibParType* par = *it;
        if( ! par->isRepeat() ){ //Parameters of type repeat do not exist as a line in the parameter file
            QString line = it_lines.next();
            this->parseParFileLine( line, par );
        }
        else //parameters of type repeat are represented by several lines depending on the referenced parameter
             // that holds the count number (e.g. number of variograms and the variogram definition lines)
        {
            Application::instance()->logError("GSLibParameterFile::parseLinesForRepeatObject(), 1: Repeat parameters nested in repeart parameters not supported.");
        }
    }
    //then fill in the replicated parameters, if any
    it = repeat_par->_repeated_parameters.begin();
    for(; it != repeat_par->_repeated_parameters.end() && it_lines.hasNext(); ++it, ++processed_lines)
    {
        GSLibParType* par = *it;
        if( ! par->isRepeat() ){ //Parameters of type repeat do not exist as a line in the parameter file
            QString line = it_lines.next();
            this->parseParFileLine( line, par );
        }
        else //parameters of type repeat are represented by several lines depending on the referenced parameter
             // that holds the count number (e.g. number of variograms and the variogram definition lines)
        {
            Application::instance()->logError("GSLibParameterFile::parseLinesForRepeatObject(), 2: Repeat parameters nested in repeart parameters not supported.");
        }
    }
    //test whether the lines passed correspond to one parameter and vice-versa
    if( (int)processed_lines != lines.size() )
    {
        Application::instance()->logWarn("GSLibParameterFile::parseLinesForRepeatObject(): Mismatch between line count and parameter count.");
    }
}

void GSLibParameterFile::parseTemplateLine(const QString line)
{
    //parse the parameter file template line.
    std::pair<QStringList, QString> param_type_and_desc = Util::parseTagsAndDescription(line);

    //get line indentation
    //the line indentation is used to track the scope of a previous <repeat> tag
    //so, lines with an indentation represent parameters that are child of a GSLibParRepeat object
    //instead of being directly added to this object _params collection.
    uint indent = Util::getIndentation( line );

    bool type_recognized = false;

    //if the line is an atomic type (one tag)
    if( param_type_and_desc.first.size() == 1 ){
        type_recognized = parseType( indent, param_type_and_desc.first.first(), &(this->_params), param_type_and_desc.second );
    } else { //if the line is a multi-valued line with a number of atomic types
        ///////for a multi-valued line with a fixed number of types
        //constructs the container type first
        GSLibParMultiValuedFixed* multi = new GSLibParMultiValuedFixed("", "", param_type_and_desc.second);
        type_recognized = true; //assumes all tags will be recognized
        //for each tag found in the multi-valued template line
        for( QStringList::iterator it = param_type_and_desc.first.begin(); it != param_type_and_desc.first.end() ; ++it ){
            //populate the container type with the atomic types
            //pass a line indentation equal to zero, since the line indentation is relative to the parent multivalue parameter object
            GSLibParRepeat *tmp = _last_repeat; //keep the last <repeat> if the multivalued parameter is itself inside a loop
            bool parse_result = parseType(0, *it, &(multi->_parameters), param_type_and_desc.second );
            _last_repeat = tmp; //restore the last <repeat> pointer prior to component parameter type parsing
            //just one unrecognized tag is enough to invalidate the entire line
            if( !parse_result )
                type_recognized = false;
        }
        //add the multivalued parameter container to the proper parameter collection (either directly
        //to this object or as child of some <repeat> parameter object
        if( indent > 0 ){
            //if line indentation is not zero, it means that the paramter is under
            // a structure (likely to be a <repeat> tag, so it is added to last processed structure
            // instead of being directly added to the parameter file object
            if( ! _last_repeat )
                Application::instance()->logError( "ERROR: indented multivalued tag without structure (e.g. <repeat> tag)." );
            else{
                _last_repeat->_original_parameters.append( multi );
            }
        } else {
            //reset last <repeat> if indentation gets back to zero
            _last_repeat = nullptr;
            this->_params.append( multi );
        }
        /////////
    }

    if( ! type_recognized ){
        QString message("GSLibParameterFile::parseTemplateLine(): unrecognized parameter type found in parameter file template ");
        message.append( Application::instance()->getProject()->getTemplatePath( this->_program_name ));
        Application::instance()->logError( message );
        message = "     at line: ";
        message.append( line );
        Application::instance()->logError( message );
    }

}

void GSLibParameterFile::setDefaultValuesForLocmap()
{
    getParameter<GSLibParFile*>(0)->_path = "bogus/path/to/nowhere.dat";

    GSLibParMultiValuedFixed* par1;
    par1 = getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = 1;
    par1->getParameter<GSLibParUInt*>(1)->_value = 2;
    par1->getParameter<GSLibParUInt*>(2)->_value = 3;

    GSLibParMultiValuedFixed* par2;
    par2 = getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParDouble*>(0)->_value = -1.0e21;
    par2->getParameter<GSLibParDouble*>(1)->_value = 1.0e21;

    getParameter<GSLibParFile*>(3)->_path = "foo/foo.ps";

    GSLibParMultiValuedFixed* par4;
    par4 = getParameter<GSLibParMultiValuedFixed*>(4);
    par4->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par4->getParameter<GSLibParDouble*>(1)->_value = 50.0;

    GSLibParMultiValuedFixed* par5;
    par5 = getParameter<GSLibParMultiValuedFixed*>(5);
    par5->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par5->getParameter<GSLibParDouble*>(1)->_value = 50.0;

    getParameter<GSLibParOption*>(6)->_selected_value = 0;
    getParameter<GSLibParOption*>(7)->_selected_value = 0;
    getParameter<GSLibParOption*>(8)->_selected_value = 1;
    getParameter<GSLibParOption*>(9)->_selected_value = 0;

    GSLibParMultiValuedFixed* par10;
    par10 = getParameter<GSLibParMultiValuedFixed*>(10);
    par10->getParameter<GSLibParDouble*>(0)->_value = 0.01;
    par10->getParameter<GSLibParDouble*>(1)->_value = 10.0;
    par10->getParameter<GSLibParDouble*>(2)->_value = 1.0;

    getParameter<GSLibParRange*>(11)->_value = 0.5;

    getParameter<GSLibParString*>(12)->_value = "The Title";
}

void GSLibParameterFile::setDefaultValuesForScatplt()
{
    GSLibParInputData* par0;
    par0 = this->getParameter<GSLibParInputData*>(0);
    par0->_file_with_data._path = "bogus/path/to/nowhere.dat";
    par0->_trimming_limits._max = 1E21;
    par0->_trimming_limits._min = -1E21;
    par0->_var_wgt_pairs.append( new GSLibParVarWeight(1, 0) );
    par0->_var_wgt_pairs.append( new GSLibParVarWeight(2, 0) );

    this->getParameter<GSLibParFile*>(1)->_path = "foo/woo.ps";

    GSLibParMultiValuedFixed* par2 = this->getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par2->getParameter<GSLibParDouble*>(1)->_value = 1.0;
    par2->getParameter<GSLibParOption*>(2)->_selected_value = 0;

    GSLibParMultiValuedFixed* par3 = this->getParameter<GSLibParMultiValuedFixed*>(3);
    par3->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par3->getParameter<GSLibParDouble*>(1)->_value = 1.0;
    par3->getParameter<GSLibParOption*>(2)->_selected_value = 0;

    this->getParameter<GSLibParUInt*>(4)->_value = 1;
    this->getParameter<GSLibParRange*>(5)->_value = 0.1;

    GSLibParLimitsDouble* par6 = this->getParameter<GSLibParLimitsDouble*>(6);
    par6->_min = 0.0;
    par6->_max = 2.0;

    this->getParameter<GSLibParString*>(7)->_value = "The Title";
}

void GSLibParameterFile::setDefaultValuesForPixelplt()
{
    GSLibParInputData* par0;
    par0 = this->getParameter<GSLibParInputData*>(0);
    par0->_file_with_data._path = "bogus/path/to/nowhere.dat";
    par0->_trimming_limits._max = 1E21;
    par0->_trimming_limits._min = -1E21;
    par0->_var_wgt_pairs.append( new GSLibParVarWeight(1, 0) );

    this->getParameter<GSLibParFile*>(1)->_path = "foo/woo.ps";

    this->getParameter<GSLibParUInt*>(2)->_value = 1;

    GSLibParGrid* par3;
    par3 = this->getParameter<GSLibParGrid*>(3);
    par3->_specs_x->getParameter<GSLibParUInt*>(0)->_value = 10; //nx
    par3->_specs_x->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min x
    par3->_specs_x->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size x
    par3->_specs_y->getParameter<GSLibParUInt*>(0)->_value = 10; //ny
    par3->_specs_y->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min y
    par3->_specs_y->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size y
    par3->_specs_z->getParameter<GSLibParUInt*>(0)->_value = 1; //nz
    par3->_specs_z->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min z
    par3->_specs_z->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size z

    this->getParameter<GSLibParOption*>(4)->_selected_value = 1; //1 == projection onto XY plane

    this->getParameter<GSLibParUInt*>(5)->_value = 1;

    this->getParameter<GSLibParString*>(6)->_value = "The Title";

    this->getParameter<GSLibParString*>(7)->_value = "X Label";

    this->getParameter<GSLibParString*>(8)->_value = "Y Label";

    this->getParameter<GSLibParOption*>(9)->_selected_value = 0;

    this->getParameter<GSLibParOption*>(10)->_selected_value = 1;

    this->getParameter<GSLibParOption*>(11)->_selected_value = 0;

    GSLibParMultiValuedFixed* par12 = this->getParameter<GSLibParMultiValuedFixed*>(12);
    par12->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par12->getParameter<GSLibParDouble*>(1)->_value = 1.0;
    par12->getParameter<GSLibParDouble*>(2)->_value = 0.01;

    this->getParameter<GSLibParUInt*>(13)->_value = 2;

    //expand the <repeat> parameter according to the count value given by the referenced parameter
    GSLibParRepeat* par14 = this->getParameter<GSLibParRepeat*>(14);
    GSLibParUInt* ref_parameter = (GSLibParUInt*)this->findParameterByName( par14->_ref_par_name );
    par14->setCount( ref_parameter->_value );

    GSLibParMultiValuedFixed* par14_0 = par14->getParameter<GSLibParMultiValuedFixed*>(0, 0);
    par14_0->getParameter<GSLibParUInt*>(0)->_value = 1;
    par14_0->getParameter<GSLibParColor*>(1)->_color_code = 1;
    par14_0->getParameter<GSLibParString*>(2)->_value = "facies_1";

    GSLibParMultiValuedFixed* par14_1 = par14->getParameter<GSLibParMultiValuedFixed*>(1, 0);
    par14_1->getParameter<GSLibParUInt*>(0)->_value = 2;
    par14_1->getParameter<GSLibParColor*>(1)->_color_code = 2;
    par14_1->getParameter<GSLibParString*>(2)->_value = "facies_2";
}

void GSLibParameterFile::setDefaultValuesForGamv()
{
    this->getParameter<GSLibParFile*>(0)->_path = "";

    GSLibParMultiValuedFixed *par1 = getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = 0;
    par1->getParameter<GSLibParUInt*>(1)->_value = 0;
    par1->getParameter<GSLibParUInt*>(2)->_value = 0;

    GSLibParMultiValuedFixed *par2 = getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParUInt*>(0)->_value = 1;
    GSLibParMultiValuedVariable *par2_1 = par2->getParameter<GSLibParMultiValuedVariable*>(1);
    par2_1->getParameter<GSLibParUInt*>(0)->_value = 0;

    GSLibParMultiValuedFixed *par3 = getParameter<GSLibParMultiValuedFixed*>(3);
    par3->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par3->getParameter<GSLibParDouble*>(1)->_value = 0.0;

    this->getParameter<GSLibParFile*>(4)->_path = "";
    this->getParameter<GSLibParUInt*>(5)->_value = 10;
    this->getParameter<GSLibParDouble*>(6)->_value = 10.0;
    this->getParameter<GSLibParDouble*>(7)->_value = 5.0;
    this->getParameter<GSLibParUInt*>(8)->_value = 1; //ndir

    GSLibParRepeat *par9 = getParameter<GSLibParRepeat*>(9); //repeat ndir-times
    GSLibParMultiValuedFixed *par9_0 = par9->getParameter<GSLibParMultiValuedFixed*>(0, 0);
    par9_0->getParameter<GSLibParDouble*>(0)->_value = 180.0;
    par9_0->getParameter<GSLibParDouble*>(1)->_value = 90.0;
    par9_0->getParameter<GSLibParDouble*>(2)->_value = 10000.0;
    par9_0->getParameter<GSLibParDouble*>(3)->_value = 0.0;
    par9_0->getParameter<GSLibParDouble*>(4)->_value = 22.5;
    par9_0->getParameter<GSLibParDouble*>(5)->_value = 10000.0;

    this->getParameter<GSLibParOption*>(10)->_selected_value = 0;
    this->getParameter<GSLibParUInt*>(11)->_value = 1; //nvarios

    GSLibParRepeat *par12 = getParameter<GSLibParRepeat*>(12); //repeat nvarios-times
    GSLibParMultiValuedFixed *par12_0 = par12->getParameter<GSLibParMultiValuedFixed*>(0, 0);
    par12_0->getParameter<GSLibParUInt*>(0)->_value = 1;
    par12_0->getParameter<GSLibParUInt*>(1)->_value = 1;
    par12_0->getParameter<GSLibParOption*>(2)->_selected_value = 1;
    par12_0->getParameter<GSLibParDouble*>(3)->_value = 0.0;
}

void GSLibParameterFile::setDefaultValuesForVargplt()
{
    this->getParameter<GSLibParFile*>(0)->_path = "";
    this->getParameter<GSLibParUInt*>(1)->_value = 1; // nvarios

    GSLibParMultiValuedFixed *par2 = this->getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par2->getParameter<GSLibParDouble*>(1)->_value = -1.0;

    GSLibParMultiValuedFixed *par3 = this->getParameter<GSLibParMultiValuedFixed*>(3);
    par3->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par3->getParameter<GSLibParDouble*>(1)->_value = -1.0;

    GSLibParMultiValuedFixed *par4 = this->getParameter<GSLibParMultiValuedFixed*>(4);
    par4->getParameter<GSLibParOption*>(0)->_selected_value = 0;
    par4->getParameter<GSLibParDouble*>(1)->_value = 1.0;

    this->getParameter<GSLibParString*>(5)->_value = "";

    GSLibParRepeat *par6 = getParameter<GSLibParRepeat*>(6); //repeat nvarios-times
    par6->getParameter<GSLibParFile*>(0, 0)->_path = "";
    GSLibParMultiValuedFixed *par6_0_1 = par6->getParameter<GSLibParMultiValuedFixed*>(0, 1);
    par6_0_1->getParameter<GSLibParUInt*>(0)->_value = 1;
    par6_0_1->getParameter<GSLibParUInt*>(1)->_value = 1;
    par6_0_1->getParameter<GSLibParOption*>(2)->_selected_value = 1;
    par6_0_1->getParameter<GSLibParOption*>(3)->_selected_value = 1;
    par6_0_1->getParameter<GSLibParColor*>(4)->_color_code = 1;

    this->getParameter<GSLibParFile*>(7)->_path = "no_file";
}

void GSLibParameterFile::setDefaultValuesForVarmap()
{
    getParameter<GSLibParFile*>(0)->_path = "";

    GSLibParMultiValuedFixed *par1 = getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = 1;
    GSLibParMultiValuedVariable *par1_1 = par1->getParameter<GSLibParMultiValuedVariable*>(1);
    par1_1->getParameter<GSLibParUInt*>(0)->_value = 1;

    GSLibParMultiValuedFixed *par2 = getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par2->getParameter<GSLibParDouble*>(1)->_value = -1.0;

    getParameter<GSLibParOption*>(3)->_selected_value = 0;

    GSLibParMultiValuedFixed *par4 = getParameter<GSLibParMultiValuedFixed*>(4);
    par4->getParameter<GSLibParUInt*>(0)->_value = 1;
    par4->getParameter<GSLibParUInt*>(1)->_value = 1;
    par4->getParameter<GSLibParUInt*>(2)->_value = 1;

    GSLibParMultiValuedFixed *par5 = getParameter<GSLibParMultiValuedFixed*>(5);
    par5->getParameter<GSLibParDouble*>(0)->_value = 1.0;
    par5->getParameter<GSLibParDouble*>(1)->_value = 1.0;
    par5->getParameter<GSLibParDouble*>(2)->_value = 1.0;

    GSLibParMultiValuedFixed *par6 = getParameter<GSLibParMultiValuedFixed*>(6);
    par6->getParameter<GSLibParUInt*>(0)->_value = 1;
    par6->getParameter<GSLibParUInt*>(1)->_value = 2;
    par6->getParameter<GSLibParUInt*>(2)->_value = 0;

    getParameter<GSLibParFile*>(7)->_path = "";

    GSLibParMultiValuedFixed *par8 = getParameter<GSLibParMultiValuedFixed*>(8);
    par8->getParameter<GSLibParUInt*>(0)->_value = 10;
    par8->getParameter<GSLibParUInt*>(1)->_value = 10;
    par8->getParameter<GSLibParUInt*>(2)->_value = 0;

    GSLibParMultiValuedFixed *par9 = getParameter<GSLibParMultiValuedFixed*>(9);
    par9->getParameter<GSLibParDouble*>(0)->_value = 5.0;
    par9->getParameter<GSLibParDouble*>(1)->_value = 5.0;
    par9->getParameter<GSLibParDouble*>(2)->_value = 1.0;

    getParameter<GSLibParUInt*>(10)->_value = 5;

    getParameter<GSLibParOption*>(11)->_selected_value = 0;

    getParameter<GSLibParUInt*>(12)->_value = 1; // nvarios

    GSLibParRepeat *par13 = getParameter<GSLibParRepeat*>(13); //repeat nvarios-times
    GSLibParMultiValuedFixed *par13_0 = par13->getParameter<GSLibParMultiValuedFixed*>(0, 0);
    par13_0->getParameter<GSLibParUInt*>(0)->_value = 1;
    par13_0->getParameter<GSLibParUInt*>(1)->_value = 1;
    par13_0->getParameter<GSLibParOption*>(2)->_selected_value = 1;
}

void GSLibParameterFile::setDefaultValuesForGam()
{
    this->getParameter<GSLibParFile*>(0)->_path = "";

    GSLibParMultiValuedFixed *par1 = getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = 1;
    GSLibParMultiValuedVariable *par1_1 = par1->getParameter<GSLibParMultiValuedVariable*>(1);
    par1_1->getParameter<GSLibParUInt*>(0)->_value = 1;

    GSLibParMultiValuedFixed *par2 = getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParDouble*>(0)->_value = -10000.0;
    par2->getParameter<GSLibParDouble*>(1)->_value = 10000.0;

    this->getParameter<GSLibParFile*>(3)->_path = "";
    this->getParameter<GSLibParUInt*>(4)->_value = 1; //realization number

    GSLibParGrid* par5;
    par5 = this->getParameter<GSLibParGrid*>(5);
    par5->_specs_x->getParameter<GSLibParUInt*>(0)->_value = 10; //nx
    par5->_specs_x->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min x
    par5->_specs_x->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size x
    par5->_specs_y->getParameter<GSLibParUInt*>(0)->_value = 10; //ny
    par5->_specs_y->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min y
    par5->_specs_y->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size y
    par5->_specs_z->getParameter<GSLibParUInt*>(0)->_value = 1; //nz
    par5->_specs_z->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min z
    par5->_specs_z->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size z

    GSLibParMultiValuedFixed *par6 = getParameter<GSLibParMultiValuedFixed*>(6);
    par6->getParameter<GSLibParUInt*>(0)->_value = 1; //ndir
    par6->getParameter<GSLibParUInt*>(1)->_value = 10;

    GSLibParRepeat *par7 = getParameter<GSLibParRepeat*>(7); //repeat ndir-times
    GSLibParMultiValuedFixed *par7_0 = par7->getParameter<GSLibParMultiValuedFixed*>(0, 0);
    par7_0->getParameter<GSLibParInt*>(0)->_value = 1;
    par7_0->getParameter<GSLibParInt*>(1)->_value = 0;
    par7_0->getParameter<GSLibParInt*>(2)->_value = 0;

    this->getParameter<GSLibParOption*>(8)->_selected_value = 0;
    this->getParameter<GSLibParUInt*>(9)->_value = 1; //nvarios

    GSLibParRepeat *par10 = getParameter<GSLibParRepeat*>(10); //repeat nvarios-times
    GSLibParMultiValuedFixed *par10_0 = par10->getParameter<GSLibParMultiValuedFixed*>(0, 0);
    par10_0->getParameter<GSLibParUInt*>(0)->_value = 1;
    par10_0->getParameter<GSLibParUInt*>(1)->_value = 1;
    par10_0->getParameter<GSLibParOption*>(2)->_selected_value = 1;
    par10_0->getParameter<GSLibParDouble*>(3)->_value = 0.0;
}

void GSLibParameterFile::setDefaultValuesForDeclus()
{
    this->getParameter<GSLibParFile*>(0)->_path = "foo/woo.ps";

    GSLibParMultiValuedFixed* par1;
    par1 = this->getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = 1;
    par1->getParameter<GSLibParUInt*>(1)->_value = 2;
    par1->getParameter<GSLibParUInt*>(2)->_value = 0;
    par1->getParameter<GSLibParUInt*>(3)->_value = 3;

    GSLibParMultiValuedFixed* par2;
    par2 = this->getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParDouble*>(0)->_value = -1E21;
    par2->getParameter<GSLibParDouble*>(1)->_value = 1E21;

    this->getParameter<GSLibParFile*>(3)->_path = "foo/woo.ps";
    this->getParameter<GSLibParFile*>(4)->_path = "foo/woo.ps";

    GSLibParMultiValuedFixed* par5;
    par5 = this->getParameter<GSLibParMultiValuedFixed*>(5);
    par5->getParameter<GSLibParDouble*>(0)->_value = 1.0;
    par5->getParameter<GSLibParDouble*>(1)->_value = 1.0;

    this->getParameter<GSLibParOption*>(6)->_selected_value = 0;

    GSLibParMultiValuedFixed* par7;
    par7 = this->getParameter<GSLibParMultiValuedFixed*>(7);
    par7->getParameter<GSLibParUInt*>(0)->_value = 24;
    par7->getParameter<GSLibParDouble*>(1)->_value = 1.0;
    par7->getParameter<GSLibParDouble*>(2)->_value = 25.0;

    this->getParameter<GSLibParUInt*>(8)->_value = 5;
}

void GSLibParameterFile::setDefaultValuesForGetPoints()
{
    this->getParameter<GSLibParFile*>(0)->_path = "no file";

    GSLibParMultiValuedFixed* par1;
    par1 = this->getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = 0;
    par1->getParameter<GSLibParDouble*>(1)->_value = 0.0;
    par1->getParameter<GSLibParDouble*>(2)->_value = 0.0;

    GSLibParMultiValuedFixed* par2;
    par2 = this->getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParUInt*>(0)->_value = 0;
    par2->getParameter<GSLibParDouble*>(1)->_value = 0.0;
    par2->getParameter<GSLibParDouble*>(2)->_value = 0.0;

    this->getParameter<GSLibParFile*>(3)->_path = "no file";

    GSLibParMultiValuedFixed* par4;
    par4 = this->getParameter<GSLibParMultiValuedFixed*>(4);
    par4->getParameter<GSLibParUInt*>(0)->_value = 0;
    par4->getParameter<GSLibParUInt*>(1)->_value = 0;

    this->getParameter<GSLibParFile*>(5)->_path = "no file";
}

void GSLibParameterFile::setDefaultValuesForNScoreMV()
{
    this->getParameter<GSLibParFile*>(0)->_path = "well_data.dat";
    this->getParameter<GSLibParUInt*>(1)->_value = 1;

    GSLibParMultiValuedVariable *par2 = this->getParameter<GSLibParMultiValuedVariable*>(2);
    par2->getParameter<GSLibParUInt*>(0)->_value = 1;

    GSLibParMultiValuedVariable *par3 = this->getParameter<GSLibParMultiValuedVariable*>(3);
    par3->getParameter<GSLibParUInt*>(0)->_value = 0;

    GSLibParMultiValuedFixed *par4 = this->getParameter<GSLibParMultiValuedFixed*>(4);
    par4->getParameter<GSLibParDouble*>(0)->_value = -1E21;
    par4->getParameter<GSLibParDouble*>(1)->_value = 1E21;

    this->getParameter<GSLibParFile*>(5)->_path = "nscore.out";

    GSLibParRepeat *par6 = getParameter<GSLibParRepeat*>(6); //repeat nvar-times
    GSLibParFile *par6_0 = par6->getParameter<GSLibParFile*>(0, 0);
    par6_0->_path = "nscore_var1.trn";

    GSLibParRepeat *par7 = getParameter<GSLibParRepeat*>(7); //repeat nvar-times
    GSLibParMultiValuedVariable *par7_0 = par7->getParameter<GSLibParMultiValuedVariable*>(0, 0);
    par7_0->getParameter<GSLibParUInt*>(0)->_value = 0;
    GSLibParFile *par7_1 = par7->getParameter<GSLibParFile*>(0, 1);
    par7_1->_path = "histsmth.out";
}

void GSLibParameterFile::setDefaultValuesForVmodel()
{
    getParameter<GSLibParFile*>(0)->_path = "no-file.var";

    GSLibParMultiValuedFixed *par1 = this->getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = 2; //ndir
    par1->getParameter<GSLibParUInt*>(1)->_value = 40;

    GSLibParRepeat *par2 = getParameter<GSLibParRepeat*>(2); //repeat ndir-times
    GSLibParMultiValuedFixed *par2_0 = par2->getParameter<GSLibParMultiValuedFixed*>(0, 0);
    par2_0->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par2_0->getParameter<GSLibParDouble*>(1)->_value = 0.0;
    par2_0->getParameter<GSLibParDouble*>(2)->_value = 0.5;
    par2->setCount( 2 );
    GSLibParMultiValuedFixed *par2_1 = par2->getParameter<GSLibParMultiValuedFixed*>(1, 0);
    par2_1->getParameter<GSLibParDouble*>(0)->_value = 90.0;
    par2_1->getParameter<GSLibParDouble*>(1)->_value = 0.0;
    par2_1->getParameter<GSLibParDouble*>(2)->_value = 0.5;

    GSLibParMultiValuedFixed *par3 = this->getParameter<GSLibParMultiValuedFixed*>(3);
    par3->getParameter<GSLibParUInt*>(0)->_value = 1; //nst
    par3->getParameter<GSLibParDouble*>(1)->_value = 0.2;

    GSLibParRepeat *par4 = getParameter<GSLibParRepeat*>(4); //repeat nst-times
    GSLibParMultiValuedFixed *par4_0 = par4->getParameter<GSLibParMultiValuedFixed*>(0, 0);
    par4_0->getParameter<GSLibParOption*>(0)->_selected_value = 1;
    par4_0->getParameter<GSLibParDouble*>(1)->_value = 0.8;
    par4_0->getParameter<GSLibParDouble*>(2)->_value = 0.0;
    par4_0->getParameter<GSLibParDouble*>(3)->_value = 0.0;
    par4_0->getParameter<GSLibParDouble*>(4)->_value = 0.0;
    GSLibParMultiValuedFixed *par4_1 = par4->getParameter<GSLibParMultiValuedFixed*>(0, 1);
    par4_1->getParameter<GSLibParDouble*>(0)->_value = 10.0;
    par4_1->getParameter<GSLibParDouble*>(1)->_value = 5.0;
    par4_1->getParameter<GSLibParDouble*>(2)->_value = 1.0;
}

void GSLibParameterFile::setDefaultValuesForHistsmth()
{
    GSLibParInputData* par0;
    par0 = this->getParameter<GSLibParInputData*>(0);
    par0->_file_with_data._path = "bogus/path/to/nowhere.dat";
    par0->_trimming_limits._max = 1E21;
    par0->_trimming_limits._min = -1E21;
    par0->_var_wgt_pairs.append( new GSLibParVarWeight(1, 0) );

    getParameter<GSLibParString*>(1)->_value = "No title";

    getParameter<GSLibParFile*>(2)->_path = "no_file.ps";

    getParameter<GSLibParUInt*>(3)->_value = 30;

    getParameter<GSLibParFile*>(4)->_path = "histsmth.out";

    GSLibParMultiValuedFixed *par5 = this->getParameter<GSLibParMultiValuedFixed*>(5);
    par5->getParameter<GSLibParUInt*>(0)->_value = 100;
    par5->getParameter<GSLibParDouble*>(1)->_value = 0.1;
    par5->getParameter<GSLibParDouble*>(2)->_value = 100.0;

    getParameter<GSLibParOption*>(6)->_selected_value = 0;

    GSLibParMultiValuedFixed *par7 = this->getParameter<GSLibParMultiValuedFixed*>(7);
    par7->getParameter<GSLibParDouble*>(0)->_value = 750;
    par7->getParameter<GSLibParDouble*>(1)->_value = 50;
    par7->getParameter<GSLibParDouble*>(2)->_value = 0.001;
    par7->getParameter<GSLibParUInt*>(3)->_value = 69069;

    GSLibParMultiValuedFixed *par8 = this->getParameter<GSLibParMultiValuedFixed*>(8);
    par8->getParameter<GSLibParOption*>(0)->_selected_value = 1;
    par8->getParameter<GSLibParOption*>(1)->_selected_value = 1;
    par8->getParameter<GSLibParOption*>(2)->_selected_value = 1;
    par8->getParameter<GSLibParOption*>(3)->_selected_value = 1;

    GSLibParMultiValuedFixed *par9 = this->getParameter<GSLibParMultiValuedFixed*>(9);
    par9->getParameter<GSLibParDouble*>(0)->_value = 1.0;
    par9->getParameter<GSLibParDouble*>(1)->_value = 1.0;
    par9->getParameter<GSLibParDouble*>(2)->_value = 2.0;
    par9->getParameter<GSLibParDouble*>(3)->_value = 2.0;

    getParameter<GSLibParDouble*>(10)->_value = 5;

    GSLibParMultiValuedFixed *par11 = this->getParameter<GSLibParMultiValuedFixed*>(11);
    par11->getParameter<GSLibParDouble*>(0)->_value = -999.0;
    par11->getParameter<GSLibParDouble*>(1)->_value = -999.0;

    getParameter<GSLibParUInt*>(12)->_value = 1;

    getParameter<GSLibParUInt*>(13)->_value = 1; //nuserquantiles

    GSLibParRepeat *par14 = getParameter<GSLibParRepeat*>(14); //repeat nuserquantiles-times
    GSLibParMultiValuedFixed *par14_0 = par14->getParameter<GSLibParMultiValuedFixed*>(0, 0);
    par14_0->getParameter<GSLibParDouble*>(0)->_value = 0.5;
    par14_0->getParameter<GSLibParDouble*>(1)->_value = -999;
}

void GSLibParameterFile::setDefaultValuesForScatsmth()
{

    getParameter<GSLibParFile*>(0)->_path = "no_file.dat";

    GSLibParMultiValuedFixed *par1 = this->getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = 1;
    par1->getParameter<GSLibParUInt*>(1)->_value = 2;
    par1->getParameter<GSLibParUInt*>(2)->_value = 0;

    getParameter<GSLibParFile*>(2)->_path = "histsmthX.out";

    GSLibParMultiValuedFixed *par3 = this->getParameter<GSLibParMultiValuedFixed*>(3);
    par3->getParameter<GSLibParUInt*>(0)->_value = 1;
    par3->getParameter<GSLibParUInt*>(1)->_value = 2;

    getParameter<GSLibParFile*>(4)->_path = "histsmthY.out";

    GSLibParMultiValuedFixed *par5 = this->getParameter<GSLibParMultiValuedFixed*>(5);
    par5->getParameter<GSLibParUInt*>(0)->_value = 1;
    par5->getParameter<GSLibParUInt*>(1)->_value = 2;

    GSLibParMultiValuedFixed *par6 = this->getParameter<GSLibParMultiValuedFixed*>(6);
    par6->getParameter<GSLibParOption*>(0)->_selected_value = 0;
    par6->getParameter<GSLibParOption*>(1)->_selected_value = 0;

    getParameter<GSLibParFile*>(7)->_path = "scatsmth.debug";

    getParameter<GSLibParFile*>(8)->_path = "finaldistX.dist";

    getParameter<GSLibParFile*>(9)->_path = "finaldistY.dist";

    getParameter<GSLibParFile*>(10)->_path = "scatsmth.out";

    GSLibParMultiValuedFixed *par11 = this->getParameter<GSLibParMultiValuedFixed*>(11);
    par11->getParameter<GSLibParDouble*>(0)->_value = 150;
    par11->getParameter<GSLibParDouble*>(1)->_value = 1;
    par11->getParameter<GSLibParDouble*>(2)->_value = 0.0001;
    par11->getParameter<GSLibParUInt*>(3)->_value = 69069;

    GSLibParMultiValuedFixed *par12 = this->getParameter<GSLibParMultiValuedFixed*>(12);
    par12->getParameter<GSLibParOption*>(0)->_selected_value = 1;
    par12->getParameter<GSLibParOption*>(1)->_selected_value = 1;
    par12->getParameter<GSLibParOption*>(2)->_selected_value = 1;
    par12->getParameter<GSLibParOption*>(3)->_selected_value = 1;

    GSLibParMultiValuedFixed *par13 = this->getParameter<GSLibParMultiValuedFixed*>(13);
    par13->getParameter<GSLibParDouble*>(0)->_value = 1.0;
    par13->getParameter<GSLibParDouble*>(1)->_value = 1.0;
    par13->getParameter<GSLibParDouble*>(2)->_value = 2.0;
    par13->getParameter<GSLibParDouble*>(3)->_value = 2.0;

    getParameter<GSLibParDouble*>(14)->_value = 25;

    getParameter<GSLibParDouble*>(15)->_value = -999;

    GSLibParMultiValuedFixed *par16 = this->getParameter<GSLibParMultiValuedFixed*>(16);
    par16->getParameter<GSLibParUInt*>(0)->_value = 9;
    par16->getParameter<GSLibParUInt*>(1)->_value = 9;

    getParameter<GSLibParUInt*>(17)->_value = 1;

    GSLibParRepeat *par18 = getParameter<GSLibParRepeat*>(18); //repeat nenvelope-times
    GSLibParMultiValuedFixed *par18_0 = par18->getParameter<GSLibParMultiValuedFixed*>(0, 0);
    par18_0->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par18_0->getParameter<GSLibParDouble*>(1)->_value = 999;
}

void GSLibParameterFile::setDefaultValuesForBivplt()
{
    getParameter<GSLibParFile*>(0)->_path = "data.dat";

    GSLibParMultiValuedFixed *par1 = this->getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = 1;
    par1->getParameter<GSLibParUInt*>(1)->_value = 2;
    par1->getParameter<GSLibParUInt*>(2)->_value = 0;

    GSLibParMultiValuedFixed *par2 = this->getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParOption*>(0)->_selected_value = 0;
    par2->getParameter<GSLibParOption*>(1)->_selected_value = 0;

    GSLibParMultiValuedFixed *par3 = this->getParameter<GSLibParMultiValuedFixed*>(3);
    par3->getParameter<GSLibParDouble*>(0)->_value = -1e21;
    par3->getParameter<GSLibParDouble*>(1)->_value = 1e21;

    getParameter<GSLibParFile*>(4)->_path = "histsmthX.out";

    GSLibParMultiValuedFixed *par5 = this->getParameter<GSLibParMultiValuedFixed*>(5);
    par5->getParameter<GSLibParUInt*>(0)->_value = 1;
    par5->getParameter<GSLibParUInt*>(1)->_value = 2;

    getParameter<GSLibParFile*>(6)->_path = "histsmthY.out";

    GSLibParMultiValuedFixed *par7 = this->getParameter<GSLibParMultiValuedFixed*>(7);
    par7->getParameter<GSLibParUInt*>(0)->_value = 1;
    par7->getParameter<GSLibParUInt*>(1)->_value = 2;

    getParameter<GSLibParFile*>(8)->_path = "scatsmth.out";

    GSLibParMultiValuedFixed *par9 = this->getParameter<GSLibParMultiValuedFixed*>(9);
    par9->getParameter<GSLibParUInt*>(0)->_value = 1;
    par9->getParameter<GSLibParUInt*>(1)->_value = 2;
    par9->getParameter<GSLibParUInt*>(2)->_value = 3;

    getParameter<GSLibParFile*>(10)->_path = "bivplt.ps";

    GSLibParMultiValuedFixed *par11 = this->getParameter<GSLibParMultiValuedFixed*>(11);
    par11->getParameter<GSLibParOption*>(0)->_selected_value = 1;
    par11->getParameter<GSLibParOption*>(1)->_selected_value = 1;

    GSLibParMultiValuedFixed *par12 = this->getParameter<GSLibParMultiValuedFixed*>(12);
    par12->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par12->getParameter<GSLibParDouble*>(1)->_value = 1.0;
    par12->getParameter<GSLibParDouble*>(2)->_value = 0.05;

    getParameter<GSLibParUInt*>(13)->_value = 30;

    getParameter<GSLibParString*>(14)->_value = "no title";
}

void GSLibParameterFile::setDefaultValuesForProbplt()
{
    GSLibParInputData* par0;
    par0 = this->getParameter<GSLibParInputData*>(0);
    par0->_file_with_data._path = "bogus/path/to/nowhere.dat";
    par0->_trimming_limits._max = 1E21;
    par0->_trimming_limits._min = -1E21;
    par0->_var_wgt_pairs.append( new GSLibParVarWeight(1, 0) );

    getParameter<GSLibParFile*>(1)->_path = "probplt.ps";

    getParameter<GSLibParUInt*>(2)->_value = 0;

    getParameter<GSLibParOption*>(3)->_selected_value = 0;

    GSLibParMultiValuedFixed *par4 = this->getParameter<GSLibParMultiValuedFixed*>(4);
    par4->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par4->getParameter<GSLibParDouble*>(1)->_value = 30.0;
    par4->getParameter<GSLibParDouble*>(2)->_value = 5.0;

    getParameter<GSLibParString*>(5)->_value = "no title";
}

void GSLibParameterFile::setDefaultValuesForQpplt()
{

    getParameter<GSLibParFile*>(0)->_path = "nofile.dat";

    GSLibParMultiValuedFixed *par1 = getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = 1;
    par1->getParameter<GSLibParUInt*>(1)->_value = 0;

    getParameter<GSLibParFile*>(2)->_path = "nofile2.dat";

    GSLibParMultiValuedFixed *par3 = getParameter<GSLibParMultiValuedFixed*>(3);
    par3->getParameter<GSLibParUInt*>(0)->_value = 1;
    par3->getParameter<GSLibParUInt*>(1)->_value = 0;

    GSLibParMultiValuedFixed *par4 = this->getParameter<GSLibParMultiValuedFixed*>(4);
    par4->getParameter<GSLibParDouble*>(0)->_value = -1e21;
    par4->getParameter<GSLibParDouble*>(1)->_value = 1e21;

    getParameter<GSLibParFile*>(5)->_path = "output.ps";

    getParameter<GSLibParOption*>(6)->_selected_value = 0;

    getParameter<GSLibParUInt*>(7)->_value = 0;

    GSLibParMultiValuedFixed *par8 = this->getParameter<GSLibParMultiValuedFixed*>(8);
    par8->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par8->getParameter<GSLibParDouble*>(1)->_value = 20.0;

    GSLibParMultiValuedFixed *par9 = this->getParameter<GSLibParMultiValuedFixed*>(9);
    par9->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par9->getParameter<GSLibParDouble*>(1)->_value = 20.0;

    getParameter<GSLibParOption*>(10)->_selected_value = 0;

    getParameter<GSLibParString*>(11)->_value = "no title";
}

void GSLibParameterFile::setDefaultValuesForKt3d()
{

    getParameter<GSLibParFile*>(0)->_path = "nofile.dat";

    GSLibParMultiValuedFixed *par1 = this->getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = 0;
    par1->getParameter<GSLibParUInt*>(1)->_value = 1;
    par1->getParameter<GSLibParUInt*>(2)->_value = 2;
    par1->getParameter<GSLibParUInt*>(3)->_value = 0;
    par1->getParameter<GSLibParUInt*>(4)->_value = 3;
    par1->getParameter<GSLibParUInt*>(5)->_value = 0;

    GSLibParMultiValuedFixed *par2 = this->getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParDouble*>(0)->_value = -1e21;
    par2->getParameter<GSLibParDouble*>(1)->_value = 1e21;

    getParameter<GSLibParOption*>(3)->_selected_value = 0;

    getParameter<GSLibParFile*>(4)->_path = "nofile.dat";

    GSLibParMultiValuedFixed *par5 = this->getParameter<GSLibParMultiValuedFixed*>(5);
    par5->getParameter<GSLibParUInt*>(0)->_value = 0;
    par5->getParameter<GSLibParUInt*>(1)->_value = 0;
    par5->getParameter<GSLibParUInt*>(2)->_value = 0;
    par5->getParameter<GSLibParUInt*>(3)->_value = 0;
    par5->getParameter<GSLibParUInt*>(4)->_value = 0;

    getParameter<GSLibParOption*>(6)->_selected_value = 0;

    getParameter<GSLibParFile*>(7)->_path = "nofile.dat";

    getParameter<GSLibParFile*>(8)->_path = "nofile.dat";

    GSLibParGrid* par9;
    par9 = getParameter<GSLibParGrid*>(9);
    par9->_specs_x->getParameter<GSLibParUInt*>(0)->_value = 10; //nx
    par9->_specs_x->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min x
    par9->_specs_x->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size x
    par9->_specs_y->getParameter<GSLibParUInt*>(0)->_value = 10; //ny
    par9->_specs_y->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min y
    par9->_specs_y->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size y
    par9->_specs_z->getParameter<GSLibParUInt*>(0)->_value = 1; //nz
    par9->_specs_z->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min z
    par9->_specs_z->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size z

    GSLibParMultiValuedFixed *par10 = this->getParameter<GSLibParMultiValuedFixed*>(10);
    par10->getParameter<GSLibParUInt*>(0)->_value = 1;
    par10->getParameter<GSLibParUInt*>(1)->_value = 1;
    par10->getParameter<GSLibParUInt*>(2)->_value = 1;

    GSLibParMultiValuedFixed *par11 = this->getParameter<GSLibParMultiValuedFixed*>(11);
    par11->getParameter<GSLibParUInt*>(0)->_value = 4;
    par11->getParameter<GSLibParUInt*>(1)->_value = 8;

    getParameter<GSLibParUInt*>(12)->_value = 0;

    GSLibParMultiValuedFixed *par13 = this->getParameter<GSLibParMultiValuedFixed*>(13);
    par13->getParameter<GSLibParDouble*>(0)->_value = 100.0;
    par13->getParameter<GSLibParDouble*>(1)->_value = 100.0;
    par13->getParameter<GSLibParDouble*>(2)->_value = 100.0;

    GSLibParMultiValuedFixed *par14 = this->getParameter<GSLibParMultiValuedFixed*>(14);
    par14->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par14->getParameter<GSLibParDouble*>(1)->_value = 0.0;
    par14->getParameter<GSLibParDouble*>(2)->_value = 0.0;

    GSLibParMultiValuedFixed *par15 = this->getParameter<GSLibParMultiValuedFixed*>(15);
    par15->getParameter<GSLibParOption*>(0)->_selected_value = 0;
    par15->getParameter<GSLibParDouble*>(1)->_value = 0.0;

    GSLibParMultiValuedFixed *par16 = this->getParameter<GSLibParMultiValuedFixed*>(16);
    par16->getParameter<GSLibParOption*>(0)->_selected_value = 0;
    par16->getParameter<GSLibParOption*>(1)->_selected_value = 0;
    par16->getParameter<GSLibParOption*>(2)->_selected_value = 0;
    par16->getParameter<GSLibParOption*>(3)->_selected_value = 0;
    par16->getParameter<GSLibParOption*>(4)->_selected_value = 0;
    par16->getParameter<GSLibParOption*>(5)->_selected_value = 0;
    par16->getParameter<GSLibParOption*>(6)->_selected_value = 0;
    par16->getParameter<GSLibParOption*>(7)->_selected_value = 0;
    par16->getParameter<GSLibParOption*>(8)->_selected_value = 0;

    getParameter<GSLibParOption*>(17)->_selected_value = 0;

    getParameter<GSLibParFile*>(18)->_path = "nofile.dat";

    getParameter<GSLibParUInt*>(19)->_value = 0;

    GSLibParMultiValuedFixed *par20 = getParameter<GSLibParMultiValuedFixed*>(20);
    par20->getParameter<GSLibParUInt*>(0)->_value = 1; //nst
    par20->getParameter<GSLibParDouble*>(1)->_value = 0.2;

    GSLibParRepeat *par21 = getParameter<GSLibParRepeat*>(21); //repeat nst-times
    GSLibParMultiValuedFixed *par21_0 = par21->getParameter<GSLibParMultiValuedFixed*>(0, 0);
    par21_0->getParameter<GSLibParOption*>(0)->_selected_value = 1;
    par21_0->getParameter<GSLibParDouble*>(1)->_value = 0.8;
    par21_0->getParameter<GSLibParDouble*>(2)->_value = 0.0;
    par21_0->getParameter<GSLibParDouble*>(3)->_value = 0.0;
    par21_0->getParameter<GSLibParDouble*>(4)->_value = 0.0;
    GSLibParMultiValuedFixed *par21_1 = par21->getParameter<GSLibParMultiValuedFixed*>(0, 1);
    par21_1->getParameter<GSLibParDouble*>(0)->_value = 10.0;
    par21_1->getParameter<GSLibParDouble*>(1)->_value = 5.0;
    par21_1->getParameter<GSLibParDouble*>(2)->_value = 1.0;
}

void GSLibParameterFile::setDefaultValuesForIk3d()
{
    //NOTE: ik3d is known to quit with an error if the file names are the same
    //      even when they are not used, so setting something like NO_FILE for
    //      all unused file parameters will likely make ik3d fail.

    getParameter<GSLibParOption*>(0)->_selected_value = 1;

    getParameter<GSLibParOption*>(1)->_selected_value = 0;

    getParameter<GSLibParFile*>(2)->_path = "jack.dat";

    GSLibParMultiValuedFixed *par3 = getParameter<GSLibParMultiValuedFixed*>(3);
    par3->getParameter<GSLibParUInt*>(0)->_value = 0;
    par3->getParameter<GSLibParUInt*>(1)->_value = 0;
    par3->getParameter<GSLibParUInt*>(2)->_value = 0;
    par3->getParameter<GSLibParUInt*>(3)->_value = 0;

    getParameter<GSLibParUInt*>(4)->_value = 2; //ndist (number of thresholds/categories)

    GSLibParMultiValuedVariable *par5 = getParameter<GSLibParMultiValuedVariable*>(5);
    par5->assure( getParameter<GSLibParUInt*>(4)->_value );
    par5->getParameter<GSLibParDouble*>(0)->_value = 1.0;
    par5->getParameter<GSLibParDouble*>(1)->_value = 2.5;

    GSLibParMultiValuedVariable *par6 = getParameter<GSLibParMultiValuedVariable*>(6);
    par6->assure( getParameter<GSLibParUInt*>(4)->_value );
    par6->getParameter<GSLibParDouble*>(0)->_value = 0.25;
    par6->getParameter<GSLibParDouble*>(1)->_value = 0.65;

    getParameter<GSLibParFile*>(7)->_path = "input.dat";

    GSLibParMultiValuedFixed *par8 = getParameter<GSLibParMultiValuedFixed*>(8);
    par8->getParameter<GSLibParUInt*>(0)->_value = 0;
    par8->getParameter<GSLibParUInt*>(1)->_value = 0;
    par8->getParameter<GSLibParUInt*>(2)->_value = 0;
    par8->getParameter<GSLibParUInt*>(3)->_value = 0;
    par8->getParameter<GSLibParUInt*>(4)->_value = 0;

    getParameter<GSLibParFile*>(9)->_path = "soft.dat";

    GSLibParMultiValuedFixed *par10 = getParameter<GSLibParMultiValuedFixed*>(10);
    par10->getParameter<GSLibParUInt*>(0)->_value = 1;
    par10->getParameter<GSLibParUInt*>(1)->_value = 2;
    par10->getParameter<GSLibParUInt*>(2)->_value = 0;
    GSLibParMultiValuedVariable *par10_3 = par10->getParameter<GSLibParMultiValuedVariable*>(3);
    par10_3->assure( getParameter<GSLibParUInt*>(4)->_value );
    par10_3->getParameter<GSLibParUInt*>(0)->_value = 0;
    par10_3->getParameter<GSLibParUInt*>(1)->_value = 0;

    GSLibParMultiValuedFixed *par11 = getParameter<GSLibParMultiValuedFixed*>(11);
    par11->getParameter<GSLibParDouble*>(0)->_value = -1e21;
    par11->getParameter<GSLibParDouble*>(1)->_value = 1e21;

    getParameter<GSLibParOption*>(12)->_selected_value = 0;

    getParameter<GSLibParFile*>(13)->_path = "debug.out";

    getParameter<GSLibParFile*>(14)->_path = "result.dat";

    GSLibParGrid* par15= getParameter<GSLibParGrid*>(15);
    par15->_specs_x->getParameter<GSLibParUInt*>(0)->_value = 10; //nx
    par15->_specs_x->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min x
    par15->_specs_x->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size x
    par15->_specs_y->getParameter<GSLibParUInt*>(0)->_value = 10; //ny
    par15->_specs_y->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min y
    par15->_specs_y->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size y
    par15->_specs_z->getParameter<GSLibParUInt*>(0)->_value = 1; //nz
    par15->_specs_z->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min z
    par15->_specs_z->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size z

    GSLibParMultiValuedFixed *par16 = this->getParameter<GSLibParMultiValuedFixed*>(16);
    par16->getParameter<GSLibParUInt*>(0)->_value = 4;
    par16->getParameter<GSLibParUInt*>(1)->_value = 8;

    GSLibParMultiValuedFixed *par17 = this->getParameter<GSLibParMultiValuedFixed*>(17);
    par17->getParameter<GSLibParDouble*>(0)->_value = 100.0;
    par17->getParameter<GSLibParDouble*>(1)->_value = 100.0;
    par17->getParameter<GSLibParDouble*>(2)->_value = 100.0;

    GSLibParMultiValuedFixed *par18 = getParameter<GSLibParMultiValuedFixed*>(18);
    par18->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par18->getParameter<GSLibParDouble*>(1)->_value = 0.0;
    par18->getParameter<GSLibParDouble*>(2)->_value = 0.0;

    getParameter<GSLibParUInt*>(19)->_value = 0;

    GSLibParMultiValuedFixed *par20 = getParameter<GSLibParMultiValuedFixed*>(20);
    par20->getParameter<GSLibParOption*>(0)->_selected_value = 0;
    par20->getParameter<GSLibParDouble*>(1)->_value = 0.0;

    getParameter<GSLibParOption*>(21)->_selected_value = 0;

    GSLibParRepeat *par22 = getParameter<GSLibParRepeat*>(22);
    par22->setCount( getParameter<GSLibParUInt*>(4)->_value );
    for(uint i = 0; i < getParameter<GSLibParUInt*>(4)->_value; ++i ){
        GSLibParVModel *par22_i = par22->getParameter<GSLibParVModel*>(i, 0);
        par22_i->makeDefault();
    }
}

void GSLibParameterFile::setDefaultValuesForPostik()
{
    getParameter<GSLibParFile*>(0)->_path = "ik3d_output.dat";
    getParameter<GSLibParFile*>(1)->_path = "postik_output.dat";

    GSLibParMultiValuedFixed *par2 = getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParOption*>(0)->_selected_value = 1;
    par2->getParameter<GSLibParDouble*>(1)->_value = 0.0;

    getParameter<GSLibParUInt*>(3)->_value = 5;

    GSLibParMultiValuedVariable *par4 = getParameter<GSLibParMultiValuedVariable*>(4);
    par4->assure( getParameter<GSLibParUInt*>(3)->_value );
    par4->getParameter<GSLibParDouble*>(0)->_value = 0.5;
    par4->getParameter<GSLibParDouble*>(1)->_value = 1.0;
    par4->getParameter<GSLibParDouble*>(2)->_value = 2.5;
    par4->getParameter<GSLibParDouble*>(3)->_value = 5.0;
    par4->getParameter<GSLibParDouble*>(4)->_value = 10.0;

    GSLibParMultiValuedFixed *par5 = getParameter<GSLibParMultiValuedFixed*>(5);
    par5->getParameter<GSLibParOption*>(0)->_selected_value = 0;
    par5->getParameter<GSLibParOption*>(1)->_selected_value = 1;
    par5->getParameter<GSLibParDouble*>(2)->_value = 0.75;

    getParameter<GSLibParFile*>(6)->_path = "data_for_global_dist.dat";

    GSLibParMultiValuedFixed *par7 = getParameter<GSLibParMultiValuedFixed*>(7);
    par7->getParameter<GSLibParUInt*>(0)->_value = 3;
    par7->getParameter<GSLibParUInt*>(1)->_value = 0;
    par7->getParameter<GSLibParDouble*>(2)->_value = -1e10;
    par7->getParameter<GSLibParDouble*>(3)->_value = 1e10;

    GSLibParMultiValuedFixed *par8 = getParameter<GSLibParMultiValuedFixed*>(8);
    par8->getParameter<GSLibParDouble*>(0)->_value = -1e10;
    par8->getParameter<GSLibParDouble*>(1)->_value = 1e10;

    GSLibParMultiValuedFixed *par9 = getParameter<GSLibParMultiValuedFixed*>(9);
    par9->getParameter<GSLibParOption*>(0)->_selected_value = 1;
    par9->getParameter<GSLibParDouble*>(1)->_value = 1.0;

    GSLibParMultiValuedFixed *par10 = getParameter<GSLibParMultiValuedFixed*>(10);
    par10->getParameter<GSLibParOption*>(0)->_selected_value = 1;
    par10->getParameter<GSLibParDouble*>(1)->_value = 1.0;

    GSLibParMultiValuedFixed *par11 = getParameter<GSLibParMultiValuedFixed*>(11);
    par11->getParameter<GSLibParOption*>(0)->_selected_value = 1;
    par11->getParameter<GSLibParDouble*>(1)->_value = 2.0;

    getParameter<GSLibParUInt*>(12)->_value = 50;
}

void GSLibParameterFile::setDefaultValuesForCokb3d()
{
    //input data file
    getParameter<GSLibParFile*>(0)->_path = "nofile.dat";

    //number of variables (primary + secondaries)
    getParameter<GSLibParUInt*>(1)->_value = 2;

    //columns for X,Y,Z,primary and secondaries
    GSLibParMultiValuedVariable *par2 = getParameter<GSLibParMultiValuedVariable*>(2);
    par2->assure( 3 + 2 );
    par2->getParameter<GSLibParUInt*>(0)->_value = 1;
    par2->getParameter<GSLibParUInt*>(1)->_value = 2;
    par2->getParameter<GSLibParUInt*>(2)->_value = 3;
    par2->getParameter<GSLibParUInt*>(3)->_value = 4;
    par2->getParameter<GSLibParUInt*>(4)->_value = 5;

    //trimming limits
    GSLibParMultiValuedFixed *par3 = this->getParameter<GSLibParMultiValuedFixed*>(3);
    par3->getParameter<GSLibParDouble*>(0)->_value = -1e21;
    par3->getParameter<GSLibParDouble*>(1)->_value = 1e21;

    //cokriging type
    getParameter<GSLibParOption*>(4)->_selected_value = 0;

    //file with co-located secondary data
    getParameter<GSLibParFile*>(5)->_path = "nofile.dat";

    //column with co-located secondary data
    getParameter<GSLibParUInt*>(6)->_value = 1;

    //debug option
    getParameter<GSLibParOption*>(7)->_selected_value = 0;

    //file for debug output
    getParameter<GSLibParFile*>(8)->_path = "nofile.dat";

    //file with estimates output
    getParameter<GSLibParFile*>(9)->_path = "nofile.dat";

    //grid parameters
    GSLibParGrid* par10 = getParameter<GSLibParGrid*>(10);
    par10->_specs_x->getParameter<GSLibParUInt*>(0)->_value = 10; //nx
    par10->_specs_x->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min x
    par10->_specs_x->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size x
    par10->_specs_y->getParameter<GSLibParUInt*>(0)->_value = 10; //ny
    par10->_specs_y->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min y
    par10->_specs_y->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size y
    par10->_specs_z->getParameter<GSLibParUInt*>(0)->_value = 1; //nz
    par10->_specs_z->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min z
    par10->_specs_z->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size z

    //x,y and z block discretization
    GSLibParMultiValuedFixed *par11 = this->getParameter<GSLibParMultiValuedFixed*>(11);
    par11->getParameter<GSLibParUInt*>(0)->_value = 1;
    par11->getParameter<GSLibParUInt*>(1)->_value = 1;
    par11->getParameter<GSLibParUInt*>(2)->_value = 1;

    //min primary, max primary, max secondary data for kriging
    GSLibParMultiValuedFixed *par12 = this->getParameter<GSLibParMultiValuedFixed*>(12);
    par12->getParameter<GSLibParUInt*>(0)->_value = 1;
    par12->getParameter<GSLibParUInt*>(1)->_value = 12;
    par12->getParameter<GSLibParUInt*>(2)->_value = 12;

    //maximum search radii for primary
    GSLibParMultiValuedFixed *par13 = this->getParameter<GSLibParMultiValuedFixed*>(13);
    par13->getParameter<GSLibParDouble*>(0)->_value = 100.0;
    par13->getParameter<GSLibParDouble*>(1)->_value = 100.0;
    par13->getParameter<GSLibParDouble*>(2)->_value = 100.0;

    //maximum search radii for secondaries
    GSLibParMultiValuedFixed *par14 = this->getParameter<GSLibParMultiValuedFixed*>(14);
    par14->getParameter<GSLibParDouble*>(0)->_value = 20.0;
    par14->getParameter<GSLibParDouble*>(1)->_value = 20.0;
    par14->getParameter<GSLibParDouble*>(2)->_value = 20.0;

    //angles for search ellipsoid
    GSLibParMultiValuedFixed *par15 = this->getParameter<GSLibParMultiValuedFixed*>(15);
    par15->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par15->getParameter<GSLibParDouble*>(1)->_value = 0.0;
    par15->getParameter<GSLibParDouble*>(2)->_value = 0.0;

    //kriging option
    getParameter<GSLibParOption*>(16)->_selected_value = 0;

    //means (primary and secondaries)
    GSLibParMultiValuedVariable *par17 = getParameter<GSLibParMultiValuedVariable*>(17);
    par17->assure( 2 );
    par17->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par17->getParameter<GSLibParDouble*>(1)->_value = 0.0;

    //--------auto and cross variograms-------------------------//
    GSLibParRepeat *par18 = getParameter<GSLibParRepeat*>(18);
    par18->setCount( 3 );
    uint i1 = 1;
    uint i2 = 1;
    uint nvars = 2;
    for(uint i = 0; i < 3; ++i ){
        //variable indexes
        GSLibParMultiValuedFixed *par18_ii = par18->getParameter<GSLibParMultiValuedFixed*>(i, 0);
        par18_ii->getParameter<GSLibParUInt*>(0)->_value = i1;
        par18_ii->getParameter<GSLibParUInt*>(1)->_value = i2;
        //variogram model
        GSLibParVModel *par18_i = par18->getParameter<GSLibParVModel*>(i, 1);
        par18_i->makeDefault();
        //compute the variable indexes
        ++i2;
        if( i2 > nvars ){
            ++i1;
            i2 = i1;
            if( i1 > nvars )
                break;
        }
    }
}

void GSLibParameterFile::setDefaultValuesForHistpltsim()
{
    //file with lithology information
    this->getParameter<GSLibParFile*>(0)->_path = "foo/lithology.dat";
    //   lithology column (0=not used), code
    GSLibParMultiValuedFixed* par1 = this->getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = 0;
    par1->getParameter<GSLibParUInt*>(1)->_value = 0;
    //file with reference distribution
    this->getParameter<GSLibParFile*>(2)->_path = "foo/primary.dat";
    //   columns for reference variable and weight
    GSLibParMultiValuedFixed* par3 = this->getParameter<GSLibParMultiValuedFixed*>(3);
    par3->getParameter<GSLibParUInt*>(0)->_value = 3;
    par3->getParameter<GSLibParUInt*>(1)->_value = 0;
    //file with distributions to check
    this->getParameter<GSLibParFile*>(4)->_path = "foo/sgsim.out";
    //   columns for variable and weight
    GSLibParMultiValuedFixed* par5 = this->getParameter<GSLibParMultiValuedFixed*>(5);
    par5->getParameter<GSLibParUInt*>(0)->_value = 1;
    par5->getParameter<GSLibParUInt*>(1)->_value = 0;
    //   data, numeric output
    GSLibParMultiValuedFixed* par6 = this->getParameter<GSLibParMultiValuedFixed*>(6);
    par6->getParameter<GSLibParOption*>(0)->_selected_value = 0;
    par6->getParameter<GSLibParUInt*>(1)->_value = 0;
    //   start and finish histograms (usually 1 and nreal)
    GSLibParMultiValuedFixed* par7 = this->getParameter<GSLibParMultiValuedFixed*>(7);
    par7->getParameter<GSLibParUInt*>(0)->_value = 1;
    par7->getParameter<GSLibParUInt*>(1)->_value = 10;
    //   nx, ny, nz
    GSLibParMultiValuedFixed* par8 = this->getParameter<GSLibParMultiValuedFixed*>(8);
    par8->getParameter<GSLibParUInt*>(0)->_value = 10;
    par8->getParameter<GSLibParUInt*>(1)->_value = 10;
    par8->getParameter<GSLibParUInt*>(2)->_value = 10;
    //   trimming limits
    GSLibParMultiValuedFixed* par9 = this->getParameter<GSLibParMultiValuedFixed*>(9);
    par9->getParameter<GSLibParDouble*>(0)->_value = -1e10;
    par9->getParameter<GSLibParDouble*>(1)->_value = 1e10;
    //file for PostScript output
    this->getParameter<GSLibParFile*>(10)->_path = "foo/histograms.ps";
    //file for summary output (always used)
    this->getParameter<GSLibParFile*>(11)->_path = "foo/summary.txt";
    //file for numeric output (used if flag set above)
    this->getParameter<GSLibParFile*>(12)->_path = "foo/numeric_results.dat";
    //attribute minimum and maximum
    GSLibParMultiValuedFixed* par13 = this->getParameter<GSLibParMultiValuedFixed*>(13);
    par13->getParameter<GSLibParDouble*>(0)->_value = -1e10;
    par13->getParameter<GSLibParDouble*>(1)->_value = 1e10;
    //number of cumulative quantiles to plot (<0 for all)
    this->getParameter<GSLibParInt*>(14)->_value = -1;
    //number of values to use to establish CDF (<0 for all)
    this->getParameter<GSLibParInt*>(15)->_value = -1;
    //0=arithmetic, 1=log scaling
    this->getParameter<GSLibParOption*>(16)->_selected_value = 0;
    //number of decimal places (<0 for auto.)
    this->getParameter<GSLibParInt*>(17)->_value = -1;
    //title
    this->getParameter<GSLibParString*>(18)->_value = "Realizations histograms";
    //positioning of stats (L to R: -1 to 1)
    this->getParameter<GSLibParRange*>(19)->_value = 0.0;
    //reference value for box plot
    this->getParameter<GSLibParDouble*>(20)->_value = 0.0;
}

void GSLibParameterFile::setDefaultValuesForSgsim()
{
    //file with data
    this->getParameter<GSLibParFile*>(0)->_path = "samples.dat";
    //   columns for X,Y,Z,vr,wt,sec.var.
    GSLibParMultiValuedFixed* par1 = this->getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = 1;
    par1->getParameter<GSLibParUInt*>(1)->_value = 2;
    par1->getParameter<GSLibParUInt*>(2)->_value = 0;
    par1->getParameter<GSLibParUInt*>(3)->_value = 3;
    par1->getParameter<GSLibParUInt*>(4)->_value = 0;
    par1->getParameter<GSLibParUInt*>(5)->_value = 0;
    //  trimming limits
    GSLibParMultiValuedFixed* par2 = this->getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParDouble*>(0)->_value = -1e20;
    par2->getParameter<GSLibParDouble*>(1)->_value = 1e20;
    //transform the data (0=no, 1=yes)
    this->getParameter<GSLibParOption*>(3)->_selected_value = 1;
    //   file for output trans table
    this->getParameter<GSLibParFile*>(4)->_path = "transform.trn";
    //   consider ref. dist (0=no, 1=yes)
    this->getParameter<GSLibParOption*>(5)->_selected_value = 0;
    //   file with ref. dist distribution
    this->getParameter<GSLibParFile*>(6)->_path = "histsmth.dat";
    //    columns for vr and wt
    GSLibParMultiValuedFixed* par7 = this->getParameter<GSLibParMultiValuedFixed*>(7);
    par7->getParameter<GSLibParUInt*>(0)->_value = 0;
    par7->getParameter<GSLibParUInt*>(1)->_value = 0;
    //   zmin,zmax(tail extrapolation)
    GSLibParMultiValuedFixed* par8 = this->getParameter<GSLibParMultiValuedFixed*>(8);
    par8->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par8->getParameter<GSLibParDouble*>(1)->_value = 15.0;
    //   lower tail option, parameter
    GSLibParMultiValuedFixed* par9 = this->getParameter<GSLibParMultiValuedFixed*>(9);
    par9->getParameter<GSLibParOption*>(0)->_selected_value = 1;
    par9->getParameter<GSLibParDouble*>(1)->_value = 0;
    //   upper tail option, parameter
    GSLibParMultiValuedFixed* par10 = this->getParameter<GSLibParMultiValuedFixed*>(10);
    par10->getParameter<GSLibParOption*>(0)->_selected_value = 1;
    par10->getParameter<GSLibParDouble*>(1)->_value = 0;
    //debugging level: 0,1,2,3
    this->getParameter<GSLibParOption*>(11)->_selected_value = 0;
    //file for debugging output
    this->getParameter<GSLibParFile*>(12)->_path = "debug.dat";
    //file for simulation output
    this->getParameter<GSLibParFile*>(13)->_path = "sgsim.dat";
    //number of realizations to generate
    this->getParameter<GSLibParUInt*>(14)->_value = 1;
    //nx,xmn,xsiz; ny,ymn,ysiz; nz,zmn,zsiz
    GSLibParGrid* par15 = getParameter<GSLibParGrid*>(15);
    par15->_specs_x->getParameter<GSLibParUInt*>(0)->_value = 10; //nx
    par15->_specs_x->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min x
    par15->_specs_x->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size x
    par15->_specs_y->getParameter<GSLibParUInt*>(0)->_value = 10; //ny
    par15->_specs_y->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min y
    par15->_specs_y->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size y
    par15->_specs_z->getParameter<GSLibParUInt*>(0)->_value = 1; //nz
    par15->_specs_z->getParameter<GSLibParDouble*>(1)->_value = 0.0; //min z
    par15->_specs_z->getParameter<GSLibParDouble*>(2)->_value = 1.0; //cell size z
    //random number seed
    this->getParameter<GSLibParUInt*>(16)->_value = 69069;
    //min and max original data for sim
    GSLibParMultiValuedFixed* par17 = this->getParameter<GSLibParMultiValuedFixed*>(17);
    par17->getParameter<GSLibParUInt*>(0)->_value = 4;
    par17->getParameter<GSLibParUInt*>(1)->_value = 8;
    //number of simulated nodes to use
    this->getParameter<GSLibParUInt*>(18)->_value = 12;
    //assign data to nodes (0=no, 1=yes)
    this->getParameter<GSLibParOption*>(19)->_selected_value = 0;
    //multiple grid search (0=no, 1=yes),num
    GSLibParMultiValuedFixed* par20 = this->getParameter<GSLibParMultiValuedFixed*>(20);
    par20->getParameter<GSLibParOption*>(0)->_selected_value = 0;
    par20->getParameter<GSLibParUInt*>(1)->_value = 0;
    //maximum data per octant (0=not used)
    this->getParameter<GSLibParUInt*>(21)->_value = 0;
    //maximum search radii (hmax,hmin,vert)
    GSLibParMultiValuedFixed* par22 = this->getParameter<GSLibParMultiValuedFixed*>(22);
    par22->getParameter<GSLibParDouble*>(0)->_value = 10.0;
    par22->getParameter<GSLibParDouble*>(1)->_value = 10.0;
    par22->getParameter<GSLibParDouble*>(2)->_value = 10.0;
    //angles for search ellipsoid
    GSLibParMultiValuedFixed* par23 = this->getParameter<GSLibParMultiValuedFixed*>(23);
    par23->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par23->getParameter<GSLibParDouble*>(1)->_value = 0.0;
    par23->getParameter<GSLibParDouble*>(2)->_value = 0.0;
    //size of covariance lookup table
    GSLibParMultiValuedFixed* par24 = this->getParameter<GSLibParMultiValuedFixed*>(24);
    par24->getParameter<GSLibParUInt*>(0)->_value = 25;
    par24->getParameter<GSLibParUInt*>(1)->_value = 25;
    par24->getParameter<GSLibParUInt*>(2)->_value = 5;
    //ktype: 0=SK,1=OK,2=LVM,3=EXDR,4=COLC
    GSLibParMultiValuedFixed* par25 = this->getParameter<GSLibParMultiValuedFixed*>(25);
    par25->getParameter<GSLibParOption*>(0)->_selected_value = 1;
    par25->getParameter<GSLibParDouble*>(1)->_value = 0.6;
    par25->getParameter<GSLibParDouble*>(2)->_value = 1.0;
    //   file with LVM, EXDR, or COLC variable
    this->getParameter<GSLibParFile*>(26)->_path = "seismic.dat";
    //   column for secondary variable
    this->getParameter<GSLibParUInt*>(27)->_value = 0;
    //variogram model
    GSLibParVModel *par28 = this->getParameter<GSLibParVModel*>(28);
    par28->makeDefault();
}

void GSLibParameterFile::addAsMultiValued(QList<GSLibParType *> *params, GSLibParType *parameter)
{
    GSLibParMultiValuedVariable *mvv = new GSLibParMultiValuedVariable( parameter );
    params->append( mvv );
}

void GSLibParameterFile::setDefaultValuesForHistplt()
{
    GSLibParInputData* par0;
    par0 = this->getParameter<GSLibParInputData*>(0);
    par0->_file_with_data._path = "bogus/path/to/nowhere.dat";
    par0->_trimming_limits._max = 1E21;
    par0->_trimming_limits._min = -1E21;
    par0->_var_wgt_pairs.append( new GSLibParVarWeight(1, 0) );

    this->getParameter<GSLibParFile*>(1)->_path = "foo/woo.ps";

    GSLibParMultiValuedFixed* par2;
    par2 = this->getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParDouble*>(0)->_value = 0.0;
    par2->getParameter<GSLibParDouble*>(1)->_value = -1.0;

    this->getParameter<GSLibParDouble*>(3)->_value = -1.0;
    this->getParameter<GSLibParUInt*>(4)->_value = 20;
    this->getParameter<GSLibParOption*>(5)->_selected_value = 0;
    this->getParameter<GSLibParOption*>(6)->_selected_value = 0;
    this->getParameter<GSLibParInt*>(7)->_value = -1.0;
    this->getParameter<GSLibParInt*>(8)->_value = -1.0;
    this->getParameter<GSLibParString*>(9)->_value = "The Title";
    this->getParameter<GSLibParRange*>(10)->_value = 0;
    this->getParameter<GSLibParDouble*>(11)->_value = 0.0;
    //----------------end of histplt parameters--------------------
}

void GSLibParameterFile::generateParameterFileTemplates(const QString directory_path)
{
    QDir dir( directory_path );
    QString par_file_path(dir.absoluteFilePath("histplt.par.tpl"));
    QFile par_file( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for HISTPLT\n";
        out << "                  **********************\n";
        out << '\n';
        out << "START OF PARAMETERS:\n";
        out << "<input>                                           -file with data; -variable and weight; -trimming limits\n";
        out << "<file>                                            -file for PostScript output\n";
        out << "<double> <double>                                 -attribute minimum and maximum\n";
        out << "<double>                                          -frequency maximum (<0 for automatic)\n";
        out << "<uint>                                            -number of classes\n";
        out << "<option [0:arithmetic] [1:log scaling]>           -0=arithmetic, 1=log scaling\n";
        out << "<option [0:frequency] [1:cumulative histogram]>   -0=frequency,  1=cumulative histogram\n";
        out << "<int>                                             -   number of cum. quantiles (<0 for all)\n";
        out << "<int>                                             -number of decimal places (<0 for auto.)\n";
        out << "<string>                                          -title\n";
        out << "<range [-1:L] [1:R]>                              -positioning of stats (L to R: -1 to 1)\n";
        out << "<double>                                          -reference value for box plot\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("locmap.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for LOCMAP\n";
        out << "                  **********************\n";
        out << '\n';
        out << "START OF PARAMETERS:\n";
        out << "<file>                                            -file with data\n";
        out << "<uint> <uint> <uint>                              -   columns for X, Y, variable\n";
        out << "<double> <double>                                 -   trimming limits\n";
        out << "<file>                                            -file for PostScript output\n";
        out << "<double> <double>                                 -xmn,xmx\n";
        out << "<double> <double>                                 -ymn,ymx\n";
        out << "<option [0:data values][1:cross validation]>      -0=data values, 1=cross validation\n";
        out << "<option [0:arithmetic][1:log scaling]>            -0=arithmetic,  1=log scaling\n";
        out << "<option [1:color scale][0:gray scale]>            -0=gray scale,  1=color scale\n";
        out << "<option [0:no labels][1:labels each location]>    -0=no labels,   1=label each location\n";
        out << "<double> <double> <double>                        -gray/color scale: min, max, increm\n";
        out << "<range [0.1:small][10:big]>                       -label size: 0.1(sml)-1(reg)-10(big)\n";
        out << "<string>                                          -Title\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("scatplt.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for SCATPLT\n";
        out << "                  **********************\n";
        out << '\n';
        out << "START OF PARAMETERS:\n";
        out << "<input>                                                        -file with data, -columns for X, Y, wt, third var., -  trimming limits\n";
        out << "<file>                                                         -file for Postscript output\n";
        out << "<double> <double> <option [0:linear scale] [1:log scale]>      -X min and max, (0=arith, 1=log)\n";
        out << "<double> <double> <option [0:linear scale] [1:log scale]>      -Y min and max, (0=arith, 1=log)\n";
        out << "<uint>                                                         -plot every nth data point\n";
        out << "<range [0.1:small][10:big]>                                    -bullet size: 0.1(sml)-1(reg)-10(big)\n";
        out << "<limits_double>                                                -limits for third variable gray scale\n";
        out << "<string>                                                       -title\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("pixelplt.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for PIXELPLT\n";
        out << "                  **********************\n";
        out << '\n';
        out << "START OF PARAMETERS:\n";
        out << "<input>                                                        -file with gridded data, -column number for variable., -  trimming limits\n";
        out << "<file>                                                         -file for Postscript output\n";
        out << "<uint>                                                         -realization number\n";
        out << "<grid>                                                         -nx,xmn,xsiz -nx,xmn,xsiz -nz,zmn,zsiz\n";
        out << "<option [1:XY][2:XZ][3:YZ]>                                    -slice orientation\n";
        out << "<uint>                                                         -slice number\n";
        out << "<string>                                                       -title\n";
        out << "<string>                                                       -X label\n";
        out << "<string>                                                       -Y label\n";
        out << "<option [0:arithmetic scaling][1:log scaling]>                 -0=arithmetic,  1=log scaling\n";
        out << "<option [1:color scale][0:gray scale]>                         -0=gray scale,  1=color scale\n";
        out << "<option [0:continuous][1:categorical]>                         -0=continuous, 1=categorical\n";
        out << "<double> <double> <double>                                     -continuous: min, max, increm\n";
        out << "<uint (ncat)>                                                  -categorical: number of categories\n";
        out << "<repeat [ncat]>\n";
        out << "   <uint> <color> <string>                                     -category, color code, name\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("gamv.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for GAMV\n";
        out << "                  **********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>               -file with data\n";
        out << "<uint> <uint> <uint>              -   columns for X, Y, Z coordinates\n";
        out << "<uint> <uint+>                    -   number of variables,col numbers\n";
        out << "<double> <double>                 -   trimming limits\n";
        out << "<file>                            -file for variogram output\n";
        out << "<uint>                            -number of lags\n";
        out << "<double>                          -lag separation distance\n";
        out << "<double>                          -lag tolerance\n";
        out << "<uint (ndir)>                     -number of directions\n";
        out << "<repeat [ndir]>\n";
        out << "   <double> <double> <double> <double> <double> <double>    -azm,atol,bandh,dip,dtol,bandv\n";
        out << "<option [0:no] [1:yes]>           -standardize sills? (0=no, 1=yes)\n";
        out << "<uint (nvarios)>                  -number of variograms\n";
        out << "<repeat [nvarios]>\n";
        out << "   <uint> <uint> <option [1:traditional semivariogram] \
                   [2:traditional cross semivariogram] \
                   [3:covariance] \
                   [4:correlogram] \
                   [5:general relative semivariogram] \
                   [6:pairwise relative semivariogram] \
                   [7:semivariogram of logarithms] \
                   [8:semimadogram] \
                   [9:indicator semivariogram - continuous] \
                   [10:indicator semivariogram - categorical]> <double>           -tail var., head var., variogram type, indicator threshold\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("vargplt.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for VARGPLT\n";
        out << "                  **********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>                                                                     -file for PostScript output\n";
        out << "<uint (nvarios)>                                                           -number of variograms to plot\n";
        out << "<double> <double>                                                          -distance  limits (from data if max<min)\n";
        out << "<double> <double>                                                          -variogram limits (from data if max<min)\n";
        out << "<option [0:no] [1:yes]> <double>                                           -plot sill (0=no,1=yes), sill value\n";
        out << "<string>                                                                   -Title for variogram\n";
        out << "<repeat [nvarios]>\n";
        out << "   <file>                                                                  -file with variogram data\n";
        out << "   <uint> <uint> <option [0:no] [1:yes]> <option [0:no] [1:yes]> <color>   -  variogram #, dash #, pts?, line?, color\n";
        out << "<file>                                                                     -optional text file for display\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("varmap.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for VARMAP\n";
        out << "                  **********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>                                            -file with data\n";
        out << "<uint> <uint+>                                    -number of variables: column numbers\n";
        out << "<double> <double>                                 -trimming limits\n";
        out << "<option [0:scattered values] [1:regular grid]>    -1=regular grid, 0=scattered values\n";
        out << "<uint> <uint> <uint>                              -if regular data: nx, ny, nz\n";
        out << "<double> <double> <double>                        -if regular data: cell dimensions: dx, dy, dz\n";
        out << "<uint> <uint> <uint>                              -if irregular data: columns for x,y,z\n";
        out << "<file>                                            -file for variogram output\n";
        out << "<uint> <uint> <uint>                              -number of lags: nx, ny, nz\n";
        out << "<double> <double> <double>                        -lags: dx, dy, dz\n";
        out << "<uint>                                            -minimum number of pairs\n";
        out << "<option [0:no] [1:yes]>                           -standardize sills? (0=no, 1=yes)\n";
        out << "<uint (nvarios)>                                  -number of variograms to plot\n";
        out << "<repeat [nvarios]>\n";
        out << "   <uint> <uint> <option [1:traditional semivariogram] \
                   [2:traditional cross semivariogram] \
                   [3:covariance] \
                   [4:correlogram] \
                   [5:general relative semivariogram] \
                   [6:pairwise relative semivariogram] \
                   [7:semivariogram of logarithms]>           -tail var., head var., variogram type\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("gam.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for GAM\n";
        out << "                  ******************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>                            -file with data\n";
        out << "<uint> <uint+>                    -number of variables,column numbers\n";
        out << "<double> <double>                 -trimming limits\n";
        out << "<file>                            -file for variogram output\n";
        out << "<uint>                            -grid or realization number\n";
        out << "<grid>                            -nx,xmn,xsiz -nx,xmn,xsiz -nz,zmn,zsiz\n";
        out << "<uint (ndir)> <uint>              -number of directions, number of lags\n";
        out << "<repeat [ndir]>\n";
        out << "   <int> <int> <int>              -direction as grid steps: sx,sy,sz\n";
        out << "<option [0:no] [1:yes]>           -standardize sill? (0=no, 1=yes)\n";
        out << "<uint (nvarios)>                  -number of variograms\n";
        out << "<repeat [nvarios]>\n";
        out << "   <uint> <uint> <option [1:traditional semivariogram] \
                   [2:traditional cross semivariogram] \
                   [3:covariance] \
                   [4:correlogram] \
                   [5:general relative semivariogram] \
                   [6:pairwise relative semivariogram] \
                   [7:semivariogram of logarithms] \
                   [8:semimadogram] \
                   [9:indicator semivariogram - continuous] \
                   [10:indicator semivariogram - categorical]> <double>           -tail var., head var., variogram type, indicator threshold\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("declus.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for DECLUS\n";
        out << "                  ******************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>                            -file with data\n";
        out << "<uint> <uint> <uint> <uint>       -columns for X, Y, Z, and variable\n";
        out << "<double> <double>                 -trimming limits\n";
        out << "<file>                            -file for summary output\n";
        out << "<file>                            -file for output with data & weights\n";
        out << "<double> <double>                 -Y and Z cell anisotropy (Ysize=size*Yanis in cell declustering)\n";
        out << "<option [0:min] [1:max]>          -look for min/max declustered mean\n";
        out << "<uint> <double> <double>          -number of cell sizes, min size, max size\n";
        out << "<uint>                            -number of origin offsets\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("getpoints.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for GETPOINTS\n";
        out << "                  ************************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>                            -grid data\n";
        out << "<uint> <double> <double>          -nx,xmn,xsiz\n";
        out << "<uint> <double> <double>          -ny,ymn,ysiz\n";
        out << "<file>                            -point data\n";
        out << "<uint> <uint>                     -columns of x y\n";
        out << "<file>                            -output point data with colocated grid data\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("nscoremv.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for NSCOREMV\n";
        out << "                  ************************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>                   -  file with data\n";
        out << "<uint (nvar)>            -  number of variables to transform\n";
        out << "<uint+>                  -  columns for variables\n";
        out << "<uint+>                  -  columns for weights\n";
        out << "<double> <double>        -  trimming limits\n";
        out << "<file>                   -  file for output\n";
        out << "<repeat [nvar]>\n";
        out << "   <file>                -  file for first transformation table\n";
        out << "<repeat [nvar]>\n";
        out << "   <uint+>               -  transform according to ref. dist., column numbers\n";
        out << "   <file>                -  file with reference dist.\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("vmodel.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for VMODEL\n";
        out << "                  *********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>                                            -file for variogram output\n";
        out << "<uint (ndir)> <uint>                              -number of directions and lags\n";
        out << "<repeat [ndir]>\n";
        out << "   <double> <double> <double>                     -azm, dip, lag distance\n";
        out << "<uint (nst)> <double>                             -nst, nugget effect\n";
        out << "<repeat [nst]>\n";
        out << "   <option [1:spheric] [2:exponential] [3:gaussian] [4:power law] [5:cosine hole effect]> <double> <double> <double> <double>     -it,cc,ang1,ang2,ang3\n";
        out << "   <double> <double> <double>                     -a_hmax, a_hmin, a_vert\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("gammabar.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for GAMMABAR\n";
        out << "                  *********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<double> <double> <double>                        -X, Y, Z size of block\n";
        out << "<uint> <uint> <uint>                              -X, Y, Z discretization\n";
        out << "<uint (nst)> <double>                             -nst, nugget effect\n";
        out << "<repeat [nst]>\n";
        out << "   <option [1:spheric] [2:exponential] [3:gaussian] [4:power law] [5:cosine hole effect]> <double> <double> <double> <double>     -it,cc,ang1,ang2,ang3\n";
        out << "   <double> <double> <double>                     -a_hmax, a_hmin, a_vert\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("nscore.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for NSCORE\n";
        out << "                  *********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>                   -file with data\n";
        out << "<uint> <uint>            -columns for variable and weight\n";
        out << "<double> <double>        -trimming limits\n";
        out << "<option [0:no] [1:yes]>  -1=transform according to specified ref. dist.\n";
        out << "<file>                   -file with reference distribution.\n";
        out << "<uint> <uint>            -columns for variable and weight\n";
        out << "<file>                   -file for output\n";
        out << "<file>                   -file for transformation table\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("addcoord.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for ADDCOORD\n";
        out << "                  *********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>                   -file with data\n";
        out << "<file>                   -file for output\n";
        out << "<uint>                   -realization number\n";
        out << "<grid>                   -nx,xmn,xsiz -nx,xmn,xsiz -nz,zmn,zsiz\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("histsmth.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for HISTSMTH\n";
        out << "                  ***********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<input>                                                                                        -file with data; columns for variable and weight; trimming limits\n";
        out << "<string>                                                                                       -title for plot\n";
        out << "<file>                                                                                         -file for PostScript output\n";
        out << "<uint>                                                                                         -number of classes for plot\n";
        out << "<file>                                                                                         -file with smoothed output\n";
        out << "<uint> <double> <double>                                                                       -smoothing limits: number, min, max\n";
        out << "<option [0:arithmetic][1:logarithmic]>                                                         -0=arithmetic, 1=log scaling\n";
        out << "<double> <double> <double> <uint>                                                              -maxpert, reporting interval, min obj, seed\n";
        out << "<option [1:on][0:off]> <option [1:on][0:off]> <option [1:on][0:off]> <option [1:on][0:off]>    -1=on,0=off: mean,var,smth,quantiles\n";
        out << "<double> <double> <double> <double>                                                            -weighting: mean,var,smth,quantiles\n";
        out << "<double>                                                                                       -size of smoothing window\n";
        out << "<double> <double>                                                                              -target mean and variance (-999=>data)\n";
        out << "<uint>                                                                                         -number of quantiles: defined from data.\n";
        out << "<uint (nuserquantiles)>                                                                        -number of quantiles: defined by user.\n";
        out << "<repeat [nuserquantiles]>\n";
        out << "   <double> <double>                                                                           -cdf, value\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("scatsmth.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for SCATSMTH\n";
        out << "                  ***********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>                                                                                         -file with data\n";
        out << "<uint> <uint> <uint>                                                                           -columns for X, Y, weight\n";
        out << "<file>                                                                                         -file with smooth X distribution\n";
        out << "<uint> <uint>                                                                                  -columns for value and probability\n";
        out << "<file>                                                                                         -file with smooth Y distribution\n";
        out << "<uint> <uint>                                                                                  -columns for value and probability\n";
        out << "<option [0:no][1:yes]> <option [0:no][1:yes]>                                                  -log scaling for X, Y variables\n";
        out << "<file>                                                                                         -file for debug information\n";
        out << "<file>                                                                                         -file for final X distribution\n";
        out << "<file>                                                                                         -file for final Y distribution\n";
        out << "<file>                                                                                         -smooth bidistribution output\n";
        out << "<double> <double> <double> <uint>                                                              -maxpert, reporting interval, min obj, seed\n";
        out << "<option [1:on][0:off]> <option [1:on][0:off]> <option [1:on][0:off]> <option [1:on][0:off]>    -1=on,0=off: mean,var,smth,quantiles\n";
        out << "<double> <double> <double> <double>                                                            -weighting: mean,var,smth,quantiles\n";
        out << "<double>                                                                                       -size of smoothing window\n";
        out << "<double>                                                                                       -target correlation (-999=>data)\n";
        out << "<uint> <uint>                                                                                  -number of X, Y quantiles (X, Y).\n";
        out << "<uint (nenvelope)>                                                                             -number of envelope vertexes.\n";
        out << "<repeat [nenvelope]>\n";
        out << "   <double> <double>                                                                           -X, Y coordinate for polygonal.\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("bivplt.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for BIVPLT\n";
        out << "                  ***********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>                                             -file with data\n";
        out << "<uint> <uint> <uint>                               -columns for X, Y, weight\n";
        out << "<option [0:no][1:yes]> <option [0:no][1:yes]>      -log scaling for X, Y variables\n";
        out << "<double> <double>                                  -trimming limits\n";
        out << "<file>                                             -file with smooth X distribution\n";
        out << "<uint> <uint>                                      -columns for value and probability\n";
        out << "<file>                                             -file with smooth Y distribution\n";
        out << "<uint> <uint>                                      -columns for value and probability\n";
        out << "<file>                                             -file with smooth bidistribution\n";
        out << "<uint> <uint> <uint>                               -columns for X value, Y value and probability\n";
        out << "<file>                                             -output PS file\n";
        out << "<option [0:no][1:yes]> <option [1:color][0:B&W]>   -generate pixelmap? color or grayscale\n";
        out << "<double> <double> <double>                         -color scale: prob. min, max, increment\n";
        out << "<uint>                                             -number of classes for the histograms.\n";
        out << "<string>                                           -plot title\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("probplt.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for PROBPLT\n";
        out << "                  ***********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<input>                                    -file with data; columns for variable and weight; trimming limits\n";
        out << "<file>                                     -output PostScript file\n";
        out << "<uint>                                     -number of points to plot (<=0 for all).\n";
        out << "<option [0:arithmetic][1:logarithmic]>     -0=arithmetic, 1=log scaling\n";
        out << "<double> <double> <double>                 -min, max, increment for labeling\n";
        out << "<string>                                   -plot title\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("qpplt.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for QPPLT\n";
        out << "                  ***********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>                                     -file with first data set (X axis)\n";
        out << "<uint> <uint>                              -columns for value and weight\n";
        out << "<file>                                     -file with second data set (Y axis)\n";
        out << "<uint> <uint>                              -columns for value and weight\n";
        out << "<double> <double>                          -trimming limits\n";
        out << "<file>                                     -output PostScript file\n";
        out << "<option [0:Q-Q plot][1:P-P plot]>          -0=Q-Q plot, 1=P-P plot\n";
        out << "<uint>                                     -number of points to plot (<=0 for all).\n";
        out << "<double> <double>                          -X min. and max.\n";
        out << "<double> <double>                          -Y min. and max.\n";
        out << "<option [0:arithmetic][1:logarithmic]>     -0=arithmetic, 1=log scaling\n";
        out << "<string>                                   -plot title\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("kt3d.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for KT3D\n";
        out << "                  ***********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>                                                    -file with data\n";
        out << "<uint> <uint> <uint> <uint> <uint> <uint>                 -   columns for DH_id,X,Y,Z,var,sec var\n";
        out << "<double> <double>                                         -   trimming limits\n";
        out << "<option [0:grid][1:cross validation][2:jackknife]>        -option: 0=grid, 1=cross, 2=jackknife\n";
        out << "<file>                                                    -file with jackknife data\n";
        out << "<uint> <uint> <uint> <uint> <uint>                        -   columns for X,Y,Z,vr and sec var\n";
        out << "<option [0:0][1:1][2:2][3:3]>                             -debugging level: 0,1,2,3\n";
        out << "<file>                                                    -file for debugging output\n";
        out << "<file>                                                    -file for kriged output\n";
        out << "<grid>                                                    -nx,xmn,xsiz; ny,ymn,ysiz; nz,zmn,zsiz\n";
        out << "<uint> <uint> <uint>                                      -x,y and z block discretization\n";
        out << "<uint> <uint>                                             -min, max data for kriging\n";
        out << "<uint>                                                    -max per octant (0-> not used)\n";
        out << "<double> <double> <double>                                -maximum search radii\n";
        out << "<double> <double> <double>                                -angles for search ellipsoid\n";
        out << "<option [0:SK][1:OK][2:LVM][3:KED]> <double>              -0=SK,1=OK,2=non-st SK,3=exdrift;mean for SK\n";
        out << "<option [0:  ][1: x ]> <option [0:  ][1: y ]> <option [0:  ][1: z ]> " << \
               "<option [0:  ][1:x^2]> <option [0:  ][1:y^2]> <option [0:  ][1:z^2]> " << \
               "<option [0:  ][1: xy]> <option [0:  ][1: xz]> <option [0:  ][1: zy]>       -drift: x,y,z,xx,yy,zz,xy,xz,zy\n";
        out << "<option [0:variable][1:trend]>                            -estimate what\n";
        out << "<file>                                                    -gridded file with drift/mean\n";
        out << "<uint>                                                    -  column number in gridded file\n";
        out << "<uint (nst)> <double>                                     -nst, nugget effect\n";
        out << "<repeat [nst]>\n";
        out << "   <option [1:spheric] [2:exponential] [3:gaussian] [4:power law] [5:cosine hole effect]> " << \
               " <double> <double> <double> <double>                      -it,cc,ang1,ang2,ang3\n";
        out << "   <double> <double> <double>                             -a_hmax, a_hmin, a_vert\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("ik3d.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for IK3D\n";
        out << "                  ***********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<option [1:continuous][0:categorical]>        -1=continuous(cdf), 0=categorical(pdf)\n";
        out << "<option [0:grid][1:cross validation][2:jackknife]>        -option: 0=grid, 1=cross, 2=jackknife\n";
        out << "<file>                                                    -file with jackknife data\n";
        out << "<uint> <uint> <uint> <uint>                               -   columns for X,Y,Z,vr\n";
        out << "<uint (ndist)>                                            -number thresholds/categories\n";
        out << "<double+>                                                 -thresholds/categories\n";
        out << "<double+>                                                 -global cdf/pdf\n";
        out << "<file>                                                    -file with data\n";
        out << "<uint> <uint> <uint> <uint> <uint>                        -   columns for DH_id,X,Y,Z,var\n";
        out << "<file>                                                    -file with soft indicator input\n";
        out << "<uint> <uint> <uint> <uint+>                              -   columns for X,Y,Z and indicators\n";
        out << "<double> <double>                                         -   trimming limits\n";
        out << "<option [0:0][1:1][2:2][3:3]>                             -debugging level: 0,1,2,3\n";
        out << "<file>                                                    -file for debugging output\n";
        out << "<file>                                                    -file for kriged output\n";
        out << "<grid>                                                    -nx,xmn,xsiz; ny,ymn,ysiz; nz,zmn,zsiz\n";
        out << "<uint> <uint>                                             -min, max data for kriging\n";
        out << "<double> <double> <double>                                -maximum search radii\n";
        out << "<double> <double> <double>                                -angles for search ellipsoid\n";
        out << "<uint>                                                    -max per octant (0-> not used)\n";
        out << "<option [0:full IK][1:median IK]> <double>                -IK mode: 0=full, 1=median (specify threshold)\n";
        out << "<option [0:SK][1:OK]>                                     -0=SK, 1=OK\n";
        out << "<repeat [ndist]>\n";
        out << "   <vmodel>                                               -variogram model.\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("postik.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for POSTIK\n";
        out << "                  ***********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>                                                                         -file with IK3D output (continuous)\n";
        out << "<file>                                                                         -file for output\n";
        out << "<option [1:E-type][2:Prob. and mean above][3:quantile][4:variance]> <double>   -output option, output parameter\n";
        out << "<uint>                                                                         -number of thresholds\n";
        out << "<double+>                                                                      -the thresholds\n";
        out << "<option [0:No][1:Yes]> <option [1:affine][2:indirect]> <double>                -volume support correction?, type, variance reduction\n";
        out << "<file>                                                                         -file with global distribution\n";
        out << "<uint> <uint> <double> <double>                                                -   ivr,  iwt,  tmin,  tmax\n";
        out << "<double> <double>                                                              -minimum and maximum Z value\n";
        out << "<option [1:linear interp.][2:power model interp.][3:quantiles from data]> <double>  -lower tail: option, parameter\n";
        out << "<option [1:linear interp.][2:power model interp.][3:quantiles from data]> <double>  -middle    : option, parameter\n";
        out << "<option [1:linear interp.][2:power model interp.][3:quantiles from data][4:hyperbolic model interp.]> <double>   -upper tail: option, parameter\n";
        out << "<uint>                                                                         -maximum discretization\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("cokb3d.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for COKB3D\n";
        out << "                  ***********************\n";
        out << '\n';
        out << "START OF PARAMETERS\n";
        out << "<file>                                                    -file with data\n";
        out << "<uint (nvar)>                                             -   number of variables (primary+other)\n";
        out << "<uint+>                                                   -   columns for X,Y,Z,var,sec var(s)\n";
        out << "<double> <double>                                         -   trimming limits\n";
        out << "<option [0:full][1:co-located]>                           -cokriging type.\n";
        out << "<file>                                                    -file with co-located secondary data\n";
        out << "<uint>                                                    -   column with co-located secondary data\n";
        out << "<option [0:0][1:1][2:2][3:3]>                             -debugging level: 0,1,2,3\n";
        out << "<file>                                                    -file for debugging output\n";
        out << "<file>                                                    -file for kriged output\n";
        out << "<grid>                                                    -nx,xmn,xsiz; ny,ymn,ysiz; nz,zmn,zsiz\n";
        out << "<uint> <uint> <uint>                                      -x,y and z block discretization\n";
        out << "<uint> <uint> <uint>                                           -min primary, max primary, max secondary data for kriging\n";
        out << "<double> <double> <double>                                -maximum search radii for primary\n";
        out << "<double> <double> <double>                                -maximum search radii for secondary\n";
        out << "<double> <double> <double>                                -angles for search ellipsoid\n";
        out << "<option [0:SK][1:OK-standardized][2:OK-traditional]>      -0=SK,1=OK,2=OK-trad\n";
        out << "<double+>                                                 -means (primary and secondaries)\n";
        out << "<repeat [nvar]>\n";
        out << "   <uint> <uint>                                          -variable indexes (e.g. 1 ).\n";
        out << "   <vmodel>                                               -variogram model.\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("histpltsim.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for HISTPLTSIM\n";
        out << "                  *************************\n";
        out << '\n';
        out << "START OF PARAMETERS:\n";
        out << "<file>                                              -file with lithology information\n";
        out << "<uint> <uint>                                       -   lithology column (0=not used), code\n";
        out << "<file>                                              -file with reference distribution\n";
        out << "<uint> <uint>                                       -   columns for reference variable and weight\n";
        out << "<file>                                              -file with distributions to check\n";
        out << "<uint> <uint>                                       -   columns for variable and weight\n";
        out << "<option [0:simulation] [1:multi columns]> <uint>    -   data, numeric output\n";
        out << "<uint> <uint>                                       -   start and finish histograms (usually 1 and nreal)\n";
        out << "<uint> <uint> <uint>                                -   nx, ny, nz\n";
        out << "<double> <double>                                   -   trimming limits\n";
        out << "<file>                                              -file for PostScript output\n";
        out << "<file>                                              -file for summary output (always used)\n";
        out << "<file>                                              -file for numeric output (used if flag set above)\n";
        out << "<double> <double>                                   -attribute minimum and maximum\n";
        out << "<int>                                               -number of cumulative quantiles to plot (<0 for all)\n";
        out << "<int>                                               -number of values to use to establish CDF (<0 for all)\n";
        out << "<option [0:arithmetic] [1:log scaling]>             -0=arithmetic, 1=log scaling\n";
        out << "<int>                                               -number of decimal places (<0 for auto.)\n";
        out << "<string>                                            -title\n";
        out << "<range [-1:L] [1:R]>                                -positioning of stats (L to R: -1 to 1)\n";
        out << "<double>                                            -reference value for box plot\n";
    }
    par_file.close();

    par_file_path = dir.absoluteFilePath("sgsim.par.tpl");
    par_file.setFileName( par_file_path );
    if( !par_file.exists() ){
        par_file.open( QFile::WriteOnly | QFile::Text );
        QTextStream out(&par_file);
        out << "                  Parameters for SGSIM\n";
        out << "                  ********************\n";
        out << '\n';
        out << "START OF PARAMETERS:\n";
        out << "<file>                                                -file with data\n";
        out << "<uint><uint><uint><uint><uint><uint>                  -  columns for X,Y,Z,vr,wt,sec.var.\n";
        out << "<double><double>                                      -  trimming limits\n";
        out << "<option [0:no] [1:yes]>                               -transform the data (0=no, 1=yes)\n";
        out << "<file>                                                -  file for output trans table\n";
        out << "<option [0:no] [1:yes]>                               -  consider ref. dist (0=no, 1=yes)\n";
        out << "<file>                                                -  file with ref. dist distribution\n";
        out << "<uint><uint>                                          -  columns for vr and wt\n";
        out << "<double><double>                                      -  zmin,zmax(tail extrapolation)\n";
        out << "<option [1:linear] [2:power]><double>                 -  lower tail option, parameter\n";
        out << "<option [1:linear] [2:power] [4:hyperbolic]><double>  -  upper tail option, parameter\n";
        out << "<option [0:0][1:1][2:2][3:3]>                         -debugging level: 0,1,2,3\n";
        out << "<file>                                                -file for debugging output\n";
        out << "<file>                                                -file for simulation output\n";
        out << "<uint>                                                -number of realizations to generate\n";
        out << "<grid>                                                -nx,xmn,xsiz; ny,ymn,ysiz; nz,zmn,zsiz\n";
        out << "<uint>                                                -random number seed\n";
        out << "<uint><uint>                                          -min and max original data for sim\n";
        out << "<uint>                                                -number of simulated nodes to use\n";
        out << "<option [0:no] [1:yes]>                               -assign data to nodes (0=no, 1=yes)\n";
        out << "<option [0:no] [1:yes]><uint>                         -multiple grid search (0=no, 1=yes),num\n";
        out << "<uint>                                                -maximum data per octant (0=not used)\n";
        out << "<double><double><double>                              -maximum search radii (hmax,hmin,vert)\n";
        out << "<double><double><double>                              -angles for search ellipsoid\n";
        out << "<uint><uint><uint>                                    -size of covariance lookup table\n";
        out << "<option [0:SK] [1:OK] [2:LVM] [3:EXDR] [4:COLC]><double><double>    -ktype: 0=SK,1=OK,2=LVM,3=EXDR,4=COLC\n";
        out << "<file>                                                -  file with LVM, EXDR, or COLC variable\n";
        out << "<uint>                                                -  column for secondary variable\n";
        out << "<vmodel>                                              -variogram model\n";
    }
    par_file.close();
}
