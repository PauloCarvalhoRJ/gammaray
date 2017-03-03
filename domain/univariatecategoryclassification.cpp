#include "univariatecategoryclassification.h"

#include "widgets/intervalandcategorywidget.h"
#include "categorydefinition.h"
#include "application.h"
#include "project.h"
#include "objectgroup.h"

UnivariateCategoryClassification::UnivariateCategoryClassification(CategoryDefinition *cd, QString path) :
    DoubleDoubeIntTriplets ( path ),
    m_categoryDefinition( cd )
{
}

UnivariateCategoryClassification::UnivariateCategoryClassification(const QString categoryDefinitionName, QString path) :
    DoubleDoubeIntTriplets ( path ),
    m_categoryDefinition( nullptr ),
    m_categoryDefinitionNameForDelayedLoad( categoryDefinitionName )
{
}

UnivariateCategoryClassification::~UnivariateCategoryClassification()
{

}

int UnivariateCategoryClassification::getCategory(double value)
{
    uint tot = getTripletCount();
    for(uint i = 0; i < tot; ++i)
        if( value >= get1stValue( i ) && value <= get2ndValue( i ) )
            return get3rdValue( i );
    return -1;
}

void UnivariateCategoryClassification::save(QTextStream *txt_stream)
{
    QString usedCategoryDefinitionName;
    if( m_categoryDefinition )
        usedCategoryDefinitionName = m_categoryDefinition->getName();
    else
        usedCategoryDefinitionName = m_categoryDefinitionNameForDelayedLoad;

    if( usedCategoryDefinitionName.isEmpty() )
        Application::instance()->logError(
                    "ERROR: UnivariateCategoryClassification::save(): blank CategoryDefinition name.");

    (*txt_stream) << this->getFileType() << ":" <<
                     this->getFileName() << ',' << usedCategoryDefinitionName << '\n';
}

QWidget *UnivariateCategoryClassification::createContentElementWidget()
{
    if( ! setCategoryDefinition() )
        return new QWidget();
    return new IntervalAndCategoryWidget( m_categoryDefinition );
}

QWidget *UnivariateCategoryClassification::createWidgetFilledWithContentElement(uint iContent)
{
    if( ! setCategoryDefinition() )
        return new QWidget();

    IntervalAndCategoryWidget *widget = new IntervalAndCategoryWidget( m_categoryDefinition );

    widget->setIntervalLow( get1stValue( iContent ) );
    widget->setIntervalHigh( get2ndValue( iContent ) );
    widget->setCategoryCode( get3rdValue( iContent ) );

    return widget;
}

void UnivariateCategoryClassification::addContentElementFromWidget(QWidget *w)
{
    //surely the widget is a IntervalAndCategoryWidget.
    IntervalAndCategoryWidget *widget = (IntervalAndCategoryWidget*)w;

    //add the triplet of the values read.
    addTriplet( widget->getIntervalLow(),
                widget->getIntervalHigh(),
                widget->getCategoryCode());
}

bool UnivariateCategoryClassification::setCategoryDefinition()
{
    if( ! m_categoryDefinition ){
        m_categoryDefinition = (CategoryDefinition*)Application::instance()->
                getProject()->
                getResourcesGroup()->
                getChildByName( m_categoryDefinitionNameForDelayedLoad );
        if( ! m_categoryDefinition ){
            Application::instance()->logError(
                        "ERROR: UnivariateCategoryClassification::setCategoryDefinition(): CategoryDefinition "
                        + m_categoryDefinitionNameForDelayedLoad + " not found in the project." );
            return false;
        } else
            return true;
    } else
        return true;
}
