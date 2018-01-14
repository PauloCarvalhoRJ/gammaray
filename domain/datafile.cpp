#include "datafile.h"
#include "../exceptions/invalidgslibdatafileexception.h"
#include "../util.h"
#include "algorithms/ialgorithmdatasource.h"
#include "application.h"
#include "attribute.h"
#include "auxiliary/dataloader.h"
#include "cartesiangrid.h"
#include "domain/categorydefinition.h"
#include "domain/univariatecategoryclassification.h"
#include "normalvariable.h"
#include "objectgroup.h"
#include "project.h"
#include "weight.h"
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QTextStream>
#include <QtConcurrent/QtConcurrent>
#include <cmath>
#include <iomanip> // std::setprecision
#include <limits>
#include <sstream> // std::stringstream
#include "auxiliary/dataloader.h"
#include "auxiliary/variableremover.h"
#include "auxiliary/datasaver.h"
#include "algorithms/ialgorithmdatasource.h"

/****************************** THE DATASOURCE INTERFACE TO THE ALGORITHM CLASSES
 * ****************************/
/***************************** WARNING: This class contains sensitive operations.
 * ***************************/
class AlgorithmDataSource : public IAlgorithmDataSource
{
public:
    AlgorithmDataSource(DataFile &dataFile)
        : IAlgorithmDataSource(), m_dataFile(dataFile), m_dataCache(nullptr),
          m_rowCount(0), m_colCount(0)
    {
    }
    ~AlgorithmDataSource()
    {
        if (m_dataCache)
            delete[] m_dataCache;
    }
    // IAlgorithmDataSource interface
public:
    virtual long getRowCount() const { return m_rowCount; }
    virtual int getColumnCount() const { return m_colCount; }
    virtual void clear() { throw InvalidMethodException(); }
    virtual void reserve(long /*rowCount*/, int /*columnCount*/)
    {
        throw InvalidMethodException();
    }
    virtual void setDataValue(long /*rowIndex*/, int /*columnIndex*/, DataValue /*value*/)
    {
        throw InvalidMethodException();
    }
    virtual DataValue getDataValue(long rowIndex, int columnIndex) const
    {
        if (m_isCategoricalCache[columnIndex]) // m_isCategoricalCache.size() is the
                                               // number of columns
            return std::move(DataValue(
                (int)m_dataCache[rowIndex * m_isCategoricalCache.size()
                                 + columnIndex])); // init DataValue as categorical
        else
            return std::move(DataValue(
                /*double*/ m_dataCache[rowIndex * m_isCategoricalCache.size()
                                       + columnIndex])); // init DataValue as continuous
    }
    void init()
    {
        if (!m_isCategoricalCache.empty())
            return;
        // Build the cache of isCategorical flags to avoid calling costly methods in
        // getDataValue().
        // DataFile::isCategorical() and DataFile::getAtrributeFromGEOEASIndex() are very
        // costly
        m_dataFile.loadData();
        uint dataColumnCount = m_dataFile.getDataColumnCount();
        long dataRowCount = m_dataFile.getDataLineCount();
        for (uint iColumn = 0; iColumn < dataColumnCount; ++iColumn)
            m_isCategoricalCache.push_back(m_dataFile.isCategorical(
                m_dataFile.getAttributeFromGEOEASIndex(iColumn + 1)));
        // init the data cache to avoid calls to DataFile::data(), as it contains a
        // std::vector of std::vector, which causes
        // cache misses.  The data cache is a continuous array of doubles to optimize
        // cache hits.
        m_dataCache = new double[dataRowCount * dataColumnCount];
        for (long iRow = 0; iRow < dataRowCount; ++iRow)
            for (uint iColumn = 0; iColumn < dataColumnCount; ++iColumn)
                m_dataCache[iRow * dataColumnCount + iColumn]
                    = m_dataFile.data(iRow, iColumn);
        // store the data source sizes as the DataFile methods generate too many messages
        // (performance bottleneck)
        // and/or use the filesystem often.
        m_rowCount = dataRowCount;
        m_colCount = dataColumnCount;
    }

protected:
    DataFile &m_dataFile;
    std::vector<bool> m_isCategoricalCache;
    double *m_dataCache;
    long m_rowCount;
    int m_colCount;
};
/**********************************************************************************************************************************/

DataFile::DataFile(QString path)
    : File(path), _lastModifiedDateTimeLastLoad(), _dataPageFirstLine(0),
      _dataPageLastLine(std::numeric_limits<long>::max())
{
    _algorithmDataSourceInterface.reset(new AlgorithmDataSource(*this));
}

