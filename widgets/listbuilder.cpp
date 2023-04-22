#include "listbuilder.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>
#include "domain/projectcomponent.h"
#include "domain/datafile.h"
#include "domain/attribute.h"
#include "domain/application.h"

ListBuilder::ListBuilder(QWidget *parent) : QWidget{parent} {
    makeUI();
    connectWidgets();
}

void ListBuilder::addAvailableItems(const QStringList &items) {
    m_leftHandList->addItems(items);
}

QStringList ListBuilder::seletedItems() {
    QStringList selected;
    for (int i = 0; i < m_rightHandList->count(); i++)
        selected << m_rightHandList->item(i)->text();
    return selected;
}

std::vector<Attribute*> ListBuilder::getSelectedAttributes(){
    std::vector<Attribute*> result;
    if( ! m_dataFile ){
        Application::instance()->logWarn("ListBuilder::getSelectedAttributes(): m_dataFile == nullptr. Returning empty list.");
        return result;
    }
    for( int i = 0; i < m_rightHandList->count(); ++i ){
        //by selecting a variable name, surely the object is an Attribute
        Attribute* at = dynamic_cast<Attribute*>( m_dataFile->getChildByName( m_rightHandList->item(i)->text() ) );
        if( ! at ){
            Application::instance()->logWarn("ListBuilder::getSelectedAttributes(): Item (" +
                                             m_rightHandList->item(i)->text() + ") not found or is not an Attribute.");
        } else {
            result.push_back( at );
        }
    }
    return result;
}

void ListBuilder::preSelectAttributes( const std::vector<Attribute*> list_of_attributes ){
    for( Attribute* at : list_of_attributes ){
        QList<QListWidgetItem *> search_results = m_leftHandList->findItems( at->getName(), Qt::MatchExactly );
        if( search_results.empty() ){
            Application::instance()->logWarn("ListBuilder::preSelectAttributes(): item name " +at->getName() +
                                             " not found in left-hand list.");
            continue;
        } else if( search_results.size() > 1 ){
            Application::instance()->logWarn("ListBuilder::preSelectAttributes(): more than one item named "
                                             + at->getName() + " found in left-hand list."
                                             + " Using the first one.");
        }
        m_rightHandList->addItem(m_leftHandList->takeItem(m_leftHandList->row( search_results[0] )));
    }
}

void ListBuilder::onInitListWithVariables( DataFile *file ){
    //set data file
    m_dataFile = file;
    //clear left-hand list
    m_leftHandList->clear();
    //clear right-hand list
    m_rightHandList->clear();
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
            m_leftHandList->addItem( item );
        }
    }
}


void ListBuilder::makeUI() {
    QHBoxLayout *layout = new QHBoxLayout(this);
    m_leftHandList = new QListWidget;
    m_rightHandList = new QListWidget;

    m_btnMoveAllFromLeftToRight = new QPushButton(">>");
    m_btnMoveOneToRight = new QPushButton(">");
    m_btnMoveOneToLeft = new QPushButton("<");
    m_btnMoveAllFromRightToLeft = new QPushButton("<<");

    layout->addWidget(m_leftHandList);

    QVBoxLayout *layoutm = new QVBoxLayout;
    layoutm->addItem(
                new QSpacerItem(10, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
    layoutm->addWidget(m_btnMoveAllFromLeftToRight);
    layoutm->addWidget(m_btnMoveOneToRight);
    layoutm->addWidget(m_btnMoveOneToLeft);
    layoutm->addWidget(m_btnMoveAllFromRightToLeft);
    layoutm->addItem(
                new QSpacerItem(10, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));

    layout->addLayout(layoutm);
    layout->addWidget(m_rightHandList);

    m_btnUp = new QPushButton("Up");
    m_btnDown = new QPushButton("Down");

    QVBoxLayout *layoutl = new QVBoxLayout;
    layoutl->addItem(
                new QSpacerItem(10, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));
    layoutl->addWidget(m_btnUp);
    layoutl->addWidget(m_btnDown);
    layoutl->addItem(
                new QSpacerItem(10, 20, QSizePolicy::Minimum, QSizePolicy::Expanding));

    layout->addLayout(layoutl);
    enableDisableButtons();
}

void ListBuilder::connectWidgets() {
    connect(m_rightHandList, &QListWidget::itemSelectionChanged, this,
            &ListBuilder::enableDisableButtons);
    connect(m_leftHandList, &QListWidget::itemSelectionChanged, this,
            &ListBuilder::enableDisableButtons);
    connect(m_btnMoveOneToRight, &QPushButton::clicked,
            [=]() { m_rightHandList->addItem(m_leftHandList->takeItem(m_leftHandList->currentRow())); });

    connect(m_btnMoveOneToLeft, &QPushButton::clicked,
            [=]() { m_leftHandList->addItem(m_rightHandList->takeItem(m_rightHandList->currentRow())); });

    connect(m_btnMoveAllFromRightToLeft, &QPushButton::clicked, [=]() {
        while (m_rightHandList->count() > 0) {
            m_leftHandList->addItem(m_rightHandList->takeItem(0));
        }
    });

    connect(m_btnMoveAllFromLeftToRight, &QPushButton::clicked, [=]() {
        while (m_leftHandList->count() > 0) {
            m_rightHandList->addItem(m_leftHandList->takeItem(0));
        }
    });

    connect(m_btnUp, &QPushButton::clicked, [=]() {
        int row = m_rightHandList->currentRow();
        QListWidgetItem *currentItem = m_rightHandList->takeItem(row);
        m_rightHandList->insertItem(row - 1, currentItem);
        m_rightHandList->setCurrentRow(row - 1);
    });

    connect(m_btnDown, &QPushButton::clicked, [=]() {
        int row = m_rightHandList->currentRow();
        QListWidgetItem *currentItem = m_rightHandList->takeItem(row);
        m_rightHandList->insertItem(row + 1, currentItem);
        m_rightHandList->setCurrentRow(row + 1);
    });
}

void ListBuilder::enableDisableButtons() {
    m_btnUp->setDisabled(m_rightHandList->selectedItems().isEmpty() ||
                        m_rightHandList->currentRow() == 0);
    m_btnDown->setDisabled(m_rightHandList->selectedItems().isEmpty() ||
                          m_rightHandList->currentRow() == m_rightHandList->count() - 1);
    m_btnMoveOneToRight->setDisabled(m_leftHandList->selectedItems().isEmpty());
    m_btnMoveOneToLeft->setDisabled(m_rightHandList->selectedItems().isEmpty());
}
