#ifndef VIEW3DLISTRECORD_H
#define VIEW3DLISTRECORD_H

#include <QString>
#include <QMetaType> //Q_DECLARE_METATYPE macro

/** This class is simply a data structure to hold multiple information in Viewer3DListWidget's items' data. */
class View3DListRecord
{
public:
    View3DListRecord( );
    View3DListRecord( QString p_object_locator, uint p_instance );

    /** The project object locator (a kind of URL). */
    QString objectLocator;

    /** The same object may appear multiple times in the list, thus we distinguish them via the instance number. */
    uint instance;

    /** Returns a text containing the object's data. Normally useful to give feedback to the user.*/
    QString getDescription() const;

};
Q_DECLARE_METATYPE(View3DListRecord) //allows the class to be wrapped in a QVariant

/**
 * This less-than operator enables the View3DListRecord class as key-able
 * in QMap (and possibly other containers).
 */
inline bool operator<(const View3DListRecord &e1, const View3DListRecord &e2){
    if ( e1.objectLocator != e2.objectLocator )
        return e1.objectLocator < e2.objectLocator;
    return e1.instance < e2.instance;
}


#endif // VIEW3DLISTRECORD_H