void DataFile::loadData()
{
    QFile file(this->_path);
    file.open(QFile::ReadOnly | QFile::Text);
    uint data_line_count = 0;
    QFileInfo info(_path);

    // if loaded data is not empty and was loaded before
    if (!_data.empty() && !_lastModifiedDateTimeLastLoad.isNull()) {
        QDateTime currentLastModified = info.lastModified();
        // if modified datetime didn't change since last call to loadData
        if (currentLastModified <= _lastModifiedDateTimeLastLoad) {
            Application::instance()->logInfo(
                QString("File ")
                    .append(this->_path)
                    .append(" already loaded and up to date.  Did nothing."));
            return; // does nothing
        }
    }

    // record the current datetime of file change
    _lastModifiedDateTimeLastLoad = info.lastModified();

    Application::instance()->logInfo(
        QString("Loading data from ").append(this->_path).append("..."));

    // make sure _data is empty
    _data.clear();

    // data load takes place in another thread, so we can show and update a progress bar
    //////////////////////////////////
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Loading and parsing " + _path + "...");
    progressDialog.setMinimum(0);
    progressDialog.setValue(0);
    progressDialog.setMaximum(getFileSize() / 100); // see DataLoader::doLoad(). Dividing
                                                    // by 100 allows a max value of ~400GB
                                                    // when converting from long to int
    QThread *thread = new QThread(); // does it need to set parent (a QObject)?
    DataLoader *dl = new DataLoader(file, _data, data_line_count, _dataPageFirstLine,
                                    _dataPageLastLine); // Do not set a parent. The object
                                                        // cannot be moved if it has a
                                                        // parent.
    dl->moveToThread(thread);
    dl->connect(thread, SIGNAL(finished()), dl, SLOT(deleteLater()));
    dl->connect(thread, SIGNAL(started()), dl, SLOT(doLoad()));
    dl->connect(dl, SIGNAL(progress(int)), &progressDialog, SLOT(setValue(int)));
    thread->start();
    /////////////////////////////////

    // wait for the data load to finish
    // not very beautiful, but simple and effective
    while (!dl->isFinished()) {
        thread->wait(200); // reduces cpu usage, refreshes at each 500 milliseconds
        QCoreApplication::processEvents(); // let Qt repaint widgets
    }

    file.close();

    // cartesian grids must have a given number of read lines
    if (this->getFileType() == "CARTESIANGRID") {
        CartesianGrid *cg = (CartesianGrid *)this;
        uint expected_total_lines
            = cg->getNX() * cg->getNY() * cg->getNZ() * cg->getNReal();
        if (data_line_count != expected_total_lines) {
            Application::instance()->logWarn(
                QString("DataFile::loadData(): number of parsed data lines ("
                        + QString::number(data_line_count)
                        + +") differs from the expected lines computed from the "
                           "Cartesian grid parameters ("
                        + QString::number(expected_total_lines)
                        + ").  Truncated files may cause crashes and result in incorrect "
                          "data analysis."),
                true);
        }
    }

    Application::instance()->logInfo("Finished loading data.");
}

double DataFile::data(uint line, uint column)
{
    switch (_data.size()) { // if _data is empty
    case 0:
        loadData(); // loads the data from disk.
    }
    return (this->_data.at(line)).at(column);
}

// TODO: consider adding a flag to disable NDV checking (applicable to coordinates)
double DataFile::max(uint column)
{
    if (_data.size() == 0)
        Application::instance()->logError(
            "DataFile::max(): Data not loaded. Unspecified value was returned.");
    double ndv = this->getNoDataValue().toDouble();
    bool has_ndv = this->hasNoDataValue();
    double result = -std::numeric_limits<double>::max();
    for (uint i = 0; i < _data.size(); ++i) {
        double value = data(i, column);
        if (value > result && (!has_ndv || !Util::almostEqual2sComplement(ndv, value, 1)))
            result = value;
    }
    return result;
}

double DataFile::maxAbs(uint column)
{
    if (_data.size() == 0)
        Application::instance()->logError(
            "DataFile::maxAbs(): Data not loaded. Unspecified value was returned.");
    double ndv = this->getNoDataValue().toDouble();
    bool has_ndv = this->hasNoDataValue();
    double result = 0.0d;
    for (uint i = 0; i < _data.size(); ++i) {
        double value = data(i, column);
        if (std::abs<double>(value) > result
            && (!has_ndv || !Util::almostEqual2sComplement(ndv, value, 1)))
            result = std::abs<double>(value);
    }
    return result;
}

// TODO: consider adding a flag to disable NDV checking (applicable to coordinates)
double DataFile::min(uint column)
{
    if (_data.size() == 0)
        Application::instance()->logError(
            "DataFile::min(): Data not loaded. Unspecified value was returned.");
    double ndv = this->getNoDataValue().toDouble();
    bool has_ndv = this->hasNoDataValue();
    double result = std::numeric_limits<double>::max();
    for (uint i = 0; i < _data.size(); ++i) {
        double value = data(i, column);
        if (value < result && (!has_ndv || !Util::almostEqual2sComplement(ndv, value, 1)))
            result = value;
    }
    return result;
}

