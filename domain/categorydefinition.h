#ifndef CATEGORYDEFINITION_H
#define CATEGORYDEFINITION_H

#include "domain/quintuplets.h"
#include <QList>

typedef Quintuplets<int,int,QString,QColor,QString> IntIntQStringQColorQStringQuintuplets;

class GSLibParType;

/**
 * @brief The CategoryDefinition class represents a file containing quintuplets of
 * category code (an integer), a GSLib color code (an integer), a category name (a string),
 * a custom RGB color (a QColor) and an extended name (a string).
 * The triplets are category definitions, for example, to define sandstone as having code 1,
 * color yellow and name "Arenite".
 */
class CategoryDefinition : public IntIntQStringQColorQStringQuintuplets
{
public:
    CategoryDefinition( QString path );
    ~CategoryDefinition();

    int getCategoryCount(){ return getQuintupletCount(); }

    int getCategoryCode( int category_index ){ return get1stValue(category_index); }
    int getColorCode( int category_index ){ return get2ndValue(category_index); }
    QString getCategoryName( int category_index ){ return get3rdValue(category_index); }
    QColor getCustomColor( int category_index ){ return get4thValue(category_index); }
    QString getExtendedCategoryName( int category_index ){ return get5thValue(category_index); }

    /** Returns the category name given its code. */
    QString getCategoryNameByCode( int category_code );

    /** Returns the category GSLib color code given its code. */
    uint getCategoryColorByCode( int category_code );

    /** Returns the index of the category given its code.
     * Returns -1 of the category code is not found.
     */
    int getCategoryIndex( int category_code );

    /** Tests whether the passed code exists in this collection. */
    bool codeExists( int category_code );

    /** Tests whether there is a category with the given name. */
    bool categoryExistByName( const QString& catName );

    /** Returns a category code by its name.
     * Returns -999 if the name is not found.
     */
    int getCategoryCodeByName( const QString& catName );

    // ProjectComponent interface
public:
    QIcon getIcon(){return QIcon(":icons/catdef16");}
    void save(QTextStream *txt_stream);

    // File interface
public:
    bool canHaveMetaData(){ return false; }
    QString getFileType(){ return "CATEGORYDEFINITION"; }
    void updateMetaDataFile(){}
    virtual bool isEditable(){ return true; }
    virtual QWidget *createContentElementWidget();
    QWidget *createWidgetFilledWithContentElement( uint iContent );
    void addContentElementFromWidget( QWidget* w );
    bool isDataFile(){ return false; }
	bool isDistribution(){ return false; }
    virtual void clearLoadedContents();

protected:
    /** This member stores the pointers to the created GSLibParTypes
     * instantiated just to to get their automatically created editing widgets.
     * They should be deleted at some time (destructor?).
     */
    QList<GSLibParType*> m_stashOfCreatedParameters;

    void clearStashOfCreatedParameters();
};

#endif // CATEGORYDEFINITION_H
