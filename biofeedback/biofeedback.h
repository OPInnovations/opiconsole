#ifndef BIOFEEDBACK_H
#define BIOFEEDBACK_H

#include <QtGui>
#include "../FFT/qfouriertransformer.h"
#include "../FFT/qcomplexnumber.h"
#include "opi_win.h"
#include "../converter/convertwindow.h"
#include "../converter/convertoptionswindow.h"
#include "../TutorialGame/tutorialgame.h"
#include "../converter/twodaccelviewer.h"


#define BF_MAIN_MAX_SCALE 10
#define BF_MAIN_MIN_SCALE 1/10
#define BF_OTHER_MAX_SCALE 10
#define BF_OTHER_MIN_SCALE 1/10

#define BFNUMPOINTSAMPLE  64 //it has to be a even number 4 8 16 32 64
#define BFSAMPLERATE_HR   512//adc sample rate

#define BFTOTALSECOND_HR 1.25 //1" initial search + update every 1 second

//for drawing RR
#define BFRRMAX 12000 //1200msec
#define BFRRMIN 4000 //400msec
#define BFNUMPATHRR_PEAK 9600 //20min max 480*20???
#define TIMEMORESTEP 480   //480*0.125 = 60s
#define BFTOLERATEVALUE  10

//for drawing peak
#define BFPEAKMAX  10000 //60% rail
#define BFPEAKMIN 0
#define BFPEAKTHRESHOLD 200
//drawing RR end

//for eeg max and min DISPLAY: 1000=3dB 10000=30dB
#define BFG1MAX  10000  //30dB range
#define BFG1MIN  0
#define BFG2MAX  10000
#define BFG2MIN  0
#define BFM1MAX  10000  //30dB range
#define BFM1MIN  0
#define BFM2MAX  10000
#define BFM2MIN  0
#define BFUPMAX  13000  //40dB range
#define BFUPMIN  1000
#define BFBETAMAX  11000 //30dB range
#define BFBETAMIN   1000
#define BFSIGMAMAX  10000
#define BFSIGMAMIN  0
#define BFALPHAMAX  10000
#define BFALPHAMIN  0
#define BFTHETAMAX  11000
#define BFTHETAMIN  1000
#define BFDELTAMAX  13000
#define BFDELTAMIN  3000

//for ecg max and min
#define BFLFMAX  32767
#define BFLFMIN  0
#define BFHFMAX  32767
#define BFHFMIN  0
#define BFLHMAX  32767
#define BFLHMIN  0

//checksignal
#define STOP 0
#define RR  1
#define PEAK 2
#define M2    3
#define M1    4
#define G2    5
#define G1    6
#define UP    7
#define BETA  8
#define SIGMA 9
#define ALPHA 10
#define THETA 11
#define DELTA 12


namespace Ui {
class biofeedback;
}

class biofeedback : public QMainWindow
{
    Q_OBJECT
    tutorialgame *tg;
    twoDaccelviewer *TDV;
    //count the numbers of  pdn showing window
    qint32 *pdnshowcount;
    //struct of opipkt
    OPIPKT_t PACKAGE_t;
    //Save the gamemode data
    float temp_last_data,temp_new_data;
    float x_new_data,y_new_data,z_new_data[TG_NUMPOINTSAMPLE_Z],z_new_data_average;

    //save the new data start
    QVector <qint16> adc_new_data; //sample the max and min form 0~8 8~16 16~24 24~32 32~40 40~48 48~56 56~63 and even number of index means min otherwise means max
    QVector <qint16> adc_fft_new_data;
    int adc_data_index_count; //to count the adc index ,reset 0
    int adc_fft_data_index_count;
    //save the new data end
    //Scene
    QGraphicsScene *sceneSignalA,*sceneSignalB,*sceneSignalC,*sceneSignalD,*sceneSignalE;

    QVector<qint16> eegM2QV;
    QVector<qint16> eegM1QV;
    QVector<qint16> eegG2QV;
    QVector<qint16> eegG1QV;
    QVector<qint16> eegUPQV;
    QVector<qint16> eegBetaQV;
    QVector<qint16> eegSigmaQV;
    QVector<qint16> eegalphaQV;
    QVector<qint16> eegthetaQV;
    QVector<qint16> eegdeltaQV;

    QVector<bool> missBeatFlagQV;
    int    tempRRcount,tempampcount;
    int    tempRRsum,tempampsum;
    int    tempLFcount,tempHFcount;
    int    tempLFsum,tempHFsum;
    int    tempLHRatiocount;
    int    tempLHRatiosum;
    QVector<qint16> ecgRRQV;
    QVector<qint16> ecgampQV;

    float Sigma,alpha;