double DataFile::minAbs(uint column)
{
    if (_data.size() == 0)
        Application::instance()->logError(
            "DataFile::minAbs(): Data not loaded. Unspecified value was returned.");
    double ndv = this->getNoDataValue().toDouble();
    bool has_ndv = this->hasNoDataValue();
    double result = std::numeric_limits<double>::max();
    for (uint i = 0; i < _data.size(); ++i) {
        double value = data(i, column);
        if (std::abs<double>(value) < result
            && (!has_ndv || !Util::almostEqual2sComplement(ndv, value, 1)))
            result = std::abs<double>(value);
    }
    return result;
}

// TODO: consider adding a flag to disable NDV checking (applicable to coordinates)
double DataFile::mean(uint column)
{
    if (_data.size() == 0)
        Application::instance()->logError(
            "DataFile::mean(): Data not loaded. Unspecified value was returned.");
    double ndv = this->getNoDataValue().toDouble();
    bool has_ndv = this->hasNoDataValue();
    double result = 0.0;
    uint count_valid = 0;
    for (uint i = 0; i < _data.size(); ++i) {
        double value = data(i, column);
        if (!has_ndv || !Util::almostEqual2sComplement(ndv, value, 1)) {
            result += value;
            ++count_valid;
        }
    }
    if (count_valid > 0)
        return result / count_valid;
    else
        return 0.0;
}

uint DataFile::getFieldGEOEASIndex(QString field_name)
{
    QStringList field_names = Util::getFieldNames(this->_path);
    for (int i = 0; i < field_names.size(); ++i) {
        if (field_names.at(i).trimmed() == field_name.trimmed())
            return i + 1;
    }
    return 0;
}

Attribute *DataFile::getAttributeFromGEOEASIndex(uint index)
{
    std::vector<ProjectComponent *>::iterator it = this->_children.begin();
    for (; it != this->_children.end(); ++it) {
        ProjectComponent *pi = *it;
        if (pi->isAttribute()) {
            Attribute *at = (Attribute *)pi;
            uint at_index = this->getFieldGEOEASIndex(at->getName());
            if (at_index == index)
                return at;
        }
    }
    return nullptr;
}

uint DataFile::getLastFieldGEOEASIndex()
{
    QStringList field_names = Util::getFieldNames(this->_path);
    return field_names.count();
}

QString DataFile::getNoDataValue() { return this->_no_data_value; }

double DataFile::getNoDataValueAsDouble()
{
    bool OK;
    double result = getNoDataValue().toDouble(&OK);
    if (!OK)
        return std::nan("");
    return result;
}

void DataFile::setNoDataValue(const QString new_ndv)
{
    this->_no_data_value = new_ndv;
    this->updateMetaDataFile();
}

bool DataFile::hasNoDataValue() { return !this->_no_data_value.trimmed().isEmpty(); }

bool DataFile::isNormal(Attribute *at)
{
    uint index_in_GEOEAS_file = this->getFieldGEOEASIndex(at->getName());
    return _nsvar_var_trn.contains(index_in_GEOEAS_file);
}

bool DataFile::isCategorical(Attribute *at)
{
    uint index_in_GEOEAS_file = this->getFieldGEOEASIndex(at->getName());
    QList<QPair<uint, QString>>::iterator it = _categorical_attributes.begin();
    for (; it != _categorical_attributes.end(); ++it) {
        if ((*it).first == index_in_GEOEAS_file)
            return true;
    }
    return false;
}

CategoryDefinition *DataFile::getCategoryDefinition(Attribute *at)
{
    uint index_in_GEOEAS_file = this->getFieldGEOEASIndex(at->getName());
    QList<QPair<uint, QString>>::iterator it = _categorical_attributes.begin();
    for (; it != _categorical_attributes.end(); ++it) {
        if ((*it).first == index_in_GEOEAS_file) {
            QString cd_file_name = (*it).second;
            return (CategoryDefinition *)Application::instance()
                ->getProject()
                ->getResourcesGroup()
                ->getChildByName(cd_file_name);
        }
    }
    return nullptr;
}

Attribute *DataFile::getVariableOfNScoreVar(Attribute *at)
{
    uint ns_var_index_in_GEOEAS_file = this->getFieldGEOEASIndex(at->getName());
    Attribute *variable = nullptr;
    if (_nsvar_var_trn.contains(ns_var_index_in_GEOEAS_file)) {
        QPair<uint, QString> var_index_in_GEOEAS_file_and_transform_table
            = _nsvar_var_trn[ns_var_index_in_GEOEAS_file];
        variable = this->getAttributeFromGEOEASIndex(
            var_index_in_GEOEAS_file_and_transform_table.first);
    }
    return variable;
}

void DataFile::deleteFromFS()
{
    File::deleteFromFS(); // delete the file itself.
    // also deletes the metadata file
    QFile file(this->getMetaDataFilePath());
    file.remove(); // TODO: throw exception if remove() returns false (fails).  Also see
                   // QIODevice::errorString() to see error message.
}

