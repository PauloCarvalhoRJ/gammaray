#ifndef DATAFILE_H
#define DATAFILE_H

#include "file.h"
#include <vector>
#include <QMap>

class Attribute;
class UnivariateCategoryClassification;

/**
 * @brief The DataFile class is the base class of all project components that are
 *  files with scientific data, namely Point Set and Cartesian Grid.
 * This class basically extends File with a data table and its associated load, save and access methods.
 */

class DataFile : public File
{
public:
    DataFile(QString path);

    /**
      *  Loads the tabular data in file into the _data table.
      */
    void loadData();

    /**
      *  Returns the data at the given position.
      *  This does not follow GEO_EAS convention, so the first data value, at the first line and first column of the file
      *  is at (0,0). ATTENTION: the coordinates are relative to file contents.  Do not confuse with
      *  grid coordinates in regular grids.
      *  @note Make sure to load data with loadData(), as this method does not check it for performance reasons.
      */
    double data(uint line, uint column);

    /**
     * Returns the maximum value in the given column.
     * First column is 0.
     */
    double max( uint column );

    /**
     * Returns the minimum value in the given column.
     * First column is 0.
     */
    double min( uint column );

    /**
     * Returns the arithmetic mean of the values in the given column.
     * First column is 0.
     */
    double mean( uint column );

    /**
     * Returns the index of the given field in GEO-EAS convention (first is 1).
     * If the given field name does not exist, returns zero.
     */
    uint getFieldGEOEASIndex( QString field_name );

    /**
     * Returns the Attribute that has the given index in the GEO-EAS file (first is 1).
     * Returns a null pointer of there is no such Attribute.
     */
    Attribute* getAttributeFromGEOEASIndex( uint index );

    /**
     * Returns the GEO-EAS index of the last field of the data file.
     */
    uint getLastFieldGEOEASIndex();

    /**
     * Returns the user-given no-data value text when this data file were added to the project.
     */
    virtual QString getNoDataValue();

    /**
     * Sets the new no-data value.  Empty string means not set.
     */
    virtual void setNoDataValue( const QString new_ndv );

    /**
     * Returns whether the user has given a no-data value when this data file were added to the project.
     */
    virtual bool hasNoDataValue();

    /**
     * Returns whether the given attribute is a declustering weight of another attribute.
     */
    virtual bool isWeight( Attribute* at ) = 0;

    /**
     * Returns whether the given attribute is a normal scores transform of another attribute.
     */
    bool isNormal( Attribute* at );

    /**
     * Returns whether the given attribute is a categorical variable.
     */
    bool isCategorical( Attribute* at );

    /**
     * Returns the variable associated with the declustering weight attribute.
     */
    virtual Attribute* getVariableOfWeight( Attribute* at ) = 0;

    /**
     * Returns the original variable that was normal score transformed into the given attribute.
     */
    Attribute* getVariableOfNScoreVar( Attribute* at );

    /** Replaces the contents of the current physical file with the contents
      * of the given file.
      */
    void replacePhysicalFile( const QString from_file_path );

    /**
     *  Adds a new variable-normal variable relationship in the form given by their
     *  index in the GEO-EAS file.
     * @param trn_file_name Only the file name for the transformation table used to transform
     *                      the original variable.  The file must exist in the project directory.
     */
    void addVariableNScoreVariableRelationship( uint variableGEOEASindex,
                                                uint nScoreVariableGEOEASindex,
                                                QString trn_file_name );

    /**
     * Returns the collections of triads:
     * 1- GEO-EAS index of normal score variable.
     * 2- GEO-EAS index of varibale that was normal score transformed into variable in 1-.
     * 3- File name of the transform table (.trn file).
     */
    QMap<uint, QPair<uint, QString> > getNSVarVarTrnTriads(){ return _nsvar_var_trn; }

    /**
     * Returns the list of GEO-EAS indexes (1st == 1, not zero) of the attributes considered as categorical variables.
     */
    QList<uint> getCategoricalAttributes(){ return _categorical_attributes; }

    /**
     * Adds the values stored in an Attribute object as a GEO-EAS column to the given data file.
     * The new column is appended to the end of each of the file's lines.
     * No check is done whether the Attribute has the same value count as data line count in the file.
     * If the Attribute has more values than data lines, the excess values will be ignored.  If there are less velues,
     * the remaining data file lines will be filled with the no-data value: of this DataFile object; if not set, of the
     * source file (Attribute's parent file) or -9999999 if both files do not have a no-data value set.
     * Values of Attribute that are no-data values according to its parent file are rewritten as no-data values
     * according to this DataFile if it has been set; otherwise, -9999999.
     * Of course, after the addition, this object is updated from the changed file contents.
     * @param new_name If empty, the variable name (at->getName()) is used in this file.
     */
    void addGEOEASColumn( Attribute *at, const QString new_name = "");

    /**
     * Returns the number of data lines read from file.
     * Make sure to have called loadData() prior to this call, otherwise zero will be returned.
     * Also if you made changes to the data file, it is necessary to call loadData() again to update
     * the object contents.
     */
    uint getDataLineCount();

    /**
     * Returns the number of data columns (variables) of the first line of file (assumes all lines have the
     * same number of columns).
     * Make sure to have called loadData() prior to this call, otherwise zero will be returned.
     * Also if you made changes to the data file, it is necessary to call loadData() again to update
     * the object contents.
     */
    uint getDataColumnCount();

    /** Returns whether the given value equals the no-data value set for this data file.
     * If a no-data value has not been set, this method always returns false.
     * TODO: possible performance bottleneck.
     */
    bool isNDV( double value );

    /**
     * Adds a new data column (variable/attribute) containing categorical values computed from the
     * values of the given variable as a function of the univariate category classification
     * map passed as parameter.  Nothing happens is nullptr is passed as parameter.
     * First column is zero.
     */
    void classify(uint column, UnivariateCategoryClassification* ucc , const QString name_for_new_column);

//File interface
    void deleteFromFS();
    void writeToFS();

protected:

    /**
     * The data table.  A matrix of doubles.
     */
    std::vector< std::vector<double> > _data;

    /** The no-data value specified by the user. */
    QString _no_data_value;

    /** Repopulates the _children collection.  Mainly useful when there are changes in the physical point set file. */
    void updatePropertyCollection();

    /**
     * pairs relating n-scored variables (first uint) and variables
     * (second uint) by their GEO-EAS indexes (1=first), also
     * the transform table file (QString member) is indicated in the relation
     */
    QMap<uint, QPair<uint, QString> > _nsvar_var_trn;

    /**
     * List of GEO-EAS indexes (1st = 1, not zero) of attributes considered as categorical variables.
     */
    QList<uint> _categorical_attributes;
};

#endif // DATAFILE_H
