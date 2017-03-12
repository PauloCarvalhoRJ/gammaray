#include "project.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <iostream>
#include "objectgroup.h"
#include <QIcon>
#include "projectroot.h"
#include "datafile.h"
#include "pointset.h"
#include "cartesiangrid.h"
#include "experimentalvariogram.h"
#include "variogrammodel.h"
#include "univariatedistribution.h"
#include "bivariatedistribution.h"
#include <QModelIndexList>
#include "file.h"
#include "../gslib/gslibparameterfiles/gslibparameterfile.h"
#include "util.h"
#include "domain/application.h"
#include "domain/thresholdcdf.h"
#include "domain/categorypdf.h"
#include "domain/categorydefinition.h"
#include "domain/univariatecategoryclassification.h"
#include "plot.h"

Project::Project(const QString path) : QAbstractItemModel()
{
    this->_project_directory = new QDir( path );

    //define the icons (depends on the display resolution
    QIcon iconDataFiles = QIcon(":icons/db16");
    QIcon iconVariogramFiles = QIcon(":icons/vario16");
    QIcon iconDistributionFiles = QIcon(":icons/histo16");
    QIcon iconPlotFiles = QIcon(":icons/plot16");
    QIcon iconResourceFiles = QIcon(":icons/resources16");
    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ){
        iconDataFiles = QIcon(":icons32/db32");
        iconVariogramFiles = QIcon(":icons32/vario32");
        iconDistributionFiles = QIcon(":icons32/histo32");
        iconPlotFiles = QIcon(":icons32/plot32");
        iconResourceFiles = QIcon(":icons32/resources32");
    }

    //build project tree top level structure
    this->_root = new ProjectRoot();
    this->_data_files = new ObjectGroup( "Data Files", iconDataFiles, "DATAFILES");
    this->_data_files->setParent( this->_root );
    this->_root->addChild( this->_data_files );
    this->_variograms = new ObjectGroup( "Variogram Files", iconVariogramFiles, "VARIOGRAMS");
    this->_variograms->setParent( this->_root );
    this->_root->addChild( this->_variograms );
    this->_distributions = new ObjectGroup( "Distribution Files", iconDistributionFiles, "DISTRIBUTIONS");
    this->_distributions->setParent( this->_root );
    this->_root->addChild( this->_distributions );
    this->_plots = new ObjectGroup( "Plot Files", iconPlotFiles, "PLOTS");
    this->_plots->setParent( this->_root );
    this->_root->addChild( this->_plots );
    this->_resources = new ObjectGroup( "Resource Files", iconResourceFiles, "RESOURCES");
    this->_resources->setParent( this->_root );
    this->_root->addChild( this->_resources );

    //load project metadata file, if there is one.
    //make path to gammaray.prj
    QFile prj_file( this->_project_directory->absoluteFilePath( "gammaray.prj" ) );
    //if gammaray.prj exists...
    if( prj_file.exists() ){
        prj_file.open( QFile::ReadOnly | QFile::Text );
        QTextStream in(&prj_file);
        for (int i = 0; !in.atEnd(); ++i)
        {
           QString line = in.readLine();
           //TODO: THE IFs BELOW SIGNAL A REFACTORING OPPORTUNITY
           //found a point set file reference in gammaray.prj
           if( line.startsWith( "POINTSET:" ) ){
               //get file name
                QString point_set_file = line.split(":")[1];
                //make file path
                QFile ps_file( this->_project_directory->absoluteFilePath( point_set_file ) );
                //create point set object from file
                PointSet *ps = new PointSet( ps_file.fileName() );
                //add the object to project tree structure
                this->_data_files->addChild( ps );
                ps->setParent( this->_data_files );
                //reads point set metadata from the .md file
                ps->setInfoFromMetadataFile();
           }
           //found a cartesian grid file reference in gammaray.prj
           if( line.startsWith( "CARTESIANGRID:" ) ){
               //get file name
                QString cartesian_grid_file = line.split(":")[1];
                //make file path
                QFile cg_file( this->_project_directory->absoluteFilePath( cartesian_grid_file ) );
                //create cartesian grid object from file
                CartesianGrid *cg = new CartesianGrid( cg_file.fileName() );
                //add the object to project tree structure
                this->_data_files->addChild( cg );
                cg->setParent( this->_data_files );
                //reads point set metadata from the .md file
                cg->setInfoFromMetadataFile();
           }
           //found a plot file reference in gammaray.prj
           if( line.startsWith( "PLOT:" ) ){
               //get file name
                QString plot_file_name = line.split(":")[1];
                //make file path
                QFile plot_file( this->_project_directory->absoluteFilePath( plot_file_name ) );
                //create plot object from file
                Plot *plot = new Plot( plot_file.fileName() );
                //add the object to project tree structure
                this->_plots->addChild( plot );
                plot->setParent( this->_plots );
           }
           //found an experimental variogram file reference in gammaray.prj
           if( line.startsWith( "EXPVARIOGRAM:" ) ){
               //get file name
                QString exp_var_file_name = line.split(":")[1];
                //make file path
                QFile exp_var_file( this->_project_directory->absoluteFilePath( exp_var_file_name ) );
                //create experimental variogram object from file
                ExperimentalVariogram *exp_var = new ExperimentalVariogram( exp_var_file.fileName() );
                //add the object to project tree structure
                this->_variograms->addChild( exp_var );
                exp_var->setParent( this->_variograms );
                //reads experimental variogram metadata from the .md file
                exp_var->setInfoFromMetadataFile();
           }
           //found variogram model file reference in gammaray.prj
           if( line.startsWith( "VMODEL:" ) ){
               //get file name
                QString exp_var_file_name = line.split(":")[1];
                //make file path
                QFile var_model_file( this->_project_directory->absoluteFilePath( exp_var_file_name ) );
                //create variogram model object from file
                VariogramModel *var_model = new VariogramModel( var_model_file.fileName() );
                //add the object to project tree structure
                this->_variograms->addChild( var_model );
                var_model->setParent( this->_variograms );
           }
           //found a smooth distribution file reference in gammaray.prj
           if( line.startsWith( "UNIDIST:" ) ||
               line.startsWith( "BIDIST:" ) ){
                //get file name
                QString dist_file_name = line.split(":")[1];
                //make file path
                QFile dist_file( this->_project_directory->absoluteFilePath( dist_file_name ) );
                //create distribution object from file
                Distribution *dist;
                if( line.startsWith( "UNIDIST:" ) )
                    dist = new UnivariateDistribution( dist_file.fileName() );
                else
                    dist = new BivariateDistribution( dist_file.fileName() );
                //add the object to project tree structure
                this->_distributions->addChild( dist );
                dist->setParent( this->_distributions );
                //reads the distribution metadata from the .md file
                dist->setInfoFromMetadataFile();
           }
           //found a threshold c.d.f. or category p.d.f. file reference in gammaray.prj
           if( line.startsWith( "CATEGORYPDF:" ) ){
                // get file name and the refered CategoryDefinition
                QStringList file_name_and_category_definition = line.split(":")[1].split(",");
                // get the file name
                QString file_name = file_name_and_category_definition[0];
                // get the category definition name (may be not defined)
                QString category_definition = "FILE_NOT_DEFINED";
                if( file_name_and_category_definition.size() == 2 )
                    category_definition = file_name_and_category_definition[1];
                // make file path
                QFile file_obj( this->_project_directory->absoluteFilePath( file_name ) );
                // create distribution object from file
                CategoryPDF *file = new CategoryPDF( category_definition, file_obj.fileName() );
                // add the object to project tree structure
                this->_resources->addChild( file );
                file->setParent( this->_resources );
           }
           //found a threshold c.d.f. file reference in gammaray.prj
           if( line.startsWith( "THRESHOLDCDF:" ) ){
                //get file name
                QString file_name = line.split(":")[1];
                //make file path
                QFile file_obj( this->_project_directory->absoluteFilePath( file_name ) );
                //create distribution object from file
                ThresholdCDF *file = new ThresholdCDF( file_obj.fileName() );
                //add the object to project tree structure
                this->_resources->addChild( file );
                file->setParent( this->_resources );
           }
           //found a category definition file reference in gammaray.prj
           if( line.startsWith( "CATEGORYDEFINITION:" )){
                //get file name
                QString file_name = line.split(":")[1];
                //make file path
                QFile file_obj( this->_project_directory->absoluteFilePath( file_name ) );
                //create category definition object from file
                File *file = new CategoryDefinition( file_obj.fileName() );
                //add the object to project tree structure
                this->_resources->addChild( file );
                file->setParent( this->_resources );
           }
           //found an univariate classification file reference in gammaray.prj
           if( line.startsWith( "UNIVARIATECATEGORYCLASSIFICATION:" )){
                //get file name and category definition used to create the classification
                QString fileName_and_categoryDef = line.split(":")[1];
                //get the file name
                QString file_name = fileName_and_categoryDef.split(",")[0];
                //get the name of the category defition used to define the classification
                QString cat_def_name = fileName_and_categoryDef.split(",")[1];
                //make file path
                QFile file_obj( this->_project_directory->absoluteFilePath( file_name ) );
                //create category definition object from file
                File *file = new UnivariateCategoryClassification( cat_def_name, file_obj.fileName() );
                //add the object to project tree structure
                this->_resources->addChild( file );
                file->setParent( this->_resources );
           }
        }
        prj_file.close();
    }

    this->save();
}

