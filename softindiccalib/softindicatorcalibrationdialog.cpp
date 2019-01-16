#include "softindicatorcalibrationdialog.h"
#include "ui_softindicatorcalibrationdialog.h"

#include "domain/application.h"
#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/datafile.h"
#include "domain/categorydefinition.h"
#include "domain/thresholdcdf.h"
#include "domain/categorypdf.h"
#include "domain/project.h"
#include "domain/pointset.h"
#include "domain/cartesiangrid.h"
#include "widgets/fileselectorwidget.h"
#include "dialogs/valuespairsdialog.h"
#include "softindicatorcalibplot.h"
#include "util.h"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <cmath>

SoftIndicatorCalibrationDialog::SoftIndicatorCalibrationDialog(Attribute *at, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SoftIndicatorCalibrationDialog),
    m_at( at ),
    m_softIndCalibPlot( nullptr )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle("Soft indicator calibration for " + at->getContainingFile()->getName() + "/" + at->getName());

    //add a category definition file selection drop down menu
    m_fsw = new FileSelectorWidget( FileSelectorType::CDsCDFsandPDFs, true );
    ui->frmTopBar->layout()->addWidget( m_fsw );
    connect( m_fsw, SIGNAL(fileSelected(File*)),
             this, SLOT(onUpdateNumberOfCalibrationCurves()) );

    //add a spacer for better layout
    QHBoxLayout *hl = (QHBoxLayout*)(ui->frmTopBar->layout());
    hl->addStretch();

    //add the widget used to edit the calibration curves
    m_softIndCalibPlot = new SoftIndicatorCalibPlot(this);
    ui->frmCalib->layout()->addWidget( m_softIndCalibPlot );

    //get the Attribute's data file
    File *file = m_at->getContainingFile();
    if( file->isDataFile() ){
        DataFile *dataFile = dynamic_cast<DataFile*>(file);
        //load the data
        dataFile->loadData();
        //create an array of doubles to store the Attribute's values
        std::vector<double> data;
        // get the total number of file records
        uint nData = dataFile->getDataLineCount();
        //pre-allocate the array of doubles
        data.reserve( nData );
        //get the Attribute's GEO-EAS index
        uint atGEOEASIndex = m_at->getAttributeGEOEASgivenIndex();
        //fills the array of doubles with the data from file
        for( uint i = 0; i < nData; ++i){
            double value = dataFile->data( i, atGEOEASIndex-1 );
            //adds only if the value is not a No-Data-Value
            if( ! dataFile->isNDV( value ) ){
                data.push_back( value );
            }
        }
        //move the array of doubles to the widget (it'll no longer be availabe here)
        m_softIndCalibPlot->transferData( data );
        //set the variable name as the x-axis label.
        m_softIndCalibPlot->setXAxisLabel( at->getName() );
    }

    adjustSize();
}

SoftIndicatorCalibrationDialog::~SoftIndicatorCalibrationDialog()
{
    delete ui;
    Application::instance()->logInfo("SoftIndicatorCalibrationDialog destroyed.");
}

