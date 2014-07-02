#ifndef CONVERTWINDOW_H
#define CONVERTWINDOW_H

#include <QtGui>
#include <QVector>
#include "../FFT/qfouriertransformer.h"
#include "../FFT/qcomplexnumber.h"
#include "signalviewer.h"


//following paramrters will be used in findHRmax
#define  PAVGRATIO  0.5  //Pavg= PAVGRATIO*Pavg + (1-PAVGRATIO)*Pnew
#define  DAVGRATIO  0.7  //Davg= DAVGRATIO*Davg + (1-DAVGRATIO)*Dnew
#define  IDAVG 0.7  //in seconds for 85bpm x2.1=40bpm minimum in findHRmax
#define  GAPSKIP  16    //the skip points after invalid peak
#define  INCASERATIO 1.3  //larger the Davg to insure find the max value 1/1.25~1.25
#define  PAVGCHANGE 0.5  //Pavg change range allowed 0.5 ~ 1/0.5 wide is better
#define  DAVGCHANGE 0.7   //DAVG change range allowed 0.75 ~ 1/0.75
#define  PAVGMAX  18000  //max PAVG allowed
#define  PAVGMIN  500    //min PAVG allowed
#define  MINHRSEC    0.3  //defind the interval seconds MINIMUM 0.3sec=>200bpm
#define  PTOPCHECK   0.45  //check top flatness: smaller than 0.45/2=>22.5% change

#define  GENERICMDATATYPE 0  // what kind of data type is requested
#define  EEGMDATATYPE     1  // what kind of metadata type is requested
#define  ECGMDATATYPE      2  // what kind of data type is requested

#define  FFTMAX  100 //define the max value of eeg for edf header //the same as .wav,max and min
#define  FFTMIN -100 //define the min value of eeg for edf header //the same as .wav,max and min

#define  FFTSIZE 1024 //define the fft size
#define  FFTOVERLAP 512 //define the fft overlap
//M2
#define  M2A 156.5
#define  M2B 173.5  //A TO B
#define  M2C 186.5
#define  M2D 191.5  //C TO D
#define  M2E 208.5
#define  M2F 231.5  //E TO F
#define  M2G 248.5
#define  M2H 255.5  //G TO H
//M1
#define  M1A 104.5
#define  M1B 115.5  //A TO B
#define  M1C 124.5
#define  M1D 143.5  //C TO D
//G2
#define  G2A 62.5
#define  G2B 95.5  //A TO B
//G1
#define  G1A 40
#define  G1B 47.5  //A TO B
#define  G1C 52.5
#define  G1D 57.5  //C TO D
//UP
#define  UPA 48
#define  UPB 52  //A TO B
#define  UPC 58
#define  UPD 62  //C TO D
#define  UPE 96
#define  UPF 104 //E TO F
#define  UPG 116
#define  UPH 124 //G TO H
#define  UPI 144
#define  UPJ 156  //A TO B
#define  UPK 174
#define  UPL 186  //C TO D
#define  UPM 192
#define  UPN 208 //E TO F
#define  UPO 232
#define  UPP 248 //G TO H
//Beta
#define  BetaA  16 //must be above Sigma
#define  BetaB  39.5 //A TO B

//theta
#define  thetaA  4
#define  thetaB  6.5  //A TO B FC7
//delta
#define  deltaA  1.0
#define  deltaB  3.5  //A TO B

//define the sideband for Alpha,Sigma
#define alphaGap 1.0
#define SigmaGap 1.0

// for FFT conversion
#define FFTDBSCALE  327.68 // 100dB = 32768
#define FFTDBOFFSET 0.0 //(-)offset 0db => -70db default (-70db built-in)

//save the file date to qvector
#define CONVERTADCCHANNEL 0
#define CONVERTACCXCHANNEL 1
#define CONVERTACCYCHANNEL 2
#define CONVERTACCZCHANNEL 3
#define CONVERTTEMPCHANNEL 4

//FOR ECG
#define ECGFFTOVERLAP 512 //16sec
#define ECGFFTSIZE    4096  //128sec
#define LFA      0.040 //LF low bin=5
#define LFB      0.146 //LF high bin=18
#define HFA      0.150 //HF low bin=19
#define HFB      0.400 //HF high bin=51
#define LFHFTOINDEXRATIO 0.00781 //ensure integer+ bins 0.00390625*2

namespace Ui {
class ConvertWindow;
}

class ConvertWindow : public QMainWindow
{
    Q_OBJECT