Project::~Project()
{
    delete this->_project_directory;
    delete this->_data_files;
    delete this->_variograms;
    delete this->_resources;
    delete this->_plots;
    delete this->_distributions;
    delete this->_root;
}

void Project::save()
{
    QFile file( this->_project_directory->absoluteFilePath( "gammaray.prj" ) );
    file.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&file);
    out << APP_NAME << " project file.  This file is generated automatically.  Avoid editing this file.\n";
    out << "version=" << APP_VERSION << '\n';
    this->_data_files->save( &out );
    this->_variograms->save( &out );
    this->_distributions->save( &out );
    this->_plots->save( &out );
    this->_resources->save( &out );
    file.close();

    //ensures all the parameter file templates exist for the GSLib programs necessary for
    //Gamma Ray operation
    QDir tpl_dir = QDir( this->getTemplatesPath() );
    if( ! tpl_dir.exists() ){
        this->_project_directory->mkdir("templates");
    }
    GSLibParameterFile::generateParameterFileTemplates( this->getTemplatesPath() );

    //makes sure the temporary files directory exists
    QDir tmp_dir = QDir( this->getTmpPath() );
    if( ! tmp_dir.exists() ){
        this->_project_directory->mkdir("tmp");
    }
}


QString Project::getName()
{
    return this->_project_directory->dirName();
}