void SoftIndicatorCalibrationDialog::onUpdateNumberOfCalibrationCurves()
{
    File *selectedFile = m_fsw->getSelectedFile();
    if( selectedFile ){
        selectedFile->readFromFS();
        int nCategories = selectedFile->getContentsCount();
        if( selectedFile->getFileType() == "THRESHOLDCDF"){
            ui->btnGlobalPDF->hide();
            ui->btnGlobalCDF->show();
            m_softIndCalibPlot->setNumberOfCurves( selectedFile->getContentsCount() );
            //sets the curve labels with each threshold
            ThresholdCDF *cdf = (ThresholdCDF*)selectedFile;
            for( int i = 0; i < nCategories; ++i ){
                m_softIndCalibPlot->setCurveLabel(i, "thr. = " + QString::number( cdf->get1stValue( i ), 'g', 12 ) );
                QRgb rgb = static_cast<QRgb>( (uint)( (double) std::rand() / (double)RAND_MAX * (1U<<31)) );
                m_softIndCalibPlot->setCurveColor( i, QColor( rgb ));
                m_softIndCalibPlot->setCurveBase( i, cdf->get2ndValue(i) * 100.0);
            }
        }else if( selectedFile->getFileType() == "CATEGORYDEFINITION"){
            ui->btnGlobalPDF->show();
            ui->btnGlobalCDF->hide();
            //for categorical variables the calibration curves separate the categories, thus -1.
            m_softIndCalibPlot->setNumberOfCurves( nCategories-1 );
            //fills the areas between the curves with the colors of the categories
            CategoryDefinition *cd = (CategoryDefinition*)selectedFile;
            for( int i = 0; i < nCategories; ++i ){
                m_softIndCalibPlot->fillColor( Util::getGSLibColor( cd->getColorCode(i) ) ,
                                               i-1,
                                               cd->getCategoryName( i ));
            }
        }else if( selectedFile->getFileType() == "CATEGORYPDF"){
            ui->btnGlobalPDF->show();
            ui->btnGlobalCDF->hide();
            //for categorical variables the calibration curves separate the categories, thus -1.
            m_softIndCalibPlot->setNumberOfCurves( nCategories-1 );
            //fills the areas between the curves with the colors of the categories
            CategoryPDF *pdf = (CategoryPDF*)selectedFile;
            CategoryDefinition *cd = pdf->getCategoryDefinition();
            cd->loadQuintuplets();
            double curveBase = 0.0;
            for( int i = 0; i < nCategories; ++i ){
                m_softIndCalibPlot->fillColor( Util::getGSLibColor( cd->getCategoryColorByCode( pdf->get1stValue( i ) ) ) ,
                                               i-1,
                                               cd->getCategoryNameByCode( pdf->get1stValue( i ) ) );
                if( i > 0 )
                    m_softIndCalibPlot->setCurveBase( i-1, curveBase * 100.0);
                curveBase += pdf->get2ndValue( i );
            }
            m_softIndCalibPlot->updateFillAreas();
        }
    }
}

void SoftIndicatorCalibrationDialog::onSave()
{
    File *selectedFile = m_fsw->getSelectedFile();
    if( !selectedFile ){
        QMessageBox::critical( this, "Error", "Please, select a file containing categories or thresholds.");
        return;
    }

    //get the Attribute's data file
    File *dataFile = m_at->getContainingFile();

	//ask the user for the name to the new data set file
    bool ok = false;
	QString proposed_name = dataFile->getName().append("_SOFT.dat");
	QString new_name = QInputDialog::getText(this, "Name the new data set file (" + dataFile->getFileType() + ")",
											 "New data set (" + dataFile->getFileType() + ") file name with soft indicators:", QLineEdit::Normal,
                                             proposed_name, &ok);
    if(!ok) return;

	//get the temporary data file with the computed soft indicators
	QString fileWithSoftIndicators = saveTmpFileWithSoftIndicators();

	//rename the output point set file
	QString newDataSetPath = Util::renameFile( fileWithSoftIndicators, new_name );

	if( dataFile->getFileType() == "POINTSET" ){

        //assuming the data file is a point set file
        PointSet* psData = (PointSet*)dataFile;

        //create a new point set object corresponding to a file created in the tmp directory
		PointSet* ps = new PointSet( newDataSetPath );

        //set the new point set metadata
		ps->setInfo( psData->getXindex(), psData->getYindex(), psData->getZindex(), psData->getNoDataValue() );

        //adds the point set to the project
        Application::instance()->getProject()->addDataFile( ps );

        //refreshes the project tree
        Application::instance()->refreshProjectTree();

	} else if( dataFile->getFileType() == "CARTESIANGRID" ){

		//assuming the data file is a Cartesian grid file
		CartesianGrid* cgData = (CartesianGrid*)dataFile;

		//create a new Cartesian grid object corresponding to a file created in the tmp directory
		CartesianGrid* cg = new CartesianGrid( newDataSetPath );

		//set the new Cartesian grid metadata
		cg->setInfoFromOtherCG( cgData );

		//adds the Cartesiangrid to the project
		Application::instance()->getProject()->addDataFile( cg );

		//refreshes the project tree
		Application::instance()->refreshProjectTree();

	}
}