void DataFile::writeToFS()
{

    if( _data.size() <= 0 ){
        Application::instance()->logError("DataFile::writeToFS(): No data. Save failed.");
        return;
    }

    if( isSetToBePaged() ){
        Application::instance()->logError("DataFile::writeToFS(): Paged data files not currently supported in saving operation. Save failed.");
        return;
    }

    //create a new file for output
    QFile outputFile( QString( this->getPath() ).append(".new") );
    outputFile.open( QFile::WriteOnly | QFile::Text );

    QTextStream out(&outputFile);

    // if file already exists, keep copy of the file description or make up one otherwise
    QString comment;
    if (this->exists())
        comment = Util::getGEOEAScomment(this->getPath());
    else
        comment = this->getFileType() + " created by GammaRay";
    out << comment << endl;

    // next, we need to know the number of columns
    //(assumes the first data line has the correct number of variables)
    uint nvars = _data[0].size();
    out << nvars << endl;

    // get all child objects (mostly attributes directly under this file or attached under
    // another attribute)
    // we do this because some attributes (columns) may not be in the current GEO-EAS
    // file.
    std::vector<ProjectComponent *> allChildren;
    this->getAllObjects(allChildren);

    // for each GEO-EAS column index (start with 1, not zero)
    uint control = 0;
    for (uint iGEOEAS = 1; iGEOEAS <= nvars; ++iGEOEAS) {
        // find the attribute by the GEO-EAS index
        std::vector<ProjectComponent *>::iterator it = allChildren.begin();
        for (; it != allChildren.end(); ++it) {
            if ((*it)->isAttribute()) {
                Attribute *at = (Attribute *)(*it);
                if (at->getAttributeGEOEASgivenIndex() == (int)iGEOEAS) {
                    out << at->getName() << endl;
                    ++control;
                    break;
                }
            }
        }
    }

    if (control != nvars) {
        Application::instance()->logWarn("WARNING: DataFile::writeToFS(): mismatch "
                                         "between data column count and Attribute object "
                                         "count.");
        // make up names for mismatched data columns
        for (uint iGEOEAS = control; iGEOEAS <= nvars; ++iGEOEAS) {
            out << "ATTRIBUTE " << iGEOEAS << endl;
        }
    }

    //data save takes place in another thread, so we can show and update a progress bar
    //////////////////////////////////
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Saving data to filesystem...");
    progressDialog.setMinimum( 0 );
    progressDialog.setValue( 0 );
    progressDialog.setMaximum( getDataLineCount() );
    QThread* thread = new QThread();  //does it need to set parent (a QObject)?
    DataSaver* ds = new DataSaver( _data, out );  // Do not set a parent. The object cannot be moved if it has a parent.
    ds->moveToThread(thread);
    ds->connect(thread, SIGNAL(finished()), ds, SLOT(deleteLater()));
    ds->connect(thread, SIGNAL(started()), ds, SLOT(doSave()));
    ds->connect(ds, SIGNAL(progress(int)), &progressDialog, SLOT(setValue(int)));
    thread->start();
    /////////////////////////////////

    //wait for the data save to finish
    //not very beautiful, but simple and effective
    while( ! ds->isFinished() ){
        thread->wait( 200 ); //reduces cpu usage, refreshes at each 200 milliseconds
        QCoreApplication::processEvents(); //let Qt repaint widgets
    }

    // close output file
    outputFile.close();

    // deletes the current file
    QFile currentFile(this->getPath());
    currentFile.remove();
    // renames the .new file, effectively replacing the current file.
    outputFile.rename(this->getPath());
    // updates properties list so any changes appear in the project tree.
    updatePropertyCollection();
    // update the project tree in the main window.
    Application::instance()->refreshProjectTree();
}

void DataFile::updatePropertyCollection()
{
    // updates attribute collection
    this->_children.clear(); // TODO: deallocate elements/deep delete (minor memory leak)
    QStringList fields = Util::getFieldNames(this->_path);
    for (int i = 0; i < fields.size(); ++i) {
        int index_in_file = i + 1;
        // do not include the x,y,z coordinates among the attributes
        /*if( index_in_file != this->_x_field_index &&
            index_in_file != this->_y_field_index &&
            index_in_file != this->_z_field_index ){*/
        Attribute *at = new Attribute(fields[i].trimmed(), index_in_file);
        if (isWeight(
                at)) { // if attribute is a weight, it is represented as a child object
            // of the variable it refers to
            Attribute *variable = this->getVariableOfWeight(at);
            if (!variable)
                Application::instance()->logError("Error in variable-weight assignment.");
            else {
                delete at; // it is not a generic Attribute
                Weight *wgt = new Weight(fields[i].trimmed(), index_in_file, variable);
                variable->addChild(wgt);
            }
        } else if (isNormal(
                       at)) { // if attribute is a normal transform of another variable,
            // it is represented as a child object of the variable it refers to
            Attribute *variable = this->getVariableOfNScoreVar(at);
            if (!variable)
                Application::instance()->logError(
                    "Error in variable-normal variable assignment.");
            else {
                delete at; // it is not a generic Attribute
                NormalVariable *ns_var
                    = new NormalVariable(fields[i].trimmed(), index_in_file, variable);
                variable->addChild(ns_var);
            }
        } else { // common variables are direct children of files
            if (isCategorical(at))
                at->setCategorical(true);
            this->_children.push_back(at);
            at->setParent(this);
        }
        /*}*/
    }
}