ObjectGroup *Project::getDataFilesGroup()
{
    return this->_data_files;
}

ObjectGroup *Project::getVariogramsGroup()
{
    return this->_variograms;
}

ObjectGroup *Project::getDistributionsGroup()
{
    return this->_distributions;
}

ObjectGroup *Project::getResourcesGroup()
{
    return this->_resources;
}

QString Project::getPath()
{
    return this->_project_directory->canonicalPath();
}

void Project::addDataFile(DataFile *df)
{
    //before enrolling the file, copies it to the
    //project's directory
    df->changeDir( this->_project_directory->canonicalPath() );

    //then remove possible trailing whitespaces from variable names
    //calling renameGEOEASvariable with bogus arguments does this.
    //TODO: The names without the trailing spaces appear only after closing and opening the program as
    //      the metadata are read from the original file.  This can only be solved by changing the original
    //       file, which doesn't seem right.
    Util::renameGEOEASvariable( df->getPath(), "aaaaa", "aaaaa");

    this->_data_files->addChild( df );
    df->setParent( this->_data_files );
    this->save();
}

void Project::addPlot(Plot *plot)
{
    this->_plots->addChild( plot );
    plot->setParent( this->_plots );
    this->save();
}

void Project::addExperimentalVariogram(ExperimentalVariogram *exp_var)
{
    this->_variograms->addChild( exp_var );
    exp_var->setParent( this->_variograms );
    this->save();
}

void Project::addVariogramModel(VariogramModel *var_model)
{
    this->_variograms->addChild( var_model );
    var_model->setParent( this->_variograms );
    this->save();
}

void Project::addDistribution(Distribution *dist)
{
    this->_distributions->addChild( dist );
    dist->setParent( this->_distributions );
    this->save();
}

void Project::addThresholdCDF(ThresholdCDF *tcdf)
{
    this->_resources->addChild( tcdf );
    tcdf->setParent( this->_resources );
    this->save();
}

