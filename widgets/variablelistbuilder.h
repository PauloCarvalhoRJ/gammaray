#ifndef VARIABLELISTBUILDER_H
#define VARIABLELISTBUILDER_H

#include <QWidget>

class DataFile;
class Attribute;

namespace Ui {
class VariableListBuilder;
}

/** This widget allows the user to build an ordered list of attributes of a data set. */
class VariableListBuilder : public QWidget
{
    Q_OBJECT

public:
    explicit VariableListBuilder(QWidget *parent = nullptr);
    ~VariableListBuilder();

    /** Sets an optional caption text. */
    void setCaption( QString caption );

    /** Sets an optional background color for the caption text.
     * Text color is automatically set to either black or white depending on how dark or light
     * is the background color according to the criterion defined in Util::isDark().
     */
    void setCaptionBGColor( const QColor& color );

    /**
     *  Returns a vector containing the pointers to the attributes selected by the user.
     */
    std::vector<Attribute*> getSelectedAttributes(){ return m_attributeList; }

public Q_SLOTS:

    void onOpenBuildList();

    /** Updates the list of variables from the passed data file. */
    void onListVariables( DataFile* file );

private:
    Ui::VariableListBuilder *ui;
    DataFile *m_dataFile;
    std::vector<Attribute*> m_attributeList;
};

#endif // VARIABLELISTBUILDER_H
