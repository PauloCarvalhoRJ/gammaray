#include "thinsectionanalysisdialog.h"
#include "ui_thinsectionanalysisdialog.h"

#include "domain/application.h"

#include <QFileDialog>
#include <QFileIconProvider>

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

    //for each pixel, find the maxima and minima for each color component
    //this is necessary to normalize the values so we can maximize information content
    //for feature extraction
    QColor rgb_tmp;
    uchar uFeatureVector[12];
    uchar uMaxima[12] = {  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0}; //the max values for each color component of both PP and PX images
    uchar uMinima[12] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}; //the min values for each color component of both PP and PX images
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
    }

    //for each pixel, normalize the values for each color component
    std::vector< std::array< float, 12 > > featureVectors( pixelCount );
    const float epsilon = 0.00001f;
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
                                 static_cast<float>(( uMaxima[ 0] - uMinima[ 0] )) + epsilon;
        featureVectors[i][ 1] =              ( uFeatureVector[ 1] - uMinima[ 1] ) /
                                 static_cast<float>(( uMaxima[ 1] - uMinima[ 1] )) + epsilon;
        featureVectors[i][ 2] =              ( uFeatureVector[ 2] - uMinima[ 2] ) /
                                 static_cast<float>(( uMaxima[ 2] - uMinima[ 2] )) + epsilon;
        featureVectors[i][ 3] =              ( uFeatureVector[ 3] - uMinima[ 3] ) /
                                 static_cast<float>(( uMaxima[ 3] - uMinima[ 3] )) + epsilon;
        featureVectors[i][ 4] =              ( uFeatureVector[ 4] - uMinima[ 4] ) /
                                 static_cast<float>(( uMaxima[ 4] - uMinima[ 4] )) + epsilon;
        featureVectors[i][ 5] =              ( uFeatureVector[ 5] - uMinima[ 5] ) /
                                 static_cast<float>(( uMaxima[ 5] - uMinima[ 5] )) + epsilon;
        featureVectors[i][ 6] =              ( uFeatureVector[ 6] - uMinima[ 6] ) /
                                 static_cast<float>(( uMaxima[ 6] - uMinima[ 6] )) + epsilon;
        featureVectors[i][ 7] =              ( uFeatureVector[ 7] - uMinima[ 7] ) /
                                 static_cast<float>(( uMaxima[ 7] - uMinima[ 7] )) + epsilon;
        featureVectors[i][ 8] =              ( uFeatureVector[ 8] - uMinima[ 8] ) /
                                 static_cast<float>(( uMaxima[ 8] - uMinima[ 8] )) + epsilon;
        featureVectors[i][ 9] =              ( uFeatureVector[ 9] - uMinima[ 9] ) /
                                 static_cast<float>(( uMaxima[ 9] - uMinima[ 9] )) + epsilon;
        featureVectors[i][10] =              ( uFeatureVector[10] - uMinima[10] ) /
                                 static_cast<float>(( uMaxima[10] - uMinima[10] )) + epsilon;
        featureVectors[i][11] =              ( uFeatureVector[11] - uMinima[11] ) /
                                 static_cast<float>(( uMaxima[11] - uMinima[11] )) + epsilon;

    }

    //lambda function to compute the similarity of two 12-element vectors according to
    // Izadi et al (2020): "Altered mineral segmentation using incremental-dynamic clustering", eq 2.
    // the result varies between 0.0 (least similar) and 1.0 (most similar).