    //save the max value of HR
    //QVector <long long> HRmaxindex;
    float   Sigma,alpha;
    //temp
    QString DtoMecgQVsoutFileName;
public:
    explicit ConvertWindow(QWidget *parent = 0);
    ~ConvertWindow();
    qint32 whatInType(QString filename);
    qint32 edfhdrread(QDataStream *instrp, QString *lpidp, QString *lridp,
                      QDateTime *startDTp, qint32 *numDataRecsp, qint32 *dataRecDurp,
                      qint32 *numSignalsp, QVector<QString> *labelSignalsQVp,
                      QVector<QString> *transTypeQVp, QVector<QString> *physDimQVp,
                      QVector<qint32> *physMinQVp, QVector<qint32> *physMaxQVp,
                      QVector<qint32> *digMinQVp, QVector<qint32> *digMaxQVp,
                      QVector<QString> *prefiltQVp, QVector<qint32> *sampsPerDRQVp);
    qint32 edfDread(QDataStream *instrp, QDateTime startDT,
                    qint32 numDataRecs, qint32 dataRecDur,
                    qint32 numSignals, QVector<qint32> sampsPerDRQV,
                    QVector<qint64> *tsQVp, QVector<quint8> *skpQVp,
                    QVector<quint8> *batQVp, QVector<qint16> *adcQVp,
                    QVector<qint16> *tmpQVp, QVector<qint16> *axQVp,
                    QVector<qint16> *ayQVp, QVector<qint16> *azQVp,
                    QVector<qint16> *sqQVp, QVector<quint8> *edQVp,
                    QVector<qint64> *annOnsetTSQVp, QVector<QString> *annTextQVp);
    qint32 edfMeegread(QDataStream *instrp, QDateTime startDT,
                       qint32 numDataRecs, qint32 dataRecDur,
                       qint32 numSignals, QVector<qint32> sampsPerDRQV,
                       QVector<qint16> *M2QVp, QVector<qint16> *M1QVp,
                       QVector<qint16> *G2QVp, QVector<qint16> *G1QVp,
                       QVector<qint16> *UPQVp, QVector<qint16> *BetaQVp,
                       QVector<qint16> *SigmaQVp, QVector<qint16> *alphaQVp,
                       QVector<qint16> *thetaQVp, QVector<qint16> *deltaQVp,
                       QVector<qint64> *annOnsetTSQVp, QVector<QString> *annTextQVp,
                       QVector<qint64> *tsQVp, QVector<quint8> *skpQVp,
                       QVector<quint8> *batQVp, QVector<quint8> *edQVp);

    void delQVs(QVector<qint64> **tsQVpp, QVector<quint8> **skpQVpp,
                QVector<quint8> **batQVpp, QVector<qint16> **adcQVpp,
                QVector<qint16> **tmpQVpp, QVector<qint16> **axQVpp,
                QVector<qint16> **ayQVpp, QVector<qint16> **azQVpp,
                QVector<qint16> **sqQVpp, QVector<quint8> **edQVpp);

    void delspecQVs(qint32 pdnSlot,
                    QVector<qint64> **tsQVpp, QVector<quint8> **skpQVpp,
                    QVector<quint8> **batQVpp, QVector<qint16> **adcQVpp,
                    QVector<qint16> **tmpQVpp, QVector<qint16> **axQVpp,
                    QVector<qint16> **ayQVpp, QVector<qint16> **azQVpp,
                    QVector<qint16> **sqQVpp, QVector<quint8> **edQVpp);
    //find the heart max
    //parameter data save the data
    //samplerate save the sample rate
    //totalsecond save the total seconds in the file
    //return when will happen max value(s)
    static QVector<qint32> findHRmax(QVector <qint16> *datap, int samplerate,
                               int totalsecond, QVector<bool> *missBeatFlagqvectp,
                              bool reset,bool extcall,int *Pavg,int *Davg,int *Dlast);

    void DtoMecgQVs(QVector<qint32> *adcECGindQVp, QVector<qint16> *adcQVp,
                    QVector<bool> *missBeatFlagQVp, QVector<qint16> *ecgRRQVp,
                    QVector<qint16> *ecgampQVp, qint32 decimateFactor);
    //1Hz
    void DtoMeegQVs(QVector<qint16> *adcqvectQvp , QVector<qint16> *M2Qvp ,
                    QVector<qint16> *M1Qvp ,QVector<qint16> *G2Qvp ,
                    QVector<qint16> *G1Qvp ,QVector<qint16> *UPQvp ,
                    QVector<qint16> *BetaQvp , QVector<qint16> *SigmaQvp ,
                    QVector<qint16> *alphaQvp , QVector<qint16> *thetaQvp ,
                    QVector<qint16> *deltaQvp);
private slots:
    void on_inBrwsPB_clicked();

    void on_cnvtPB_clicked();

private:
    Ui::ConvertWindow *ui;


    bool convertopitowav();
    bool convertedfDtowav();
    void wavhdrwrite(QDataStream *out, quint32 sampFreq, quint16 numOfChan,
                     quint32 dataLen, quint16 bitsPerSamp);

    void wavDadcwrite(QDataStream *outstrp, QVector<qint16> *adcQVp);