void SoftIndicatorCalibrationDialog::onPreview()
{
    File *selectedFile = m_fsw->getSelectedFile();
    if( !selectedFile ){
        QMessageBox::critical( this, "Error", "Please, select a file containing categories or thresholds.");
        return;
    }

    //get the Attribute's data file
    File *dataFile = m_at->getContainingFile();

    //get the temporary data file with the computed soft indicators
    QString fileWithSoftIndicators = saveTmpFileWithSoftIndicators();

    if( dataFile->getFileType() == "POINTSET" ){
		//create a new point set object corresponding to a file to be created in the tmp directory
        PointSet* ps = new PointSet( fileWithSoftIndicators );

        //set the new point set metadata
        ps->setInfo( 1, 2, 3, (dynamic_cast<DataFile*>(dataFile))->getNoDataValue() );

        //get the number of direct children (assumed to be all fields found in the point set file)
        uint nFields = ps->getChildCount();

        //3: skip X, Y and Z variables
        //display only the soft indicators
        for( uint i = 3; i < nFields; ++i){
            //get the variable with the soft indicator
            Attribute* softIndicator = (Attribute*)ps->getChildByIndex( i );
            //open the plot dialog
            Util::viewPointSet( softIndicator, this );
        }
	} else if( dataFile->getFileType() == "CARTESIANGRID" ){
		//create a new grid object corresponding to a file to be created in the tmp directory
		CartesianGrid* cg = new CartesianGrid( fileWithSoftIndicators );

		//set the new grid metadata
		cg->setInfoFromOtherCG( dynamic_cast<CartesianGrid*>(dataFile) );

		//get the number of data columns (variables)
		uint nFields = cg->getDataColumnCount();

		for( uint i = 0; i < nFields; ++i){
			//get the variable with the soft indicator
			Attribute* softIndicator = (Attribute*)cg->getAttributeFromGEOEASIndex( i + 1 );
			//open the plot dialog
			Util::viewGrid( softIndicator, this );
		}
	}

    //TODO: destroy the created object pointed to by ps.
}

