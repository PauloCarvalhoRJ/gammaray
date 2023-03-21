#ifndef LISTBUILDER_H
#define LISTBUILDER_H


#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>


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
        for (int i = 0; i < mOuput->count(); i++)
            selected << mOuput->item(i)->text();
        return selected;
    }

private:
    void init() {
        QHBoxLayout *layout = new QHBoxLayout(this);
        mInput = new QListWidget;
        mOuput = new QListWidget;

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
        layout->addWidget(mOuput);

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
        connect(mOuput, &QListWidget::itemSelectionChanged, this,
                &ListBuilder::setStatusButton);
        connect(mInput, &QListWidget::itemSelectionChanged, this,
                &ListBuilder::setStatusButton);
        connect(mBtnMoveToAvailable, &QPushButton::clicked,
                [=]() { mOuput->addItem(mInput->takeItem(mInput->currentRow())); });

        connect(mBtnMoveToSelected, &QPushButton::clicked,
                [=]() { mInput->addItem(mOuput->takeItem(mOuput->currentRow())); });

        connect(mButtonToAvailable, &QPushButton::clicked, [=]() {
            while (mOuput->count() > 0) {
                mInput->addItem(mOuput->takeItem(0));
            }
        });

        connect(mButtonToSelected, &QPushButton::clicked, [=]() {
            while (mInput->count() > 0) {
                mOuput->addItem(mInput->takeItem(0));
            }
        });

        connect(mBtnUp, &QPushButton::clicked, [=]() {
            int row = mOuput->currentRow();
            QListWidgetItem *currentItem = mOuput->takeItem(row);
            mOuput->insertItem(row - 1, currentItem);
            mOuput->setCurrentRow(row - 1);
        });

        connect(mBtnDown, &QPushButton::clicked, [=]() {
            int row = mOuput->currentRow();
            QListWidgetItem *currentItem = mOuput->takeItem(row);
            mOuput->insertItem(row + 1, currentItem);
            mOuput->setCurrentRow(row + 1);
        });
    }

    void setStatusButton() {
        mBtnUp->setDisabled(mOuput->selectedItems().isEmpty() ||
                            mOuput->currentRow() == 0);
        mBtnDown->setDisabled(mOuput->selectedItems().isEmpty() ||
                              mOuput->currentRow() == mOuput->count() - 1);
        mBtnMoveToAvailable->setDisabled(mInput->selectedItems().isEmpty());
        mBtnMoveToSelected->setDisabled(mOuput->selectedItems().isEmpty());
    }

    QListWidget *mInput;
    QListWidget *mOuput;

    QPushButton *mButtonToAvailable;
    QPushButton *mButtonToSelected;

    QPushButton *mBtnMoveToAvailable;
    QPushButton *mBtnMoveToSelected;

    QPushButton *mBtnUp;
    QPushButton *mBtnDown;
};


#endif // LISTBUILDER_H
