#ifndef TRANSIOGRAMDIALOG_H
#define TRANSIOGRAMDIALOG_H

#include <QDialog>

namespace Ui {
class TransiogramDialog;
}

class Attribute;

class TransiogramDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransiogramDialog( QWidget *parent = nullptr );
    ~TransiogramDialog();

    void dragEnterEvent(QDragEnterEvent *e);

    void dragMoveEvent(QDragMoveEvent *e);

    void dropEvent(QDropEvent *e);

private:
    Ui::TransiogramDialog *ui;

    std::vector<Attribute*> m_categoricalAttributes;

    void tryToAddAttribute( Attribute* attribute );


private Q_SLOTS:
    void onResetAttributesList();
    void performCalculation();
};

#endif // TRANSIOGRAMDIALOG_H
