#include "thinsectionanalysisdialog.h"
#include "ui_thinsectionanalysisdialog.h"

#include "domain/application.h"

#include <QFileDialog>
#include <QFileIconProvider>
#include <QProgressDialog>

#include "util.h"

ThinSectionAnalysisDialog::ThinSectionAnalysisDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ThinSectionAnalysisDialog),
    m_directoryPath(""),
    m_qFileIconProvider( new QFileIconProvider() ) //not needed to be a member variable, but construction of QFileIconProvider is heavy
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Thin Section Analysis" );
}

ThinSectionAnalysisDialog::~ThinSectionAnalysisDialog()
{
    delete ui;
    delete m_qFileIconProvider;
}

void ThinSectionAnalysisDialog::onOpenDir()
{
    m_directoryPath = QFileDialog::getExistingDirectory( this, "Directory with images", m_directoryPath );
    onDirectoryChanged();
}

void ThinSectionAnalysisDialog::onDirectoryChanged()
{
    //Update UI to show current directory.
    ui->lblDirectory->setText( m_directoryPath );

    //Build a list of filters for the common image file extensions.
    QStringList exts;
    for( QString ext : Util::getListOfImageFileExtensions() )
        exts << ( "*." + ext );

    //Get a list of image files in current directory (if any).
    QDir dir( m_directoryPath );
    dir.setNameFilters( exts );
    QStringList fileList = dir.entryList();

    //Update the list widget with the image files found in the directory.
    ui->lstFiles->clear();
    for( QString fileName : fileList ){
        QFileInfo fileInfo( m_directoryPath + "/" + fileName );
        QIcon fileIcon = m_qFileIconProvider->icon( fileInfo );
        ui->lstFiles->addItem( new QListWidgetItem( fileIcon, fileName ) );
    }

    //Clear the lists of the images selected as plane and cross polarizations
    ui->lstPlanePolarizationImages->clear();
    ui->lstCrossPolarizationImages->clear();
}

void ThinSectionAnalysisDialog::onPlanePolarizationImageSelected()
{
    ui->lstCrossPolarizationImages->setCurrentRow( ui->lstPlanePolarizationImages->currentRow() );
    onUpdateImageDisplays();
}

void ThinSectionAnalysisDialog::onCrossPolarizationImageSelected()
{
    ui->lstPlanePolarizationImages->setCurrentRow( ui->lstCrossPolarizationImages->currentRow() );
    onUpdateImageDisplays();
}

void ThinSectionAnalysisDialog::onUpdateImageDisplays()
{
    QImage imagePP( ui->lblDirectory->text() + "/" + ui->lstPlanePolarizationImages->currentItem()->text() );
    QPixmap pixPP = QPixmap::fromImage(imagePP);
    ui->lblImgPlanePolarization->setPixmap( pixPP.scaled( ui->lblImgPlanePolarization->size(),
                                                          Qt::KeepAspectRatio,
                                                          Qt::SmoothTransformation) );

    QImage imagePX( ui->lblDirectory->text() + "/" + ui->lstCrossPolarizationImages->currentItem()->text() );
    QPixmap pixPX = QPixmap::fromImage(imagePX);
    ui->lblImgCrossPolarization->setPixmap( pixPX.scaled( ui->lblImgCrossPolarization->size(),
                                                          Qt::KeepAspectRatio,
                                                          Qt::SmoothTransformation) );
}

