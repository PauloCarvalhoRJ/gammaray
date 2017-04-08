#ifndef CATEGORYDEFINITION_H
#define CATEGORYDEFINITION_H

#include "triads.h"
#include <QList>

typedef Triads<int,int,QString> IntIntQStringTriplets;

class GSLibParType;

/**
 * @brief The CategoryDefinition class represents a file containing triplets of
 * category code (an integer), a GSLib color code (an integer) and a category name (a string).
 * The triplets are category definitions, for example, to define sandstone as having code 1,
 * color yellow and name "Arenite".
 */
class CategoryDefinition : public IntIntQStringTriplets
{
public:
    CategoryDefinition( QString path );
    ~CategoryDefinition();

    int getCategoryCount(){ return getTripletCount(); }
    int getCategoryCode( int category_index ){ return get1stValue(category_index); }
    int getColorCode( int category_index ){ return get2ndValue(category_index); }
    QString getCategoryName( int category_index ){ return get3rdValue(category_index); }

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
    QWidget *createContentElementWidget();
    QWidget *createWidgetFilledWithContentElement( uint iContent );
    void addContentElementFromWidget( QWidget* w );
    bool isDataFile(){ return false; }

protected:
    /** This member stores the pointers to the created GSLibParTypes
     * instantiated just to to get their automatically created editing widgets.
     * They should be deleted at some time (destructor?).
     */
    QList<GSLibParType*> m_stashOfCreatedParameters;
};

#endif // CATEGORYDEFINITION_H
