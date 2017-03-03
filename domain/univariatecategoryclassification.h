#ifndef UNIVARIATECATEGORYCLASSIFICATION_H
#define UNIVARIATECATEGORYCLASSIFICATION_H

#include "triads.h"

typedef Triads<double,double,int> DoubleDoubeIntTriplets;

class CategoryDefinition;

/**
 * @brief The UnivariateCategoryClassification class is a set of triplets of double, double, int representing
 * value intervals (the two doubles) that must be mapped to a certain category id (the integer).
 */
class UnivariateCategoryClassification : public DoubleDoubeIntTriplets
{
public:

    UnivariateCategoryClassification( CategoryDefinition* cd, QString path );

    /** Constructor with the name of a CategoryDefinition (delayed load).
      * @param categoryDefinitionName The name of a CategoryDefinition object existing in the project.
      */
    UnivariateCategoryClassification( const QString categoryDefinitionName, QString path );

    ~UnivariateCategoryClassification();

    /**
     * Returns the category id corresponding to the given value.
     * Returns -1 if no category is found.
     */
    int getCategory( double value );

    /**
     * Returns the categorical definition used to build this categorical classification.
     */
    CategoryDefinition* getCategoryDefinition(){ return m_categoryDefinition; }

    // ProjectComponent interface
public:
    QIcon getIcon(){return QIcon(":icons/catuniclass16");}
    void save(QTextStream *txt_stream);

    // File interface
public:
    bool canHaveMetaData(){ return false; }
    QString getFileType(){ return "UNIVARIATECATEGORYCLASSIFICATION"; }
    void updateMetaDataFile(){}
    virtual bool isEditable(){ return true; }
    QWidget *createContentElementWidget();
    QWidget *createWidgetFilledWithContentElement( uint iContent );
    void addContentElementFromWidget( QWidget* w );

private:
    CategoryDefinition* m_categoryDefinition;
    /** This value does not need to be updated after m_categoryDefinition pointer is set. */
    QString m_categoryDefinitionNameForDelayedLoad;

    /** Sets m_categoryDefinition by searching a CategoryDefinition with the name given by
     * m_categoryDefinitionNameForDelayedLoad.
     * Returns true if the search was successful.
     */
    bool setCategoryDefinition();
};

#endif // UNIVARIATECATEGORYCLASSIFICATION_H