void Project::addCategoryPDF(CategoryPDF *cpdf)
{
    this->_resources->addChild( cpdf );
    cpdf->setParent( this->_resources );
    this->save();
}

void Project::addResourceFile(File *file)
{
    this->_resources->addChild( file );
    file->setParent( this->_resources );
    this->save();
}

void Project::importPlot(const QString from_path, const QString new_file_name)
{
    //create a new plot object from the generated .ps file
    Plot* plot = new Plot( from_path );
    //copy the .ps file to the project's directory
    plot->changeDir( Application::instance()->getProject()->getPath() );
    //rename the file
    plot->rename( new_file_name );
    //adds the plot object to the project
    //TODO: do not add again if there is already a plot with the same name.
    this->addPlot( plot );
    //refreshes project tree display
    Application::instance()->refreshProjectTree();
}

void Project::importCartesianGrid(CartesianGrid *cg, const QString new_file_name)
{
    CartesianGrid * new_cg = new CartesianGrid( cg->getPath() );
    new_cg->setInfoFromOtherCG( cg );
    new_cg->changeDir( Application::instance()->getProject()->getPath() );
    new_cg->rename( new_file_name );
    //TODO: do not add again if there is already a plot with the same name.
    this->addDataFile( new_cg );
    Application::instance()->refreshProjectTree();
}

void Project::importExperimentalVariogram(const QString from_path,
                                          const QString new_file_name,
                                          const QString vargplt_par_file)
{
    //create a new plot object from the generated .out file
    ExperimentalVariogram* exp_var = new ExperimentalVariogram( from_path );
    //set the peer vargplt file so the user can view it later
    exp_var->setInfo( vargplt_par_file );
    //copy the file to the project's directory
    exp_var->changeDir( Application::instance()->getProject()->getPath() );
    //rename the file
    exp_var->rename( new_file_name );
    //adds the experimental variogram object to the project
    //TODO: do not add again if there is already a plot with the same name.
    this->addExperimentalVariogram( exp_var );
    //refreshes project tree display
    Application::instance()->refreshProjectTree();
}

void Project::importVariogramModel(const QString from_path, const QString new_file_name)
{
    //create a new variogram model object from the generated parameter file
    VariogramModel* var_model = new VariogramModel( from_path );
    //copy the file to the project's directory
    var_model->changeDir( Application::instance()->getProject()->getPath() );
    //rename the file
    var_model->rename( new_file_name );
    //adds the variogram model object to the project
    //TODO: do not add again if there is already a variogam model with the same name.
    this->addVariogramModel( var_model );
    //refreshes project tree display
    Application::instance()->refreshProjectTree();
}

void Project::importUnivariateDistribution(const QString from_path, const QString new_file_name,
                                           const QMap<uint, Roles::DistributionColumnRole>& roles)
{
    //create a new Univariate Distribution object from the given histsmth output file
    UnivariateDistribution* uni_dist = new UnivariateDistribution( from_path );
    //copy the file to the project's directory
    uni_dist->changeDir( Application::instance()->getProject()->getPath() );
    //rename the file
    uni_dist->rename( new_file_name );
    //set column roles (if any)
    uni_dist->setInfo( roles );
    //adds the univariate distribution object to the project
    //TODO: do not add again if there is already a univariate distribution with the same name.
    this->addDistribution( uni_dist );
    //refreshes project tree display
    Application::instance()->refreshProjectTree();
}

void Project::importBivariateDistribution(const QString from_path, const QString new_file_name, const QMap<uint, Roles::DistributionColumnRole> &roles)
{
    //create a new Bivariate Distribution object from the given scatsmth output file
    BivariateDistribution* bi_dist = new BivariateDistribution( from_path );
    //copy the file to the project's directory
    bi_dist->changeDir( Application::instance()->getProject()->getPath() );
    //rename the file
    bi_dist->rename( new_file_name );
    //set column roles (if any)
    bi_dist->setInfo( roles );
    //adds the bivariate distribution object to the project
    //TODO: do not add again if there is already a univariate distribution with the same name.
    this->addDistribution( bi_dist );
    //refreshes project tree display
    Application::instance()->refreshProjectTree();
}

void Project::registerThresholdCDF(ThresholdCDF *tcdf)
{
    this->addThresholdCDF( tcdf );
    //refreshes project tree display
    Application::instance()->refreshProjectTree();
}

