#ifndef READMMDIALOG_H
#define READMMDIALOG_H

#include <QtGui>


namespace Ui {
class ReadMMDialog;
}

class ReadMMDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ReadMMDialog(QString *outfileNamep, qint32 progBarMin,
                          qint32 progBarMax, QString *tagfileNamep,
                          bool *useProfileFlagp, QWidget *parent = 0);
    ~ReadMMDialog();
    void setProgBarValue(qint32 value);
    void setStatusText(QString statstr);
    void setTagStatText(QString statstr);
    void endState();
    bool abortedFlag;
    QString *mytagfileNamep;
    bool *myuseProfileFlagp;
    QString *myoutFileNamep;

private slots:
    void on_buttonBox_accepted();

    void on_editProfilePB_clicked();

    void on_tagFileBrwsPB_clicked();

    void on_outDirBrwsPB_clicked();

private:
    Ui::ReadMMDialog *ui;
};

#endif // READMMDIALOG_H