void SoftIndicatorCalibrationDialog::onResultedGlobalPDF()
{
    File *selectedFile = m_fsw->getSelectedFile();
    if( !selectedFile ){
        QMessageBox::critical( this, "Error", "Please, select a file containing categories or thresholds.");
        return;
    }

    //get the category definition associated with the selected file with the source of categories
    CategoryDefinition *selectedCategoryDefinition = nullptr;
    if( selectedFile->getFileType() == "CATEGORYDEFINITION"){
        selectedCategoryDefinition = (CategoryDefinition*)selectedFile;
    }else if( selectedFile->getFileType() == "CATEGORYPDF"){
        CategoryPDF *pdf = (CategoryPDF*)selectedFile;
        selectedCategoryDefinition = pdf->getCategoryDefinition();
    }

    //verify the CategoryDefinition
    if( ! selectedCategoryDefinition ){
        Application::instance()->logError("SoftIndicatorCalibrationDialog::onResultedGlobalPDF(): null CategoryDefinition.  Aborting.");
        return;
    }

    //get the Attribute's data file
    File *dataFile = m_at->getContainingFile();

	//get the temporary data file with the computed soft indicators
	QString fileWithSoftIndicators = saveTmpFileWithSoftIndicators();

	//create a p.d.f. object to store the global
	CategoryPDF* globalPDF = new CategoryPDF( selectedCategoryDefinition, "");

	if( dataFile->getFileType() == "POINTSET" ){

        //assuming the data file is a point set file
        PointSet* psData = (PointSet*)dataFile;

        //create a new point set object corresponding to a file created in the tmp directory
        PointSet* ps = new PointSet( fileWithSoftIndicators );

        //set the new point set metadata
        ps->setInfo( 1, 2, 3, psData->getNoDataValue() );

        //loads the soft indicator data
        ps->loadData();

        //get the number of computed soft indicator variables (number of data columns - 3 for the x,y,z coordinates)
        uint nSoftIndicators = ps->getDataColumnCount() - 3;

        //for each soft indicator
        for( uint i = 0; i < nSoftIndicators; ++i){
            //get its mean through the resulted data file
            double mean = ps->mean( 3 + i );
            //added the pair category code-probability to the p.d.f. object
            globalPDF->addPair( selectedCategoryDefinition->getCategoryCode( i ), mean );
        }

	}else if( dataFile->getFileType() == "CARTESIANGRID" ){

		//assuming the data file is a grid file
		CartesianGrid* cgData = (CartesianGrid*)dataFile;

		//create a new grid object corresponding to a file created in the tmp directory
		CartesianGrid* cg = new CartesianGrid( fileWithSoftIndicators );

		//set the new grid metadata
		cg->setInfoFromOtherCG( cgData );

		//loads the soft indicator data
		cg->loadData();

		//get the number of computed soft indicator variables (number of data columns)
		uint nSoftIndicators = cg->getDataColumnCount();

		//for each soft indicator
		for( uint i = 0; i < nSoftIndicators; ++i){
			//get its mean through the resulted data file
			double mean = cg->mean( i );
			//added the pair category code-probability to the p.d.f. object
			globalPDF->addPair( selectedCategoryDefinition->getCategoryCode( i ), mean );
		}

	}

	//show the p.d.f. editor to display the computed global frequencies
	ValuesPairsDialog* vpd = new ValuesPairsDialog( globalPDF, this );
	vpd->show(); //show dialog non-modally
}

void SoftIndicatorCalibrationDialog::onResultedGlobalCDF()
{
    File *selectedFile = m_fsw->getSelectedFile();
    if( !selectedFile ){
        QMessageBox::critical( this, "Error", "Please, select a file containing categories or thresholds.");
        return;
    }

    //get the category definition associated with the selected file with the source of categories
    ThresholdCDF *cdf = nullptr;
    if( selectedFile->getFileType() == "THRESHOLDCDF"){
        cdf = (ThresholdCDF*)selectedFile;
    }

    //get the Attribute's data file
    File *dataFile = m_at->getContainingFile();

	//get the temporary data file with the computed soft indicators
	QString fileWithSoftIndicators = saveTmpFileWithSoftIndicators();

	//create a c.d.f. object to store the global cumulative probabilities
	ThresholdCDF* globalCDF = new ThresholdCDF( "" );

	if( dataFile->getFileType() == "POINTSET" ){

        //assuming the data file is a point set file
        PointSet* psData = (PointSet*)dataFile;

        //create a new point set object corresponding to a file created in the tmp directory
        PointSet* ps = new PointSet( fileWithSoftIndicators );

        //set the new point set metadata
        ps->setInfo( 1, 2, 3, psData->getNoDataValue() );

        //loads the soft indicator data
        ps->loadData();

        //get the number of computed soft indicator variables (number of data columns - 3 for the x,y,z coordinates)
        uint nSoftIndicators = ps->getDataColumnCount() - 3;

        //for each soft indicator
        for( uint i = 0; i < nSoftIndicators; ++i){
            //get its mean through the resulted data file
            double mean = ps->mean( 3 + i );
            //added the pair category code-probability to the p.d.f. object
            globalCDF->addPair( cdf->get1stValue( i ) , mean );
        }

	} else if( dataFile->getFileType() == "CARTESIANGRID" ){

		//assuming the data file is a grid file
		CartesianGrid* cgData = (CartesianGrid*)dataFile;

		//create a new point set object corresponding to a file created in the tmp directory
		CartesianGrid* cg = new CartesianGrid( fileWithSoftIndicators );

		//set the new point set metadata
		cg->setInfoFromOtherCG( cgData );

		//loads the soft indicator data
		cg->loadData();

		//get the number of computed soft indicator variables (number of data columns)
		uint nSoftIndicators = cg->getDataColumnCount();

		//for each soft indicator
		for( uint i = 0; i < nSoftIndicators; ++i){
			//get its mean through the resulted data file
			double mean = cg->mean( i );
			//added the pair category code-probability to the p.d.f. object
			globalCDF->addPair( cdf->get1stValue( i ) , mean );
		}

	}

	//show the c.d.f. editor to display the computed global cumulative frequencies
	ValuesPairsDialog* vpd = new ValuesPairsDialog( globalCDF, this );
	vpd->show(); //show dialog non-modally

	//TODO: delete globalCDF somewhere
}

