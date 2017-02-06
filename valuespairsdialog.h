#ifndef VALUESPAIRSDIALOG_H
#define VALUESPAIRSDIALOG_H

#include <QDialog>
#include <QList>

namespace Ui {
class ValuesPairsDialog;
}

class File;


class ValuesPairsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ValuesPairsDialog( File* valuePairsFile, QWidget *parent = 0 );

    File* getValuePairsFile(){ return m_valuePairsFile; }

    ~ValuesPairsDialog();

private:
    void initDialog();
    Ui::ValuesPairsDialog *ui;
    QList<QWidget*> m_pairWidgets;
    File* m_valuePairsFile;

private slots:
    void onAddPair();
    void onRemovePair();
    void onSave();
};

#endif // VALUESPAIRSDIALOG_H