void ThinSectionAnalysisDialog::onRun()
{
    //loading the images as strings of 4-byte tuples: Alpha, Red, Green and Blue as values between 0 and 255.
    QImage imagePPRGB( ui->lblDirectory->text() + "/" + ui->lstPlanePolarizationImages->currentItem()->text() );
    imagePPRGB = imagePPRGB.convertToFormat( QImage::Format_ARGB32 );
    QImage imagePXRGB( ui->lblDirectory->text() + "/" + ui->lstCrossPolarizationImages->currentItem()->text() );
    imagePXRGB = imagePXRGB.convertToFormat( QImage::Format_ARGB32 );

    //assuming both images are exactly the same size in pixels.
    int nCols = imagePPRGB.size().width();
    int nRows = imagePPRGB.size().height();
    int pixelCount = nCols * nRows;
    const int bytesPerPixel = 4;

    //accessing the images data directly (read-only)
    const uchar* imagePPRGBdata = imagePPRGB.constBits();
    const uchar* imagePXRGBdata = imagePXRGB.constBits();

    //create and show a progress dialog
    QProgressDialog progressDialog;
    progressDialog.show();
    uint progressRefreshRate = pixelCount / 100; //limit to 100 GUI refreshes to minize impact on performance
    if( progressRefreshRate < 1 )
        progressRefreshRate = 1;

    ///========================================================================================================
    //Find the maxima and minima for each color component
    //this is necessary to normalize the values so we can maximize information content
    //for feature extraction.
    QColor rgb_tmp;
    uchar uFeatureVector[12];
    uchar uMaxima[12] = {  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0}; //the max values for each color component of both PP and PX images
    uchar uMinima[12] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}; //the min values for each color component of both PP and PX images
    progressDialog.setLabelText("Finding maxima and minima of color components...");
    progressDialog.setMinimum(0);
    progressDialog.setValue(0);
    progressDialog.setMaximum( pixelCount );
    for( int i = 0; i < pixelCount; ++i ){

        //get the RGB values between 0 and 255 from the images' data array.
        uFeatureVector[ 0] = imagePPRGBdata[ bytesPerPixel * i + 1 ]; //alpha (index==0) is not used
        uFeatureVector[ 1] = imagePPRGBdata[ bytesPerPixel * i + 2 ];
        uFeatureVector[ 2] = imagePPRGBdata[ bytesPerPixel * i + 3 ];
        uFeatureVector[ 3] = imagePXRGBdata[ bytesPerPixel * i + 1 ];
        uFeatureVector[ 4] = imagePXRGBdata[ bytesPerPixel * i + 2 ];
        uFeatureVector[ 5] = imagePXRGBdata[ bytesPerPixel * i + 3 ];

        //get the HSI valures between 0 and 255.
        rgb_tmp.setRgb( uFeatureVector[ 0],
                        uFeatureVector[ 1],
                        uFeatureVector[ 2] );
        uFeatureVector[ 6] = rgb_tmp.hue();
        uFeatureVector[ 7] = rgb_tmp.saturation();
        uFeatureVector[ 8] = ( ui->cmbHSImode->currentIndex()==0 ? rgb_tmp.value() : rgb_tmp.lightness() );
        rgb_tmp.setRgb( uFeatureVector[ 3],
                        uFeatureVector[ 4],
                        uFeatureVector[ 5] );
        uFeatureVector[ 9] = rgb_tmp.hue();
        uFeatureVector[10] = rgb_tmp.saturation();
        uFeatureVector[11] = ( ui->cmbHSImode->currentIndex()==0 ? rgb_tmp.value() : rgb_tmp.lightness() );

        //find the maxima for each color component
        uMaxima[ 0] = std::max( uMaxima[ 0], uFeatureVector[ 0] );
        uMaxima[ 1] = std::max( uMaxima[ 1], uFeatureVector[ 1] );
        uMaxima[ 2] = std::max( uMaxima[ 2], uFeatureVector[ 2] );
        uMaxima[ 3] = std::max( uMaxima[ 3], uFeatureVector[ 3] );
        uMaxima[ 4] = std::max( uMaxima[ 4], uFeatureVector[ 4] );
        uMaxima[ 5] = std::max( uMaxima[ 5], uFeatureVector[ 5] );
        uMaxima[ 6] = std::max( uMaxima[ 6], uFeatureVector[ 6] );
        uMaxima[ 7] = std::max( uMaxima[ 7], uFeatureVector[ 7] );
        uMaxima[ 8] = std::max( uMaxima[ 8], uFeatureVector[ 8] );
        uMaxima[ 9] = std::max( uMaxima[ 9], uFeatureVector[ 9] );
        uMaxima[10] = std::max( uMaxima[10], uFeatureVector[10] );
        uMaxima[11] = std::max( uMaxima[11], uFeatureVector[11] );

        //find the minima for each color component
        uMinima[ 0] = std::min( uMinima[ 0], uFeatureVector[ 0] );
        uMinima[ 1] = std::min( uMinima[ 1], uFeatureVector[ 1] );
        uMinima[ 2] = std::min( uMinima[ 2], uFeatureVector[ 2] );
        uMinima[ 3] = std::min( uMinima[ 3], uFeatureVector[ 3] );
        uMinima[ 4] = std::min( uMinima[ 4], uFeatureVector[ 4] );
        uMinima[ 5] = std::min( uMinima[ 5], uFeatureVector[ 5] );
        uMinima[ 6] = std::min( uMinima[ 6], uFeatureVector[ 6] );
        uMinima[ 7] = std::min( uMinima[ 7], uFeatureVector[ 7] );
        uMinima[ 8] = std::min( uMinima[ 8], uFeatureVector[ 8] );
        uMinima[ 9] = std::min( uMinima[ 9], uFeatureVector[ 9] );
        uMinima[10] = std::min( uMinima[10], uFeatureVector[10] );
        uMinima[11] = std::min( uMinima[11], uFeatureVector[11] );

        //update the progress bar
        if( ! ( i % progressRefreshRate ) ){
            progressDialog.setValue(i);
            QApplication::processEvents();
        }

    }
    ///========================================================================================================

    ///========================================================================================================
    //Normalize the values for each color component of each pixel.
    std::vector< std::array< float, 12 > > featureVectors( pixelCount );
    const float epsilon = 0.00001f; //this value serves to avoid potential divisions by zero during normalization
    progressDialog.setLabelText("Normalizing feature vectors...");
    progressDialog.setMinimum(0);
    progressDialog.setValue(0);
    for( uint i = 0; i < pixelCount; ++i ){

        //get the RGB values between 0 and 255 from the images' data array.
        uFeatureVector[ 0] = imagePPRGBdata[ bytesPerPixel * i + 1 ]; //alpha (index==0) is not used
        uFeatureVector[ 1] = imagePPRGBdata[ bytesPerPixel * i + 2 ];
        uFeatureVector[ 2] = imagePPRGBdata[ bytesPerPixel * i + 3 ];
        uFeatureVector[ 3] = imagePXRGBdata[ bytesPerPixel * i + 1 ];
        uFeatureVector[ 4] = imagePXRGBdata[ bytesPerPixel * i + 2 ];
        uFeatureVector[ 5] = imagePXRGBdata[ bytesPerPixel * i + 3 ];

        //get the HSI valures between 0 and 255.
        rgb_tmp.setRgb( uFeatureVector[ 0],
                        uFeatureVector[ 1],
                        uFeatureVector[ 2] );
        uFeatureVector[ 6] = rgb_tmp.hue();
        uFeatureVector[ 7] = rgb_tmp.saturation();
        uFeatureVector[ 8] = ( ui->cmbHSImode->currentIndex()==0 ? rgb_tmp.value() : rgb_tmp.lightness() );
        rgb_tmp.setRgb( uFeatureVector[ 3],
                        uFeatureVector[ 4],
                        uFeatureVector[ 5] );
        uFeatureVector[ 9] = rgb_tmp.hue();
        uFeatureVector[10] = rgb_tmp.saturation();
        uFeatureVector[11] = ( ui->cmbHSImode->currentIndex()==0 ? rgb_tmp.value() : rgb_tmp.lightness() );

        //normalize the values for each color component
        featureVectors[i][ 0] =              ( uFeatureVector[ 0] - uMinima[ 0] ) /
                                 static_cast<float>(( uMaxima[ 0] - uMinima[ 0] + epsilon ));
        featureVectors[i][ 1] =              ( uFeatureVector[ 1] - uMinima[ 1] ) /
                                 static_cast<float>(( uMaxima[ 1] - uMinima[ 1] + epsilon ));
        featureVectors[i][ 2] =              ( uFeatureVector[ 2] - uMinima[ 2] ) /
                                 static_cast<float>(( uMaxima[ 2] - uMinima[ 2] + epsilon ));
        featureVectors[i][ 3] =              ( uFeatureVector[ 3] - uMinima[ 3] ) /
                                 static_cast<float>(( uMaxima[ 3] - uMinima[ 3] + epsilon ));
        featureVectors[i][ 4] =              ( uFeatureVector[ 4] - uMinima[ 4] ) /
                                 static_cast<float>(( uMaxima[ 4] - uMinima[ 4] + epsilon ));
        featureVectors[i][ 5] =              ( uFeatureVector[ 5] - uMinima[ 5] ) /
                                 static_cast<float>(( uMaxima[ 5] - uMinima[ 5] + epsilon ));
        featureVectors[i][ 6] =              ( uFeatureVector[ 6] - uMinima[ 6] ) /
                                 static_cast<float>(( uMaxima[ 6] - uMinima[ 6] + epsilon ));
        featureVectors[i][ 7] =              ( uFeatureVector[ 7] - uMinima[ 7] ) /
                                 static_cast<float>(( uMaxima[ 7] - uMinima[ 7] + epsilon ));
        featureVectors[i][ 8] =              ( uFeatureVector[ 8] - uMinima[ 8] ) /
                                 static_cast<float>(( uMaxima[ 8] - uMinima[ 8] + epsilon ));
        featureVectors[i][ 9] =              ( uFeatureVector[ 9] - uMinima[ 9] ) /
                                 static_cast<float>(( uMaxima[ 9] - uMinima[ 9] + epsilon ));
        featureVectors[i][10] =              ( uFeatureVector[10] - uMinima[10] ) /
                                 static_cast<float>(( uMaxima[10] - uMinima[10] + epsilon ));
        featureVectors[i][11] =              ( uFeatureVector[11] - uMinima[11] ) /
                                 static_cast<float>(( uMaxima[11] - uMinima[11] + epsilon ));

        //update the progress bar
        if( ! ( i % progressRefreshRate ) ){
            progressDialog.setValue(i);
            QApplication::processEvents();
        }

    }
    ///========================================================================================================

    // Before proceeding to the clustering algorithm, let's define a small local class
    // to encapsulate cluster data and functionality into a single unit.
    struct Cluster{
        //the constructor must accept the reference to the vector of the feature vectors
        //of all pixels to reuse it across all clusters
        Cluster( const std::vector< std::array< float, 12 > >& pRefFeatureVectors ) :
            refFeatureVectors( pRefFeatureVectors ),
            center( {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f} ),
            toDelete( false )
            {}
        //adds a pixel index to this cluster and updates its center
        void addPixelIndex( uint pixelIndex ){
            pixelIndexes.push_back( pixelIndex );
            //mean updating: https://math.stackexchange.com/questions/106700/incremental-averageing
            for( uchar j = 0; j < 12; ++j )
                center[j] += ( refFeatureVectors[pixelIndex][j] - center[j] ) / pixelIndexes.size();
        }
        // Computes the similarity of two 12-element vectors according to
        // Izadi et al (2020): "Altered mineral segmentation using incremental-dynamic clustering", eq 2.
        // the result varies between 0.0 (least similar) and 1.0 (most similar).
        float similarity(const std::array< float, 12 >& v1, const std::array< float, 12 >& v2) const
        {
            return 1 / ( 1 + std::sqrt( (v1[ 0]-v2[ 0])*(v1[ 0]-v2[ 0])+
                                        (v1[ 1]-v2[ 1])*(v1[ 1]-v2[ 1])+
                                        (v1[ 2]-v2[ 2])*(v1[ 2]-v2[ 2])+
                                        (v1[ 3]-v2[ 3])*(v1[ 3]-v2[ 3])+
                                        (v1[ 4]-v2[ 4])*(v1[ 4]-v2[ 4])+
                                        (v1[ 5]-v2[ 5])*(v1[ 5]-v2[ 5])+
                                        (v1[ 6]-v2[ 6])*(v1[ 6]-v2[ 6])+
                                        (v1[ 7]-v2[ 7])*(v1[ 7]-v2[ 7])+
                                        (v1[ 8]-v2[ 8])*(v1[ 8]-v2[ 8])+
                                        (v1[ 9]-v2[ 9])*(v1[ 9]-v2[ 9])+
                                        (v1[10]-v2[10])*(v1[10]-v2[10])+
                                        (v1[11]-v2[11])*(v1[11]-v2[11]) ) );
        }
        //returns whether the given pixel is similar to this cluster
        bool isSimilar( uint pixelIndex, float sigma ) const{
            return similarity( center, refFeatureVectors[pixelIndex] ) >= sigma;
        }
        //adds the pixel indexes of another cluster to this cluster's pixelIndexes list
        //and updates the center
        void merge( const Cluster& otherCluster ){
            //update the center (average of two averages)
            float mySum[12]    = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
            for( uchar j = 0; j < 12; ++j ) {
                //get the sum of this cluster by multiplying the mean by the current count
                mySum[j] = center[j] * pixelIndexes.size();
                //adds the sum of the other cluster, which is calculated the same way
                mySum[j] += otherCluster.center[j] * otherCluster.pixelIndexes.size();
                //the new average is then the grand sum divided by the combined number of elements
                center[j] = mySum[j] / ( pixelIndexes.size() + otherCluster.pixelIndexes.size() );
            }
            //adds the pixel indexes of the other cluster
            pixelIndexes.insert( pixelIndexes.end(),
                                 otherCluster.pixelIndexes.begin(),
                                 otherCluster.pixelIndexes.end() );
        }
        //reference to the vector of feature vectors of all pixels
        const std::vector< std::array< float, 12 > >& refFeatureVectors;
        //the cluster only stores pixel indexes, not the feature vectors to save memory
        //theses indexes refer to feature vectors in refFeatureVectors
        std::vector<uint> pixelIndexes;
        //the average feature vector of all pixels indexed by this cluster.
        std::array< float, 12 > center;
        //flag used to mark a cluster for deletion
        bool toDelete;
    };

    //let's also define a smart pointer to Cluster objects
    typedef std::shared_ptr< Cluster > ClusterPtr;

    //the collection of clusters
    std::vector< ClusterPtr > clusters;
    clusters.reserve( 5000 ); //5k clusters is a fairly large number

    //adds the first cluster with the first pixel
    ClusterPtr firstCluster = std::make_shared<Cluster>( featureVectors );
    firstCluster->addPixelIndex( 0 );
    clusters.push_back( firstCluster );

    //This list is to keep track of the clusters that will receive a same pixel.
    //Clusters that reference a same pixel must be merged.
    std::vector<uint> clusterIndexesThatReceivedThePixel;
    //A typical high-res thin section image is about 10k x 10k = 100M pixels
    //Supposing 100 classes (clusters), one can expect 1M pixels per cluster on average
    //but often there is a class that is 20% to 50% of all pixels, hence 50M pixels
    //Reserving 50M pixels is to avoid costly re-allocations due to vector growth
    clusterIndexesThatReceivedThePixel.reserve( 50000000 );

    ///========================================================================================================
    //for each pixel (2nd and on), process clustering algorithm
    progressDialog.setLabelText("Processing clustering...");
    progressDialog.setMinimum(0);
    progressDialog.setValue(0);
    for( uint iPixelIndex = 1; iPixelIndex < pixelCount; ++iPixelIndex ){

        clusterIndexesThatReceivedThePixel.clear();

        //for each cluster...
        for( int iCluster = 0; iCluster < clusters.size(); ++iCluster ){

            //...get the pointer to it.
            ClusterPtr cluster = clusters[iCluster];

            //if the pixel fits in the cluster...
            if( cluster->isSimilar( iPixelIndex, ui->dblSpinSimilarityThreshold->value() )){
                //...assigns the pixel to the cluster
                cluster->addPixelIndex( iPixelIndex );
                //save the index of the cluster that received the pixel
                clusterIndexesThatReceivedThePixel.push_back( iCluster );
            }
        }

        //if the pixel didn't fit to any cluster...
        if( clusterIndexesThatReceivedThePixel.empty() ){

            //...creates a new cluster and puts the pixel in it.
            ClusterPtr cluster = std::make_shared<Cluster>( featureVectors );
            cluster->addPixelIndex( iPixelIndex );
            clusters.push_back( cluster );

        //if the pixel fitted to more than one cluster, such clusters
        //must be merged.
        } else if( clusterIndexesThatReceivedThePixel.size() > 1 ){

            //get the first cluster that received the pixel
            ClusterPtr firstCluster = clusters[ clusterIndexesThatReceivedThePixel[0] ];

            //remove its index from the list of clusters that received the pixel
            clusterIndexesThatReceivedThePixel.erase( clusterIndexesThatReceivedThePixel.begin() );

            //for each of the other clusters that received the pixel...
            for( int clusterIndexThatReceivedThePixel : clusterIndexesThatReceivedThePixel ) {
                //...merge it into the first cluster that received the pixel
                firstCluster->merge( *clusters[ clusterIndexThatReceivedThePixel ] );
                //...mark it for deletion.
                clusters[ clusterIndexThatReceivedThePixel ]->toDelete = true;
            }

            //purge clusters marked for deletion
            clusters.erase(
                std::remove_if(
                    clusters.begin(),
                    clusters.end(),
                    [](ClusterPtr& cluster) -> bool {
                        return cluster->toDelete;
                    }
                ),
                clusters.end()
            );

        } // if the pixel was assigned to more than cluster or to none at all

        //update the progress bar
        if( ! ( iPixelIndex % progressRefreshRate ) ){
            progressDialog.setValue(iPixelIndex);
            QApplication::processEvents();
        }

    } //cluster algorithm loop
    ///========================================================================================================


    //generate an image by painting the pixels with random contrasting colors
    QImage imageOutput( imagePPRGB.size(), QImage::Format_ARGB32 );
    uchar r, g, b;
    uint clusterCount = 0;
    progressDialog.setLabelText("Genereating output image...");
    progressDialog.setMinimum(0);
    progressDialog.setValue(0);
    for( const ClusterPtr& cluster : clusters  ){
        //assign a constrasting color to the clusters
        //GSLib's colors form a good sequence of constrasting colors
        QColor clusterColor = Util::getGSLibColor( (clusterCount++) % Util::getMaxGSLibColorCode() + 1 );
        r = clusterColor.red();
        g = clusterColor.green();
        b = clusterColor.blue();
        for( uint iPixel : cluster->pixelIndexes ){
//            imageOutputData[ bytesPerPixel * iPixel + 0 ] = 0;
//            imageOutputData[ bytesPerPixel * iPixel + 1 ] = 255;
//            imageOutputData[ bytesPerPixel * iPixel + 2 ] = 0;
//            imageOutputData[ bytesPerPixel * iPixel + 3 ] = 0;
            uint line = iPixel / nCols;
            uint column = iPixel % nCols;
            imageOutput.setPixel( column, line, qRgb( r, g, b ) );
        }
        //update the progress bar
        progressDialog.setValue(clusterCount);
        QApplication::processEvents();
    }

    imageOutput.save( ui->lblDirectory->text() + "/clusters.png" );

    Application::instance()->logInfo("Thin section analysis completed.");
}