QString SoftIndicatorCalibrationDialog::saveTmpFileWithSoftIndicators()
{
    SoftIndicatorCalculationMode calcMode = SoftIndicatorCalculationMode::CATEGORICAL;
    File *selectedFile = m_fsw->getSelectedFile();
    if( selectedFile ){

        //define the soft indicator type
        if( selectedFile->getFileType() == "THRESHOLDCDF")
            calcMode = SoftIndicatorCalculationMode::CONTINUOUS;

        //compute the soft indicators (outer vector = each soft indicator; inner vector = values)
        std::vector< std::vector< double > > softIndicators = m_softIndCalibPlot->getSoftIndicators( calcMode );

        //get the number of computed soft indicator variables
        uint nSoftIndicators = softIndicators.size();

        //get the Attribute's data file
        File *file = m_at->getContainingFile();
        if( file->isDataFile() ){

            //////////////////////////Completes the results with any no-data-values that may exist in the data samples/////////////
            DataFile *dataFile = dynamic_cast<DataFile*>(file);
            //load the data
            dataFile->loadData();
            // get the number of data samples
            uint nData = dataFile->getDataLineCount();
            //get the Attribute's GEO-EAS index
            uint atGEOEASIndex = m_at->getAttributeGEOEASgivenIndex();
            //fills the arrays of soft indicator vectors with any NDVs, to make sure the
            //sizes match the data count
            for( uint i = 0; i < nData; ++i){
                //get sample value
                double value = dataFile->data( i, atGEOEASIndex-1 );
                //if the value is a No-Data-Value
                if( dataFile->isNDV( value ) ){
                    //for each soft indicator variable
                    for( uint iSoftIndicator = 0; iSoftIndicator < nSoftIndicators; ++iSoftIndicator){
                        //insert a no-data-value in the soft indicator vector
                        softIndicators[iSoftIndicator].insert( softIndicators[iSoftIndicator].begin() + i,
                                                               dataFile->getNoDataValueAsDouble() );
                    }
                }
            }
            ////////////////////////////////////////////////////////////////

            //suggest names for the soft indicator fields
            QStringList labels;
            for( uint iSoftIndicator = 0; iSoftIndicator < nSoftIndicators; ++iSoftIndicator){
                if( selectedFile->getFileType() == "THRESHOLDCDF"){
                    ThresholdCDF *cdf = (ThresholdCDF*)selectedFile;
                    labels.append( "thr. = " + QString::number( cdf->get1stValue( iSoftIndicator ), 'g', 12 ) );
                }else if( selectedFile->getFileType() == "CATEGORYDEFINITION"){
                    CategoryDefinition *cd = (CategoryDefinition*)selectedFile;
                    labels.append( cd->getCategoryName( iSoftIndicator ) );
                }else if( selectedFile->getFileType() == "CATEGORYPDF"){
                    CategoryPDF *pdf = (CategoryPDF*)selectedFile;
                    CategoryDefinition *cd = pdf->getCategoryDefinition();
                    cd->loadQuintuplets();
                    labels.append( cd->getCategoryNameByCode( pdf->get1stValue( iSoftIndicator ) ) );
                }
            }


            //////////////////////////////creates a temporary data file with the soft indicators//////////////////////////
            //open a new file for output
            QString tmpFilePath = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");
            QFile outputFile( tmpFilePath );
            outputFile.open( QFile::WriteOnly | QFile::Text );
            QTextStream out(&outputFile);

            //output the GEO-EAS header
            out << "Soft indicators for " + dataFile->getPath() + '\n';
			if( dataFile->getFileType() == "POINTSET" ){
				out << nSoftIndicators+3 << '\n'; // +3 for the X,Y,Z coordinates of a pointset
				out << "X" << '\n';
				out << "Y" << '\n';
				out << "Z" << '\n';
			} else if( dataFile->getFileType() == "CARTESIANGRID" ){
				out << nSoftIndicators << '\n';
			}
			for( uint iSoftIndicator = 0; iSoftIndicator < nSoftIndicators; ++iSoftIndicator){
                out << m_at->getName() << " soft indicator for " << labels[ iSoftIndicator ] << '\n';
            }

			//Get the point set aspect of the data (if it is actually a point set)
			PointSet* ps = nullptr;
			if( dataFile->getFileType() == "POINTSET" )
				ps = dynamic_cast<PointSet*>( dataFile );

            //output the original data and their computed soft indicators
            for( uint i = 0; i < nData; ++i){
				//output the X, Y, Z fields if the data file is a pointset
				if( ps ){
					out << dataFile->data( i, ps->getXindex()-1 ) << '\t';
					out << dataFile->data( i, ps->getYindex()-1 ) << '\t';
					if( ps->is3D() )
						out << dataFile->data( i, ps->getZindex()-1 ) << '\t';
					else
						out << "0.0" << '\t';
				}
                // the residue is used to ensure a 1.0 sum for the soft indicators
                double residue = 1.0;
                //for each soft indicator variable
                for( uint iSoftIndicator = 0; iSoftIndicator < nSoftIndicators; ++iSoftIndicator){
                    QString softIndicatorString;
                    //get the soft indicator value
                    double softIndicatorValue = softIndicators[iSoftIndicator][i];
                    //if the soft indicator value is not NDV
                    if( ! dataFile->isNDV( softIndicatorValue ) ) {
                        double softIndicatorTruncated = std::floor( (softIndicatorValue/100.0) * 10000+0.5)/10000;
                        //get the string presentation of the soft indicator value with 6-digit precision
                        softIndicatorString = QString::number( softIndicatorTruncated, 'g', 4 );
                        //for categorical case, the delivered soft indicators must sum up 1.0 exactly
                        if( calcMode == SoftIndicatorCalculationMode::CATEGORICAL ){
                            //subtract the actual output value from the residue
                            residue -= softIndicatorTruncated;
                            //ensure a 1.0 total probability
                            if( iSoftIndicator == nSoftIndicators - 1){
                                softIndicatorTruncated += residue;
                                softIndicatorString = QString::number( softIndicatorTruncated, 'g', 4 );
                            }
                        }
                    } else {
                        softIndicatorString = dataFile->getNoDataValue();
                    }
                    //output the soft indicator or NDV value
                    out << softIndicatorString << '\t';
                }
                out << '\n';
            }
            //closes the output file
            outputFile.close();
            //////////////////////////////////////////////////////////////////////////////////////

            return tmpFilePath;
        }
    }
    return "FILE_NOT_FOUND.DAT";
}
