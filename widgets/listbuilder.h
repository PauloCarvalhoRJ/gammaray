#ifndef LISTBUILDER_H
#define LISTBUILDER_H


#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>
#include "domain/projectcomponent.h"
#include "domain/datafile.h"
#include "domain/attribute.h"
#include "domain/application.h"

/**
 * This widget is a header-only Qt implementation of the known ordered list builder found in many software.
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
 * This is modified from the original one authored by StackOverflow user eyllanesc
 * Source: https://github.com/eyllanesc/stackoverflow/tree/master/questions/48327558
 */

class DataFile;

class ListBuilder : public QWidget {

    Q_OBJECT

public:
    explicit ListBuilder(QWidget *parent = nullptr) : QWidget{parent} {
        init();
        connections();
    }

    void addAvailableItems(const QStringList &items) { mInput->addItems(items); }

    QStringList seletedItems() {
        QStringList selected;
        for (int i = 0; i < mOutput->count(); i++)
            selected << mOutput->item(i)->text();
        return selected;
    }

    /**
     * Returns all the items that are Attributes in the right-hand list in the top->bottom order.
     * Selected items that are not Attributes are ignored.
     */
    std::vector<Attribute*> getSelectedAttributes(){
        std::vector<Attribute*> result;
        if( ! m_dataFile ){
            Application::instance()->logWarn("ListBuilder::getSelectedAttributes(): m_dataFile == nullptr. Returning empty list.");
            return result;
        }
        for( int i = 0; i < mOutput->count(); ++i ){
            //by selecting a variable name, surely the object is an Attribute
            Attribute* at = dynamic_cast<Attribute*>( m_dataFile->getChildByName( mOutput->item(i)->text() ) );
            if( ! at ){
                Application::instance()->logWarn("ListBuilder::getSelectedAttributes(): Item (" +
                                                 mOutput->item(i)->text() + ") not found or is not an Attribute.");
            } else {
                result.push_back( at );
            }
        }
        return result;
    }

public Q_SLOTS:

    /**
     * Inits or repopulates the left-hand list with the variables of the given data file.
     * Empties the right-hand list.
     */
    void onInitListWithVariables( DataFile *file ){
        //set data file
        m_dataFile = file;
        //clear left-hand list
        mInput->clear();
        //clear right-hand list
        mOutput->clear();
        //sanity check
        if( ! file )
            return;
        //fetch all project items under the data file object (e.g. variables).
        std::vector<ProjectComponent*> all_contained_objects;
        file->getAllObjects( all_contained_objects );
        //iterate through all objects...
        std::vector<ProjectComponent*>::iterator it = all_contained_objects.begin();
        for(; it != all_contained_objects.end(); ++it){
            ProjectComponent* pc = *it;
            //...if project item is a variable...
            if( pc->isAttribute() ){
                //...adds it to left-hand list
                //SEE https://www.qtcentre.org/threads/8723-Setting-an-icon-to-an-Item-in-a-QlistWidget ;
                QListWidgetItem* item = new QListWidgetItem( pc->getIcon(), pc->getName() );
                mInput->addItem( item );
            }
        }
    }

private:
    void init() {
        QHBoxLayout *layout = new QHBoxLayout(this);
        mInput = new QListWidget;
        mOutput = new QListWidget;

        mButtonToSelected = new QPushButton(">>");
        mBtnMoveToAvailable = new QPushButton(">");
        mBtnMoveToSelected = new QPushButton("<");
        mButtonToAvailable = new QPushButton("<<");

        layout->addWidget(mInput);

        QVBoxLayout *layoutm = new QVBoxLayout;
        layoutm->addItem(
                    new QSpacerItem(10, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
        layoutm->addWidget(mButtonToSelected);
        layoutm->addWidget(mBtnMoveToAvailable);
        layoutm->addWidget(mBtnMoveToSelected);
        layoutm->addWidget(mButtonToAvailable);
        layoutm->addItem(
                    new QSpacerItem(10, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));

        layout->addLayout(layoutm);
        layout->addWidget(mOutput);

        mBtnUp = new QPushButton("Up");
        mBtnDown = new QPushButton("Down");

        QVBoxLayout *layoutl = new QVBoxLayout;
        layoutl->addItem(
                    new QSpacerItem(10, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
        layoutl->addWidget(mBtnUp);
        layoutl->addWidget(mBtnDown);
        layoutl->addItem(
                    new QSpacerItem(10, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));

        layout->addLayout(layoutl);
        setStatusButton();
    }

    void connections() {
        connect(mOutput, &QListWidget::itemSelectionChanged, this,
                &ListBuilder::setStatusButton);
        connect(mInput, &QListWidget::itemSelectionChanged, this,
                &ListBuilder::setStatusButton);
        connect(mBtnMoveToAvailable, &QPushButton::clicked,
                [=]() { mOutput->addItem(mInput->takeItem(mInput->currentRow())); });

        connect(mBtnMoveToSelected, &QPushButton::clicked,
                [=]() { mInput->addItem(mOutput->takeItem(mOutput->currentRow())); });

        connect(mButtonToAvailable, &QPushButton::clicked, [=]() {
            while (mOutput->count() > 0) {
                mInput->addItem(mOutput->takeItem(0));
            }
        });

        connect(mButtonToSelected, &QPushButton::clicked, [=]() {
            while (mInput->count() > 0) {
                mOutput->addItem(mInput->takeItem(0));
            }
        });

        connect(mBtnUp, &QPushButton::clicked, [=]() {
            int row = mOutput->currentRow();
            QListWidgetItem *currentItem = mOutput->takeItem(row);
            mOutput->insertItem(row - 1, currentItem);
            mOutput->setCurrentRow(row - 1);
        });

        connect(mBtnDown, &QPushButton::clicked, [=]() {
            int row = mOutput->currentRow();
            QListWidgetItem *currentItem = mOutput->takeItem(row);
            mOutput->insertItem(row + 1, currentItem);
            mOutput->setCurrentRow(row + 1);
        });
    }

    void setStatusButton() {
        mBtnUp->setDisabled(mOutput->selectedItems().isEmpty() ||
                            mOutput->currentRow() == 0);
        mBtnDown->setDisabled(mOutput->selectedItems().isEmpty() ||
                              mOutput->currentRow() == mOutput->count() - 1);
        mBtnMoveToAvailable->setDisabled(mInput->selectedItems().isEmpty());
        mBtnMoveToSelected->setDisabled(mOutput->selectedItems().isEmpty());
    }

    QListWidget *mInput;
    QListWidget *mOutput;

    QPushButton *mButtonToAvailable;
    QPushButton *mButtonToSelected;

    QPushButton *mBtnMoveToAvailable;
    QPushButton *mBtnMoveToSelected;

    QPushButton *mBtnUp;
    QPushButton *mBtnDown;

    DataFile *m_dataFile;
};


#endif // LISTBUILDER_H