void Project::registerCategoryPDF(CategoryPDF *cpdf)
{
    this->addCategoryPDF( cpdf );
    //refreshes project tree display
    Application::instance()->refreshProjectTree();
}

void Project::registerFileAsResource(File *file)
{
    this->addResourceFile( file );
    //refreshes project tree display
    Application::instance()->refreshProjectTree();
}

QModelIndexList Project::getPersistentIndexList()
{
    return this->persistentIndexList();
}

QModelIndexList Project::getIndexList(const QModelIndex &parent)
{
    QModelIndexList retval;
    int rowCount = this->rowCount(parent);
    for(int i = 0; i < rowCount; ++i)
    {
        QModelIndex idx = this->index(i, 0, parent);
        if(idx.isValid())
        {
            retval << idx;
            retval << this->getIndexList(idx);
        }
    }
    return retval;
}

void Project::removeFile(File *file, bool delete_the_file)
{
    _data_files->removeChild( file );
    _plots->removeChild( file );
    _variograms->removeChild( file );
    _distributions->removeChild( file );
    _resources->removeChild( file );
    if( delete_the_file )
        file->deleteFromFS();
}

QString Project::getTemplatesPath()
{
    return this->_project_directory->absoluteFilePath("templates");
}

QString Project::getTemplatePath(const QString program_name)
{
    QString path = this->getTemplatesPath();
    QDir templates_dir(path);
    QString file_name;
    file_name.append( program_name );
    file_name.append( ".par.tpl" );
    return templates_dir.absoluteFilePath( file_name );
}

QString Project::getTmpPath()
{
    return this->_project_directory->absoluteFilePath("tmp");
}

QString Project::generateUniqueTmpFilePath(const QString file_extension)
{
    while(true){
        int r = ( (int)((double)rand() / RAND_MAX * 10000000)) + 10000000;
        QString filename = QString::number(r);
        filename.append(".");
        filename.append(file_extension);
        QDir dir(getTmpPath());
        QFile file(dir.absoluteFilePath(filename));
        if( ! file.exists() )
            return dir.absoluteFilePath(filename);
    }
}

bool Project::fileExists( const QString filename )
{
    QDir proj_dir( getPath() );
    QFile full_path( proj_dir.absoluteFilePath( filename ) );
    return full_path.exists();
}

//-------------- QAbstractItemModel interface------------
QModelIndex Project::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    ProjectComponent *parentItem;

    if (!parent.isValid())
        parentItem = this->_root;
    else
        parentItem = static_cast<ProjectComponent*>(parent.internalPointer());

    ProjectComponent *childItem = parentItem->getChildByIndex( row );
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}
QModelIndex Project::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();
    ProjectComponent *childItem = static_cast<ProjectComponent*>(child.internalPointer());
    ProjectComponent *parentItem = childItem->getParent();
    if (parentItem == this->_root)
        return QModelIndex();
    return createIndex(parentItem->getIndexInParent(), 0, parentItem);
}
int Project::rowCount(const QModelIndex &parent) const
{
    ProjectComponent *parentItem;
    if (parent.column() > 0)
        return 0;
    if (!parent.isValid())
        parentItem = this->_root;
    else
        parentItem = static_cast<ProjectComponent*>(parent.internalPointer());
    return parentItem->getChildCount();
}
int Project::columnCount(const QModelIndex &/*parent*/) const
{
    return 1;
}
QVariant Project::data(const QModelIndex &index, int role) const
{
     if (!index.isValid())
         return QVariant();
     ProjectComponent *item = static_cast<ProjectComponent*>(index.internalPointer());
     if( role == Qt::DisplayRole )
        return QVariant( item->getPresentationName() );
     if( role == Qt::DecorationRole )
        return QVariant( item->getIcon() );
     return QVariant();
}
Qt::ItemFlags Project::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}
QVariant Project::headerData(int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const
{
    //if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    //    return rootItem->data(section);
    return QVariant();
}
//-------------------------end of QAbstractItemModel interface------------------------------------

/*QString readFile() {
    QFile file("h:/test.txt");
    if (!file.open(QFile::ReadOnly | QFile::Text)) return "";
    QTextStream in(&file);
    QString temp = in.readAll();
    file.close();
    return temp;
} */