    void wavDallelsewrite(QDataStream *outstrp, QVector<qint16> *tmpQVp,
                          QVector<qint16> *axQVp, QVector<qint16> *ayQVp,
                          QVector<qint16> *azQVp, QVector<qint16> *sqQVp);

    void wavMecgwrite(QDataStream *outm1strp, QVector<qint16> *ecgRRQVp,
                      QVector<qint16> *ecgampQVp, qint32 decimateFactor);
    qint16 fftDoubleQInt16Conversion(double fftAmpSq);

    //1Hz
    void wavMeegwrite(QDataStream *outstrp,QVector<qint16> *M2Qvp ,QVector<qint16> *M1Qvp ,
                      QVector<qint16> *G2Qvp ,QVector<qint16> *G1Qvp ,
                      QVector<qint16> *UPQvp , QVector<qint16> *BetaQvp ,
                      QVector<qint16> *SigmaQvp , QVector<qint16> *alphaQvp ,
                      QVector<qint16> *thetaQvp , QVector<qint16> *deltaQvp,
                      qint32 numDataRecs);

    bool convertopitoedf();
    bool convertedfDtoedf();
    bool convertedfMeegtoedf();
    bool convertedfMeegtowav();
    bool convertedfMecgtoedf();
    bool convertedfMecgtowav();

    //1Hz
    bool edfMeeghdropiwrite(QDataStream *out, QString *lpidp, QString *lridp,
                            QDateTime *startDTp, qint32 numDataRecs);

    //1Hz
    void edfMeegwrite(QDataStream *outstrp,QVector<qint16> *M2Qvp ,QVector<qint16> *M1Qvp ,
                      QVector<qint16> *G2Qvp ,QVector<qint16> *G1Qvp ,
                      QVector<qint16> *UPQvp , QVector<qint16> *BetaQvp ,
                      QVector<qint16> *SigmaQvp , QVector<qint16> *alphaQvp ,
                      QVector<qint16> *thetaQvp , QVector<qint16> *deltaQvp,
                      qint32 numDataRecs, qint64 firstFrmTS, QVector<qint64> *tagTSQVp,
                      QVector<QString> *tagTextQVp);

    void edftoedfMecgwrite(QDataStream *outstrp,QVector<qint16> *RRQVp ,QVector<qint16> *ampQVp ,
                           QVector<qint16> *LFperQVp ,QVector<qint16> *HFperQVp ,
                           QVector<qint16> *LHRatioQVp ,
                           qint32 numDataRecs, qint64 firstFrmTS, QVector<qint64> *tagTSQVp,
                           QVector<QString> *tagTextQVp);

    void edftowavMecgwrite(QDataStream *outstrp,QVector<qint16> *RRQVp ,QVector<qint16> *ampQVp ,
                           QVector<qint16> *LFperQVp ,QVector<qint16> *HFperQVp ,
                           QVector<qint16> *LHRatioQVp ,
                           qint32 numDataRecs);

    void edfMecghdropiwrite(QDataStream *out, QString *lpid, QString *lrid,
                            QDateTime *startDT, qint32 numDataRecs);


    qint32 edfMecgwrite(QDataStream *outm1strp, QVector<qint16> *ecgRRQVp,
                        QVector<qint16> *ecgampQVp, qint32 decimateFactor,
                        int FiledataRecordCt, qint64 firstFrmTS, QVector<qint64> *tagTSQVp,
                        QVector<QString> *tagTextQVp);

    qint32 opiDread(QDataStream *instrp, quint8 pdnDes, QVector<qint64> *tsQVp,
                    QVector<quint8> *skpQVp, QVector<quint8> *batQVp,
                    QVector<qint16> *adcQVp, QVector<qint16> *tmpQVp,
                    QVector<qint16> *axQVp, QVector<qint16> *ayQVp,
                    QVector<qint16> *azQVp, QVector<qint16> *sqQVp,
                    QVector<quint8> *edQVp);

    qint32 edfMecgread(QDataStream *instrp, QDateTime startDT,
                       qint32 numDataRecs, qint32 dataRecDur,
                       qint32 numSignals, QVector<qint32> sampsPerDRQV,
                       QVector<qint16> *RRQVp, QVector<qint16> *ampQVp,
                       QVector<qint16> *LFperQVp, QVector<qint16> *HFperQVp,
                       QVector<qint16> *LHRatioQVp,
                       QVector<qint64> *annOnsetTSQVp, QVector<QString> *annTextQVp ,
                       QVector<qint64> *tsQVp, QVector<quint8> *skpQVp,
                       QVector<quint8> *batQVp, QVector<quint8> *edQVp);

    void writetoFile(QString filename,QVector<qint32> *adcECGindQVpp,int dividend);

};

#endif // CONVERTWINDOW_H