void DataFile::replacePhysicalFile(const QString from_file_path)
{
    // copies the source file over the current physical file in project.
    Util::copyFile(from_file_path, _path);
    // updates properties list so any changes appear in the project tree.
    updatePropertyCollection();
    // update the project tree in the main window.
    Application::instance()->refreshProjectTree();
}

void DataFile::addVariableNScoreVariableRelationship(uint variableGEOEASindex,
                                                     uint nScoreVariableGEOEASindex,
                                                     QString trn_file_name)
{
    this->_nsvar_var_trn[nScoreVariableGEOEASindex]
        = QPair<uint, QString>(variableGEOEASindex, trn_file_name);
    // save the updated metadata to disk.
    this->updateMetaDataFile();
}

void DataFile::addGEOEASColumn(Attribute *at, const QString new_name, bool categorical,
                               CategoryDefinition *cd)
{
    // set the variable name in the destination file
    QString var_name;
    if (new_name.isEmpty())
        var_name = at->getName();
    else
        var_name = new_name;

    // get the destination file path
    QString file_path = this->getPath();

    // get the source file
    DataFile *attributes_file = (DataFile *)at->getContainingFile();

    // loads the data in source file
    attributes_file->loadData();

    // get the variable's column index in the source file
    uint column_index_in_original_file
        = attributes_file->getFieldGEOEASIndex(at->getName()) - 1;

    // get the number of data lines in the source file
    uint data_line_count = attributes_file->getDataLineCount();

    // create a new file for output
    QFile outputFile(QString(file_path).append(".new"));
    outputFile.open(QFile::WriteOnly | QFile::Text);
    QTextStream out(&outputFile);

    // set the no-data value to be used
    QString NDV;
    if (this->hasNoDataValue()) // the expected no-data value is from the destination file
                                // (this object)
        NDV = this->getNoDataValue();
    else if (attributes_file->hasNoDataValue()) { // try the one from the source file
        NDV = attributes_file->getNoDataValue();
        Application::instance()->logWarn("WARNING: DataFile::addGEOEASColumn(): no-data "
                                         "value not set for the destination file.  Using "
                                         "the no-data value from the source file: "
                                         + NDV);
    } else {
        NDV = "-9999999";
        Application::instance()->logWarn("WARNING: DataFile::addGEOEASColumn(): no-data "
                                         "value not set for both files.  Using "
                                         "-9999999.");
    }

    // open the destination file for reading
    QFile inputFile(file_path);
    if (inputFile.open(QIODevice::ReadOnly | QFile::Text)) {
        QTextStream in(&inputFile);
        uint line_index = 0;
        uint data_line_index = 0;
        uint n_vars = 0;
        uint var_count = 0;
        uint indexGEOEAS_new_variable = 0;
        // for each line in the destination file...
        while (!in.atEnd()) {
            //...read its line
            QString line = in.readLine();
            // simply copy the first line (title)
            if (line_index == 0) {
                out << line << '\n';
                // first number of second line holds the variable count
                // writes an increased number of variables.
                // TODO: try to keep the rest of the second line (not critical, but
                // desirable)
            } else if (line_index == 1) {
                n_vars = Util::getFirstNumber(line);
                out << (n_vars + 1) << '\n';
                indexGEOEAS_new_variable = n_vars + 1; // the GEO-EAS index of the added
                                                       // column equals the new number of
                                                       // columns
                // simply copy the current variable names
            } else if (var_count < n_vars) {
                out << line << '\n';
                // if we're at the last existing variable, adds an extra line for the new
                // variable
                if ((var_count + 1) == n_vars) {
                    out << var_name << '\n';
                }
                ++var_count;
                // treat the data lines until EOF
            } else {
                // if we didn't overshoot the source file...
                if (data_line_index < data_line_count) {
                    double value = attributes_file->data(data_line_index,
                                                         column_index_in_original_file);
                    // if the value was NOT defined as no-data value according to its
                    // parent file
                    if (!attributes_file->isNDV(value)) {
                        out << line << '\t' << QString::number(value) << '\n';
                        // if the value is no-data value according to the source file
                    } else {
                        out << line << '\t' << NDV << '\n';
                    }
                } else {
                    //...otherwise append the no-data value of the destination file (this
                    // object).
                    out << line << '\t' << NDV << '\n';
                }
                // keep count of the source file data lines
                ++data_line_index;
            } // if's and else's for each file line case (header, var. count, var. name
              // and data line)
            // keep count of the source file lines
            ++line_index;
        } // for each line in destination file (this)
        // close the destination file
        inputFile.close();
        // close the newly created file
        outputFile.close();
        // deletes the destination file
        inputFile.remove();
        // renames the new file, effectively replacing the destination file.
        outputFile.rename(QFile(file_path).fileName());
        // if the added column was deemed categorical, adds its GEO-EAS index and name of
        // the category definition
        // to the list of pairs for metadata keeping.
        if (categorical) {
            // TODO: guard against cd being nullptr
            _categorical_attributes.append(
                QPair<uint, QString>(indexGEOEAS_new_variable, cd->getName()));
            // update the metadata file
            this->updateMetaDataFile();
        }
        // updates properties list so any changes appear in the project tree.
        updatePropertyCollection();
        // update the project tree in the main window.
        Application::instance()->refreshProjectTree();
    }
}

