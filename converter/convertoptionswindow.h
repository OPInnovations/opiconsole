#ifndef CONVERTOPTIONSWINDOW_H
#define CONVERTOPTIONSWINDOW_H

#include <QtGui>
#include "signalviewer.h"


//Sigma
#define  Sigma_Default  11.5  //FC
#define  Sigma_MAX     40 //prevent user input garbage

//alpha
#define  alpha_Default  9.0   //FC
#define  alpha_MAX     40  //prevent user input garbage

//EEG conver 0.5 accuracy
#define  EEGCONVERT05ACCURACY 100
//define the index of eeg in dataTypeValCB
#define  EEGINDEX 1
#define  ECGINDEX 2
#define TMPAVGFACTOR 8  // number of samples to average for temperature


namespace Ui {
class ConvertOptionsWindow;
}

class ConvertOptionsWindow : public QDialog
{
    Q_OBJECT
    int pdnSlotWant;
    signalviewer *signalviewerpointer;
public:
    //Generic
    explicit ConvertOptionsWindow(QString *outDirp, quint8 *pdnListp, qint8 *outDataTypep, float *alphap, float *sigmap,
                                  QVector<qint64> **tsQVpp, QVector<quint8> **skpQVpp,
                                  QVector<quint8> **batQVpp, QVector<qint16> **adcQVpp,
                                  QVector<qint16> **tmpQVpp, QVector<qint16> **axQVpp,
                                  QVector<qint16> **ayQVpp, QVector<qint16> **azQVpp,
                                  QVector<qint16> **sqQVpp, QVector<quint8> **edQVpp,
                                  QVector<qint64> *annOnsetTSQVp,
                                  QVector<QString> *annTextQVp,
                                  qint8 convTypeDes, bool *useProfileFlagp,
                                  QWidget *parent = 0, bool *doRRfile = 0);

    //EEG
    explicit ConvertOptionsWindow(QString *outDirp, float *alphap, float *sigmap,quint8 *pdnListp, qint8 *outDataTypep,
                                  QVector<qint64> **tsQVpp, QVector<quint8> **skpQVpp,
                                  QVector<quint8> **batQVpp, QVector<quint8> **edQVpp,
                                  QVector<qint16> **M2QVpp, QVector<qint16> **M1QVpp,
                                  QVector<qint16> **G2QVpp, QVector<qint16> **G1QVpp,
                                  QVector<qint16> **UPQVpp, QVector<qint16> **BetaQVpp,
                                  QVector<qint16> **SigmaQVpp, QVector<qint16> **alphaQVpp,
                                  QVector<qint16> **thetaQVpp, QVector<qint16> **deltaQVpp,
                                  QVector<qint64> *annOnsetTSQVp,QVector<QString> *annTextQVp,
                                  qint8 convTypeDes, bool *useProfileFlagp, QWidget *parent = 0);

    //ECG  LF=SDNN*10; HFper=log HFpower; LHRatio=log LHRatio
    explicit ConvertOptionsWindow(QString *outDirp, quint8 *pdnListp,float *alphap, float *sigmap, qint8 *outDataTypep,
                                  QVector<qint64> **tsQVpp, QVector<quint8> **skpQVpp,
                                  QVector<quint8> **batQVpp, QVector<quint8> **edQVpp,
                                  QVector<qint16> **RRQVpp, QVector<qint16> **ampQVpp,
                                  QVector<qint16> **LFperQVpp, QVector<qint16> **HFperQVpp,
                                  QVector<qint16> **LHRatioQVpp,
                                  QVector<qint64> *annOnsetTSQVp,QVector<QString> *annTextQVp,
                                  qint8 convTypeDes, bool *useProfileFlagp, QWidget *parent = 0);

    void sleepanalysis_uiconfig();
    ~ConvertOptionsWindow();
    
private slots:
    void on_buttonBox_accepted();

    void on_dataTypeValCB_currentIndexChanged(int index);

    void on_endDTE_dateTimeChanged(const QDateTime &date);

    void on_stDTE_dateTimeChanged(const QDateTime &date);

    void on_ViewercomboBox_currentIndexChanged(int index);

    void on_tagFileBrwsPB_clicked();

    void on_viewPB_released();

    void on_editProfilePB_clicked();

    void on_outDirBrwsPB_clicked();

    void on_checkBox_RRfile_toggled(bool checked);

private:
    Ui::ConvertOptionsWindow *ui;
    qint64 myfirstFrmTS, mylastFrmTS;
    qint8 *myoutDataTypep;
    float *mysigmap,*myalphap;
    quint8 *mypdnListp;
    bool *doRRfilep;
    QVector<qint64> **mytsQVpp;
    QVector<quint8> **myskpQVpp;
    QVector<quint8> **mybatQVpp;
     QVector<quint8> **myedQVpp;

    QVector<qint16> **myadcQVpp;
    QVector<qint16> **myaxQVpp;
    QVector<qint16> **myayQVpp;
    QVector<qint16> **myazQVpp;
    QVector<qint16> **mytmpQVpp;
    QVector<qint16> **mysqQVpp;

    QVector<qint16> **myM2QVpp;
    QVector<qint16> **myM1QVpp;
    QVector<qint16> **myG2QVpp;
    QVector<qint16> **myG1QVpp;
    QVector<qint16> **myUPQVpp;
    QVector<qint16> **myBetaQVpp;
    QVector<qint16> **mySigmaQVpp;
    QVector<qint16> **myalphaQVpp;
    QVector<qint16> **mythetaQVpp;
    QVector<qint16> **mydeltaQVpp;

    QVector<qint16> **myRRQVpp;
    QVector<qint16> **myampQVpp;
    QVector<qint16> **myLFperQVpp; //SDNN
    QVector<qint16> **myHFperQVpp; //HFpower
    QVector<qint16> **myLHRatioQVpp; //log

    QVector<qint64> *myannOnsetTSQVp;
    QVector<QString> *myannTextQVp;
    bool tagProcessed;
    qint8 convType;
    bool *myuseProfileFlagp;
    QString *myoutDirp;

    void hide_eeg_userinput();
    void show_eeg_userinput();
    void getfirstlastfrmTS(QVector<qint64> **tsQVpp, qint64 *firstFrmTSp, qint64 *lastFrmTSp);
    void avgtmpQVs(QVector<qint16> **tmpQVpp);
};

#endif // CONVERTOPTIONSWINDOW_H