    //for drawing RR
    QVector <qint32>  adcECGindQV;
    int countlineRR;
    bool   RRreset;  //initial true
    bool   FFTFLAG;  //new FFT data
    bool   RRmaxormin;
    int    RRPavg,RRDavg;
    int    RRDlast;
    int    difsizeandoldpeak;  //initial 16
    qint16 RRvaluenew; //initial 8000
    qint16 RRvalueold; //initial 8000
    float increaseRR; //decide the gap between every points(x-axis)
    float scalerateRR; //decide the scale of the signal(y-axis),scalerate=(scene'height)/(signal range you want to show)
    int middleRR; //calculate where to put the x-axis(the middle of the scene)
    int signalmiddleRR; //design what is the middle value of your input siganl
    int beginRR; //decide where to begin to draw
    //drawing RR end

    //for drawing peak
    qint16 ampvaluenew,ampvalueold;
    int countlinepeak;
    float increasepeak; //decide the gap between every points(x-axis)
    float scaleratepeak; //decide the scale of the signal(y-axis),scalerate=(scene'height)/(signal range you want to show)
    int middlepeak; //calculate where to put the x-axis(the middle of the scene)
    int signalmiddlepeak; //design what is the middle value of your input siganl
    int beginpeak; //decide where to begin to draw
    //drawing peak end

    //for drawing G1
    qint16 G1oldvalue;
    int countlineG1;

    //for drawing G2
    qint16 G2oldvalue;
    int countlineG2;
    //drawing G2 end
    //for drawing M1
    qint16 M1oldvalue;
    int countlineM1;
    //drawing M1 end

    //for drawing M2
    qint16 M2oldvalue;
    int countlineM2;


    //for drawing UP
    qint16 UPoldvalue;
    int countlineUP;

    //for drawing Beta
    qint16 Betaoldvalue;
    int countlineBeta;

    //for drawing Sigma
    qint16 Sigmaoldvalue;
    int countlineSigma;

    //for drawing alpha
    qint16 alphaoldvalue;
    int countlinealpha;

    //for drawing theta
    qint16 thetaoldvalue;
    int countlinetheta;

    //for drawing delta
    qint16 deltaoldvalue;
    int countlinedelta;

    //for main zoom
    float zoom_in_step,zoom_out_step,zoomrate,zoomctrlvalue;
    float other_zoom_in_step,other_zoom_out_step,other_zoomrate,other_zoomctrlvalue;
    float fftco; //display offset in dB
    bool gamemode, eegmode, ecgmode;
public:
    explicit biofeedback(qint32 *pdnshowcountp, QWidget *parent = 0);
    ~biofeedback();
    //decide the struct is new or not
    bool newdata;
    //check the window is open or hide
    bool already_open;
    int PDN_NUMBER;
    int getStruct(OPIPKT_t* opipointer);
    void setPdnNum(int num);
    int routinedrawgroup(void);
    void reset(bool setgamemode);
    void showgamewindow(int pdnnumber);
    void secretstart(bool setgamemode);
private:
    Ui::biofeedback *ui;
    void showDataControl(void);
    int calNewPack(void);
    void setUpTotalScene();

    int drawRR(bool validHRmax,QGraphicsView *view,QGraphicsScene *scene,int max,int min,
               int *countline,int num_of_points,
               QLabel * label, qint16 *showvalue,qint16 RRnowvalue,float gain);
    int drawpeak(bool validHRmax,QGraphicsView *view,QGraphicsScene *scene,int max,int min,
                 int *countline,int num_of_points,
                 QLabel * label, qint16 *showvalue,qint16 peaknowvalue,float gain);

    int draweeg(bool validHRmax,QGraphicsView *view,QGraphicsScene *scene,int max,int min,
                int *countline,int num_of_points,
                QLabel * label, qint16 *showvalue,qint16 eegnowvalue,float gain);

    int drawecg(bool validHRmax,QGraphicsView *view,QGraphicsScene *scene,int max,int min,
                int *countline,int num_of_points,
                QLabel * label, qint16 *showvalue,qint16 ecgnowvalue,float gain);
    qint16 fftDoubleQInt16Conversion(double fftAmpSq);

    void LivetoMeegQVs(QVector<qint16> *adcqvectQvp , qint32 numDataRecs ,
                       QVector<qint16> *G1Qvp ,QVector<qint16> *G2Qvp ,
                       QVector<qint16> *M1Qvp ,QVector<qint16> *M2Qvp ,
                       QVector<qint16> *UPQvp , QVector<qint16> *BetaQvp ,
                       QVector<qint16> *SigmaQvp , QVector<qint16> *alphaQvp ,
                       QVector<qint16> *thetaQvp , QVector<qint16> *deltaQvp);

protected:
    void closeEvent(QCloseEvent *);
    void showEvent(QShowEvent *);
private slots:
    void on_checkSignalA_currentIndexChanged(int index);
    void on_checkSignalB_currentIndexChanged(int index);
    void on_pushButton_5_clicked();
    void on_checkSignalC_currentIndexChanged(int index);
    void on_checkSignalD_currentIndexChanged(int index);
    void on_checkSignalE_currentIndexChanged(int index);
    void on_timeSlider_valueChanged(int value);
    void on_zoomctrl_valueChanged(int value);
    void on_other_zoomctrl_valueChanged(int value);
    void on_fftbutton_clicked();
    void on_pushButton_2D_accel_clicked();
};

#endif // BIOFEEDBACK_H