uint DataFile::getDataLineCount() { return _data.size(); }

uint DataFile::getDataColumnCount()
{
    loadData();
    if (getDataLineCount() > 0)
        return _data[0].size();
    else
        return 0;
}

bool DataFile::isNDV(double value)
{
    if (!this->hasNoDataValue())
        return false;
    else {
        double ndv = this->getNoDataValue().toDouble();
        return Util::almostEqual2sComplement(ndv, value, 1);
    }
}

void DataFile::classify(uint column, UnivariateCategoryClassification *ucc,
                        const QString name_for_new_column)
{
    // load the current data from the file system
    loadData();

    // for each data row...
    std::vector<std::vector<double>>::iterator it = _data.begin();
    for (; it != _data.end(); ++it) {
        // define the default value (for class not found)
        int noClassFoundValue = -1;
        if (hasNoDataValue())
            // hopefully the file's NDV is integer
            noClassFoundValue = (int)getNoDataValue().toDouble();
        //...get the input value
        double value = (*it).at(column);
        //...get the category code corresponding to the value
        int categoryId = ucc->getCategory(value, noClassFoundValue);
        //...append the code to the current row.
        (*it).push_back(categoryId);
    }

    // create and add a new Attribute object the represents the new column
    uint newIndexGEOEAS = Util::getFieldNames(this->getPath()).count() + 1;
    Attribute *at = new Attribute(name_for_new_column, newIndexGEOEAS, true);
    // adds the attribute's GEO-EAS index (with the name of the category definition file)
    // to the metadata as a categorical attribute
    _categorical_attributes.append(
        QPair<uint, QString>(newIndexGEOEAS, ucc->getCategoryDefinition()->getName()));
    at->setParent(this);
    this->addChild(at);

    // saves the file contents to file system
    this->writeToFS();

    // update the metadata file
    this->updateMetaDataFile();
}

void DataFile::freeLoadedData() { _data.clear(); }

void DataFile::setDataPage(long firstDataLine, long lastDataLine)
{
    // does nothing if page didn't actually change
    if (firstDataLine == _dataPageFirstLine && lastDataLine == _dataPageLastLine)
        return;
    freeLoadedData();
    _dataPageFirstLine = firstDataLine;
    _dataPageLastLine = lastDataLine;
}

void DataFile::setDataPageToAll() { setDataPage(0, std::numeric_limits<long>::max()); }

bool DataFile::isSetToBePaged()
{
    return _dataPageFirstLine > 0 || _dataPageLastLine < std::numeric_limits<long>::max();
}

void DataFile::addDataColumns(std::vector< std::complex<double> > &columns,
                              const QString nameForNewAttributeOfRealPart,
                              const QString nameForNewAttributeOfImaginaryPart)
{
    // TODO: refatorar reutilizando addEmptyDataColumn e um futuro addDataColumn
    if (_data.empty()) { // no data, column will be first column
        _data.reserve(columns.size());
        std::vector<std::complex<double>>::iterator it = columns.begin();
        for (; it != columns.end(); ++it)
            _data.push_back({(*it).real(), (*it).imag()});
    } else { // there are data already, column will be appended to the current ones
        std::vector<std::complex<double>>::iterator itColumn = columns.begin();
        std::vector<std::vector<double>>::iterator itData = _data.begin();
        // hopefully both iterators end at the same time
        for (; itColumn != columns.end(), itData != _data.end(); ++itColumn, ++itData) {
            (*itData).push_back((*itColumn).real());
            (*itData).push_back((*itColumn).imag());
        }
        if (itData != _data.end() || itColumn != columns.end())
            Application::instance()->logError("DataFile::addDataColumn(): number of "
                                              "values to add mismatched number of data "
                                              "rows.");
    }

    // get the GEO-EAS index for new attributes
    uint indexGEOEASreal
        = _data[0].size()
          - 1; // assumes the first row has the correct number of data columns
    uint indexGEOEASimag
        = _data[0].size(); // assumes the first row has the correct number of data columns

    // Create new Attribute objects that correspond to the new data columns in memory
    Attribute *newAttributeReal
        = new Attribute(nameForNewAttributeOfRealPart, indexGEOEASreal);
    Attribute *newAttributeImag
        = new Attribute(nameForNewAttributeOfImaginaryPart, indexGEOEASimag);

    // Add the new Attributes as child project component of this one
    addChild(newAttributeReal);
    addChild(newAttributeImag);

    // sets this as parent of the new Attributes
    newAttributeReal->setParent(this);
    newAttributeImag->setParent(this);
}