//    std::function<float (const std::array< float, 12 >&, const std::array< float, 12 >&)> similarity =
//            [](const std::array< float, 12 >& v1, const std::array< float, 12 >& v2)
//    {
//        return 1 / ( 1 + std::sqrt(
//                         (v1[ 0]-v2[ 0])*(v1[ 0]-v2[ 0])+
//                         (v1[ 1]-v2[ 1])*(v1[ 1]-v2[ 1])+
//                         (v1[ 2]-v2[ 2])*(v1[ 2]-v2[ 2])+
//                         (v1[ 3]-v2[ 3])*(v1[ 3]-v2[ 3])+
//                         (v1[ 4]-v2[ 4])*(v1[ 4]-v2[ 4])+
//                         (v1[ 5]-v2[ 5])*(v1[ 5]-v2[ 5])+
//                         (v1[ 6]-v2[ 6])*(v1[ 6]-v2[ 6])+
//                         (v1[ 7]-v2[ 7])*(v1[ 7]-v2[ 7])+
//                         (v1[ 8]-v2[ 8])*(v1[ 8]-v2[ 8])+
//                         (v1[ 9]-v2[ 9])*(v1[ 9]-v2[ 9])+
//                         (v1[10]-v2[10])*(v1[10]-v2[10])+
//                         (v1[11]-v2[11])*(v1[11]-v2[11])
//                         ) );
//    };

    // Small local class to organize cluster data and functionality
    struct Cluster{
        //the constructor must accept the reference to the vector of the feature vectors
        //of all pixels
        Cluster( const std::vector< std::array< float, 12 > >& pRefFeatureVectors ) :
            refFeatureVectors( pRefFeatureVectors ){}
        //computes the mean feature fector of this cluster and stores it in center member
        void updateCenter(){
            float sum[12] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
            for( uint i : pixelIndexes )
                for( uchar j = 0; j < 12; ++j )
                    sum[j] += refFeatureVectors[i][j];
            for( uchar j = 0; j < 12; ++j )
                center[j] = sum[j] / pixelIndexes.size();
        }
        //adds a pixel index to this cluster and updates its center
        void addPixelIndex( uint pixelIndex ){
            pixelIndexes.push_back( pixelIndex );
            updateCenter();
        }
        float similarity(const std::array< float, 12 >& v1, const std::array< float, 12 >& v2) const
        {
            return 1 / ( 1 + std::sqrt(
                             (v1[ 0]-v2[ 0])*(v1[ 0]-v2[ 0])+
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
                             (v1[11]-v2[11])*(v1[11]-v2[11])
                             ) );
        }
        //returns whether the given pixel is similar to this cluster
        bool isSimilar( uint pixelIndex, float sigma ) const{
            return similarity( center, refFeatureVectors[pixelIndex] ) >= sigma;
        }
        //adds the pixel indexes of another cluster to this cluster's pixelIndexes list
        //and updates the center
        void merge( const Cluster& otherCluster ){
            pixelIndexes.insert( pixelIndexes.end(),
                                 otherCluster.pixelIndexes.begin(),
                                 otherCluster.pixelIndexes.end() );
            updateCenter();
        }
        //reference to the vector of feature vectors of all pixels
        const std::vector< std::array< float, 12 > >& refFeatureVectors;
        //the cluster only stores pixel indexes, not the feature vectors to save memory
        std::vector<uint> pixelIndexes;
        //the average feature vector of all pixels indexed by this cluster.
        std::array< float, 12 > center;
    };

    //the collection of clusters
    std::vector< Cluster > clusters;

    //adds the first cluster with the first pixel
    Cluster firstCluster( featureVectors );
    firstCluster.addPixelIndex( 0 );

    //for each pixel (2nd and on), process clustering algorithm
    for( uint i = 1; i < pixelCount; ++i ){

        //keep track of the clusters that will receive the pixel
        //clusters that reference the same pixel must be merged.
        std::vector<uint> clusterIndexesThatReceivedThePixel;

        //for each cluster
        for( int iCluster = 0; iCluster < clusters.size(); ++iCluster ){

            //get the reference to a cluster
            Cluster& cluster = clusters[iCluster];

            //if the pixel fits in the cluster
            if( cluster.isSimilar( i, ui->dblSpinSimilarityThreshold->value() )){
                //assigns the pixel to the cluster
                cluster.addPixelIndex( i );
                //save the index of the cluster that will receive the pixel
                clusterIndexesThatReceivedThePixel.push_back( iCluster );
            }
        }

        //if the pixel didn't fit to any cluster...
        if( clusterIndexesThatReceivedThePixel.empty() ){
            //...creates a new cluster and puts the pixel in it.
            Cluster cluster( featureVectors );
            firstCluster.addPixelIndex( i );
            clusters.push_back( cluster );
        //if the pixel fitted to more than one cluster, such clusters
        //must be merged.
        } else if( clusterIndexesThatReceivedThePixel.size() > 1 ){
            //get the first cluster that received the pixel
            Cluster& firstCluster = clusters[ clusterIndexesThatReceivedThePixel[0] ];
            //remove its index from the list
            clusterIndexesThatReceivedThePixel.erase( clusterIndexesThatReceivedThePixel.begin() );
            //merge the other clusters that received the pixel into it
            for( int clusterIndexThatReceivedThePixel : clusterIndexesThatReceivedThePixel ){
                firstCluster.merge( clusters[ clusterIndexThatReceivedThePixel ] );
            }
        }

        //remove the clusters that have been merged into another.
        for( int clusterIndexThatReceivedThePixel : clusterIndexesThatReceivedThePixel ){
            //clusters.erase( clusters.begin() + clusterIndexThatReceivedThePixel );
        }

    }

    Application::instance()->logInfo("Thin section analysis completed.");
}
