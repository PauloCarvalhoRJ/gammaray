#ifndef LISTBUILDER_H
#define LISTBUILDER_H

#include <QWidget>

class Attribute;
class QListWidget;
class QPushButton;
class DataFile;

/**
 * This widget is a Qt implementation of the well known ordered list builder found in many software.
 * It has two lists side-by-side with the classic <, >, >>, <<, Up and Down buttons to build ordered lists via GUI.
 * One can use the "promote to" feature in Qt Designer/Creator to place this widget visually in the form designer.
 *
 * Example usage and left hand list initialization with ten items named "intem-1", "item-2", etc.
 *
 *   TwoListSelection w;
 *   QStringList input;
 *   for (int i = 0; i < 10; i++) {
 *     input << QString("item-%1").arg(i);
 *   }
 *   w.addAvailableItems(input);
 *
 * This was largely modified from the original one authored by StackOverflow user eyllanesc
 * Source: https://github.com/eyllanesc/stackoverflow/tree/master/questions/48327558
 */
class ListBuilder : public QWidget {

    Q_OBJECT

public:
    explicit ListBuilder(QWidget *parent = nullptr);

    /**
     * Populates the left-hand list with the item names in the given vector.
     */
    void addAvailableItems(const QStringList &items);

    /** Returns a string vector containing the names of the
     * items in the right-hand list.
     */
    QStringList seletedItems();

    /**
     * Returns all the items that are Attributes in the right-hand list in the top->bottom order.
     * Selected items that are not Attributes are ignored.
     */
    std::vector<Attribute*> getSelectedAttributes();

    /**
     * Moves from the left-hand list to the right-hand list the items that:
     * a) are attributes and
     * b) whose names are found in the passed list.
     * The items appear in the right-hand list in the order they are found in
     * the passed vector.
     */
    void preSelectAttributes( const std::vector<Attribute*> list_of_attributes );

public Q_SLOTS:

    /**
     * Inits or repopulates the left-hand list with the variables of the given data file.
     * Empties the right-hand list.
     */
    void onInitListWithVariables( DataFile *file );

private:

    /** Place all the GUI elements. */
    void makeUI() ;

    /** Connects all necessary siganls to their respective
     * slots to provide funcionality.
     */
    void connectWidgets();

    /** Enable/disable buttons depending on current list contents
     * and/or selected item.
     */
    void enableDisableButtons() ;

    QListWidget *m_leftHandList;
    QListWidget *m_rightHandList;

    QPushButton *m_btnMoveAllFromRightToLeft;
    QPushButton *m_btnMoveAllFromLeftToRight;

    QPushButton *m_btnMoveOneToRight;
    QPushButton *m_btnMoveOneToLeft;

    QPushButton *m_btnUp;
    QPushButton *m_btnDown;

    DataFile *m_dataFile;
};


#endif // LISTBUILDER_H