long DataFile::addEmptyDataColumn(const QString columnName, long numberOfDataElements)
{
    std::vector<double> newColumn(numberOfDataElements, 0.0);

    if (_data.empty()) { // no data, column will be first column
        _data.reserve(newColumn.size());
        std::vector<double>::iterator it = newColumn.begin();
        for (; it != newColumn.end(); ++it)
            _data.push_back({*it});
    } else { // there are data already, column will be appended to the current ones
        std::vector<double>::iterator itColumn = newColumn.begin();
        std::vector<std::vector<double>>::iterator itData = _data.begin();
        // hopefully both iterators end at the same time
        for (; itColumn != newColumn.end(), itData != _data.end(); ++itColumn, ++itData) {
            (*itData).push_back(*itColumn);
        }
        if (itData != _data.end() || itColumn != newColumn.end())
            Application::instance()->logError("DataFile::addEmptyDataColumn(): number of "
                                              "values to add mismatched number of data "
                                              "rows.");
    }

    // get the GEO-EAS index for new attribute
    uint indexGEOEAS
        = _data[0].size(); // assumes the first row has the correct number of data column

    // Create new Attribute objects that correspond to the new data column in memory
    Attribute *newAttribute = new Attribute(columnName, indexGEOEAS);

    // Add the new Attributes as child project component of this one
    addChild(newAttribute);

    // sets this as parent of the new Attributes
    newAttribute->setParent(this);

    return indexGEOEAS - 1;
}

IAlgorithmDataSource *DataFile::algorithmDataSource()
{
    AlgorithmDataSource *concreteAspect
        = (AlgorithmDataSource *)_algorithmDataSourceInterface.get();
    concreteAspect->init(); // initialize the algorithm data source object on demand.
    return _algorithmDataSourceInterface.get();
}

int DataFile::addNewDataColumn(const QString columnName,
                               const std::vector<double> &values, CategoryDefinition *cd)
{
    // loads data from disk
    loadData();

    // get a default value in case the values vector is shorter than the data set.
    double defaultValue = 0.0;
    if (hasNoDataValue())
        defaultValue = getNoDataValueAsDouble();

    // append the values to the existing data array
    std::vector<double>::const_iterator itColumn = values.cbegin();
    std::vector<std::vector<double>>::iterator itData = _data.begin();
    // hopefully both iterators end at the same time
    for (; itColumn != values.cend(), itData != _data.end(); ++itColumn, ++itData)
        (*itData).push_back(*itColumn);

    // If the transfer was not completed (the input vector is too short), fill the
    // remainder with the default value
    for (; itData != _data.end(); ++itData)
        (*itData).push_back(defaultValue);

    // get the GEO-EAS index for new attribute
    uint indexGEOEAS
        = _data[0].size(); // assumes the first row has the correct number of data column

    // if the added column was deemed categorical, adds its GEO-EAS index and name of the
    // category definition
    // to the list of pairs for metadata keeping.
    if (cd) {
        _categorical_attributes.append(QPair<uint, QString>(indexGEOEAS, cd->getName()));
        // update the metadata file
        this->updateMetaDataFile();
    }

    // Create a new Attribute object that correspond to the new data column in memory
    Attribute *newAttribute = new Attribute(columnName, indexGEOEAS);

    // Set the new Attribute as categorical
    newAttribute->setCategorical(true);

    // Add the new Attributes as child project component of this one
    addChild(newAttribute);

    // sets this as parent of the new Attributes
    newAttribute->setParent(this);

    // update the file
    writeToFS();

    // returns the index of the new column
    return indexGEOEAS - 1;
}

void DataFile::deleteVariable( uint columnToDelete )
{
    if( Util::getFieldNames( getPath() ).size() < 2 ){ //getDataColumnCount() triggers a loadData().
        Application::instance()->logError("DataFile::deleteVariable(): Cannot delete the single variable of a file.  Remove the file instead.");
        return;
    }

    //delete any data loaded to memory.
    freeLoadedData();

    uint columnToDeleteGEOEAS = columnToDelete + 1;

    //remove any references to the variable in the list of nscore-variable relation
    QMap<uint, QPair<uint, QString> >::iterator it = _nsvar_var_trn.begin();
    for(; it != _nsvar_var_trn.end();){
        if( it.key() == columnToDeleteGEOEAS || it->first == columnToDeleteGEOEAS )
            it = _nsvar_var_trn.erase( it ); //QMap::erase() does the increment to the next element (do not add ++ here)
        else
            ++it;
    }

    //Decrement all indexes greater than the deleted variable index in the list of nscore-variable relation
    QMap<uint, QPair<uint, QString> > temp;
    it = _nsvar_var_trn.begin();
    for(; it != _nsvar_var_trn.end(); ++it){
        uint key = it.key();
        uint index = it->first;
        if( key > columnToDeleteGEOEAS )
            --key;
        if( index > columnToDeleteGEOEAS )
            --index;
        temp.insert( key, QPair<uint, QString>( index, it->second ) );
    }
    _nsvar_var_trn.swap(temp);

    //remove any references to the variable in the list of categorical attributes
    //also decrements the indexes greater than the deleted variable index (can do this wat with QList)
    QList< QPair<uint, QString> >::iterator it2 = _categorical_attributes.begin();
    for(; it2 != _categorical_attributes.end();){
        if( it2->first == columnToDeleteGEOEAS )
            it2 = _categorical_attributes.erase( it2 ); //QList::erase() does the increment to the next element (do not add ++ here)
        else{
            if( it2->first > columnToDeleteGEOEAS )
                it2->first = it2->first - 1;
            ++it2;
        }
    }

    //updates the metadata file in the project
    updateMetaDataFile();

    //remove the data column from the physical file
    {
       //file manipulation takes place in another thread, so we can show and update a progress bar
       //////////////////////////////////
       QProgressDialog progressDialog;
       progressDialog.show();
       progressDialog.setLabelText("Removing variable data from file " + _path + "...");
       progressDialog.setMinimum( 0 );
       progressDialog.setValue( 0 );
       progressDialog.setMaximum( getFileSize() / 100 ); //see VariableRemover::doRemove(). Dividing by 100 allows a max value of ~400GB when converting from long to int
       QThread* thread = new QThread();  //does it need to set parent (a QObject)?
       VariableRemover* vr = new VariableRemover( *this, columnToDelete ); // Do not set a parent. The object cannot be moved if it has a parent.
       vr->moveToThread(thread);
       vr->connect(thread, SIGNAL(finished()), vr, SLOT(deleteLater()));
       vr->connect(thread, SIGNAL(started()), vr, SLOT(doRemove()));
       vr->connect(vr, SIGNAL(progress(int)), &progressDialog, SLOT(setValue(int)));
       thread->start();
       /////////////////////////////////

       //wait for the removal operation to finish
       //not very beautiful, but simple and effective
       while( ! vr->isFinished() ){
           thread->wait( 200 ); //reduces cpu usage, refreshes at each 200 milliseconds
           QCoreApplication::processEvents(); //let Qt repaint widgets
       }
    }

    //update the child Attribute objects
    updatePropertyCollection();

    //update the project tree in the main window.
    Application::instance()->refreshProjectTree();

    //reset the algorithm data source object
    _algorithmDataSourceInterface.reset( new AlgorithmDataSource(*this) );
}

double DataFile::variance(uint column)
{
    if (_data.size() == 0) {
        Application::instance()->logError(
            "DataFile::variance(): Data not loaded. Zero was returned.");
        return 0.0;
    }
    // collect the valid values
    double ndv = this->getNoDataValue().toDouble();
    bool has_ndv = this->hasNoDataValue();
    std::vector<double> values;
    values.reserve(getDataLineCount());
    for (uint i = 0; i < _data.size(); ++i) {
        double value = data(i, column);
        if (!has_ndv || !Util::almostEqual2sComplement(ndv, value, 1)) {
            values.push_back(value);
        }
    }
    // compute the variance
    double mean = this->mean(column);
    std::vector<double> diff(values.size());
    std::transform(values.begin(), values.end(), diff.begin(),
                   [mean](double x) { return x - mean; });
    double squaredSum(std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0));
    double stdev(std::sqrt(squaredSum / (double)values.size()));
    return stdev * stdev;
}

double DataFile::correlation(uint columnX, uint columnY)
{
    double sum_X = 0.0, sum_Y = 0.0, sum_XY = 0.0;
    double squareSum_X = 0.0, squareSum_Y = 0.0;
    int n = getDataLineCount();
    int nValidValues = 0;
    double ndv = this->getNoDataValue().toDouble();
    bool has_ndv = this->hasNoDataValue();

    for (int i = 0; i < n; i++) {
        double X = data(i, columnX);
        double Y = data(i, columnY);

        // if one of the values is invalid, ignore the record
        if (has_ndv && (Util::almostEqual2sComplement(ndv, X, 1)
                        || Util::almostEqual2sComplement(ndv, Y, 1))) {
            continue;
        }

        // sum of elements of array X.
        sum_X += X;

        // sum of elements of array Y.
        sum_Y += Y;

        // sum of X[i] * Y[i].
        sum_XY += (X * Y);

        // sum of square of array elements.
        squareSum_X += (X * X);
        squareSum_Y += (Y * Y);

        ++nValidValues;
    }

    // use formula for calculating correlation coefficient.
    return (nValidValues * sum_XY - sum_X * sum_Y)
           / std::sqrt((nValidValues * squareSum_X - sum_X * sum_X)
                       * (nValidValues * squareSum_Y - sum_Y * sum_Y));
}
