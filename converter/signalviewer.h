#ifndef FILEVIEW_H
#define FILEVIEW_H

#include <QtGui>
#include "../opi_win.h"
#include "../opi_helper.h"
#include "twodaccelviewer.h"
#include "mixsignaldialog.h"
#include "spectrogram.h"
#define STARTLINETEXT  "START"
#define ENDLINETEXT    "END"
#define ADCSKIPDRAWTHRESHOLD 20  //
#define ECGEEGSKIPDRAWTHRESHOLD 0.1 //
#define EEG_DRAW_SKIP_SEC 600
#define ECG_DRAW_SKIP_SEC 600
#define GENERIC_DRAW_SKIP_SEC 30  //30

#define FWMAX_SCALE 20    //the max scale rate for adc you want
#define FWMIN_SCALE 1/30  //the min scale rate for adc you want
#define FWOTHER_MAX_SCALE 20
#define FWOTHER_MIN_SCALE 1/30

#define MOUSEINDEXACCURACYRATIO 0.011
#define MOUSEMIDDLEACCURACYRATIO 1.5
#define MOUSEINDEXMOVETHRESHOLE 1
#define MOUSEMIDDLEMOVETHRESHOLE 8
//define the signal max for drawing
#define FWADCDIGITALMAX  800
#define FWACCDIGITALMAX  32767
#define FWTMPDIGITALMAX  1250
#define FWTMPDIGITALMIN  1050
#define FWEEGDIGITALMAX  10000
#define FWEEGDIGITALMIN  -2000
#define FWECGDIGITALMAX  15000
#define FWECGDIGITALMIN  000
#define FWMAXSECONDINONESCENE 86400 //24Hr
//#define FWMAXSECONDINONESCENE 3600 //1Hr
#define FWECGMAXSECONDINONESCENE 86400
#define FWEEGMAXSECONDINONESCENE 86400
//generic
#define FWSAMPLERATEADC 512
#define FWSAMPLERATEACCX 8
#define FWSAMPLERATEACCY 8
#define FWSAMPLERATEACCZ 32
#define FWSAMPLERATETEMP 8
#define FWSAMPLERATETAG 8

//ecg
#define FWSAMPLERATERR 32
#define FWSAMPLERATEAMP 32
#define FWSAMPLERATELFPER 32
#define FWSAMPLERATEHFPER 32
#define FWSAMPLERATELHRATIO 32

//eeg
#define FWSAMPLERATEG1 8
#define FWSAMPLERATEG2 8
#define FWSAMPLERATEM1 8
#define FWSAMPLERATEM2 8
#define FWSAMPLERATEUP 8
#define FWSAMPLERATEBETA 8
#define FWSAMPLERATESIGMA 8
#define FWSAMPLERATEALPHA 8
#define FWSAMPLERATETHETA 8
#define FWSAMPLERATEDELTA 8

#define EDFDTOWAV 1  //can't edit tags since won't be written out
#define EDFDTOEDF 2  //can edit tags since will be written out
#define OPITOEDF 3   //can edit tags since will be written out
#define OPITOWAV 4   //can't edit tags since won't be written out
#define EDFMECGTOEDF 5 //can edit tags since will be written out
#define EDFMECGTOWAV 6 //can't edit tags since won't be written out
#define EDFMEEGTOEDF 7 //can edit tags since will be written out
#define EDFMEEGTOWAV 8 //can't edit tags since won't be written out

#define M2INDEX 0
#define M1INDEX 1
#define G2INDEX 2
#define G1INDEX 3
#define UPINDEX 4
#define BETAINDEX 5
#define SIGMAINDEX 6
#define ALPHAINDEX 7
#define THETAINDEX 8
#define DELTAINDEX 9
#define MIXINDEX  10

#define ECGSAMPLERATE 32


namespace Ui {
class signalviewer;
}

class signalviewer : public QDialog
{
    Q_OBJECT
    twoDaccelviewer *TDV;
    Spectrogram *SpecFFT;
    bool  tagflag;
    float finalshowvaluex;
    int TSSTEP ;
    int SHOWTAGTHRESHOLD;
    QVector <qint16> adcqvect;
    QVector <qint16> accxqvect;
    QVector <qint16> accyqvect;
    QVector <qint16> acczqvect;
    QVector <qint16> tempqvect;

    QVector <qint16> M2qvect;
    QVector <qint16> M1qvect;
    QVector <qint16> G2qvect;
    QVector <qint16> G1qvect;
    QVector <qint16> UPqvect;
    QVector <qint16> Betaqvect;
    QVector <qint16> Sigmaqvect;
    QVector <qint16> alphaqvect;
    QVector <qint16> thetaqvect;
    QVector <qint16> deltaqvect;
    QVector <qint16> MixSignal;

    QVector <qint64> mytsQV;
    QVector <qint64> *TSqvect;
    QVector <QString> *Textqvect;
    QVector <int> taglocation;
    QVector <QGraphicsLineItem*> linebuffer;
    QVector <QGraphicsTextItem*> stringbuffer;

    //Scene
    QGraphicsScene *sceneSignalA,*sceneSignalB,*sceneSignalC,*sceneSignalD,*sceneSignalE,*sceneSignalF;
    //control by x_axis
    int oldSignalAindex;  //initial 0=M2
    int total_num_of_points;//the total number of points set+ by the .wav
    int show_num_of_seconds;//the points showed on the scene
    int signal_index_begin;
    //save the gainvalue
    float adcgainvalue;
    float othergainvalue;
    float zoom_out_step; //initialized as the value that every gap you have to take for every step
    float zoom_in_step;  //initialized as the value that every gap you have to take for every step
    float other_zoom_in_step;
    float other_zoom_out_step;
    //save the increase pixels to move the scene
    int mouseincreasex,mouseincreasey;  //initial 0
    //save the mouse pos
    int pressx,pressy,releasex,releasey;  //initial 0
    int getmouseflag;//initial-1; //A=1 F=6
    int changablemiddleA,changablemiddleB,changablemiddleC,changablemiddleD,changablemiddleE; //initial screen middle
    //set file start and end
    qint64 *myfirstFrmTS,*mylastFrmTS;
    qint64  middlevaluefortime;
    qint64 originalfirst,originallast;
    int    tagplusindex;
    QVector <int> highlightitemindex;
    QVector <int> tagindexbuffer;
    qint8 Filetype;
    quint8 mypdn;
public:
    bool  reversechecked;//initial false
    //generic
    explicit signalviewer(QVector<qint16> *adcqvectp, QVector<qint16> *accxqvectp,
                          QVector<qint16> *accyqvectp, QVector<qint16> *acczqvectp,
                          QVector<qint16> *tempqvectp,QVector <qint64> *mytsPointer,
                          qint64 *myfirstFrmTSp,qint64 *mylastFrmTSp,
                          QVector<qint64> *Tsqvectp,QVector<QString> *Textqvectp,quint8 pdntitle,qint8 type,
                          QWidget *parent = 0);

    //ecg
    explicit signalviewer(int dummy,float dummy2,QVector<qint16> *adcqvectp, QVector<qint16> *accxqvectp,
                          QVector<qint16> *accyqvectp, QVector<qint16> *acczqvectp,
                          QVector<qint16> *tempqvectp,QVector <qint64> *mytsPointer,
                          qint64 *myfirstFrmTSp,qint64 *mylastFrmTSp,
                          QVector<qint64> *Tsqvectp,QVector<QString> *Textqvectp,quint8 pdntitle,qint8 type,
                          QWidget *parent = 0);
    //eeg
    explicit signalviewer(float dummy,QVector<qint16> *M2qvectp, QVector<qint16> *M1qvectp,
                          QVector<qint16> *G2qvectp, QVector<qint16> *G1qvectp,
                          QVector<qint16> *UPqvectp,
                          QVector<qint16> *Betaqvectp, QVector<qint16> *Sigmaqvectp,
                          QVector<qint16> *alphaqvectp,
                          QVector<qint16> *thetaqvectp, QVector<qint16> *deltaqvectp,
                          QVector <qint64> *mytsPointer,
                          qint64 *myfirstFrmTSp,qint64 *mylastFrmTSp,
                          QVector<qint64> *Tsqvectp,QVector<QString> *Textqvectp,quint8 pdntitle,qint8 type,
                          QWidget *parent = 0);

    void tagcontrol(bool con);
    ~signalviewer();

private slots:
    void on_ShiftSlider_valueChanged(int value);

    void on_pushButton_4_clicked();

    void on_StartTimeButtion_clicked();

    void on_EndTimeButton_clicked();

    void on_pushButton_TP_clicked();

    void on_pushButton_TM_clicked();

    void on_TagText_textChanged();

    void on_TagText_2_textChanged();

    void on_comboBoxSignalA_currentIndexChanged(int index);

    void on_comboBoxSignalB_currentIndexChanged(int index);

    void on_comboBoxSignalC_currentIndexChanged(int index);

    void on_comboBoxSignalD_currentIndexChanged(int index);

    void on_comboBoxSignalE_currentIndexChanged(int index);

    void on_comboBoxTAG_activated(int index);

    void on_TimeSlider_valueChanged(int value);

    void on_adczoom_actionTriggered(int action);

    void on_otherzoom_actionTriggered(int action);

    void on_checkReverse_toggled(bool checked);

    void on_TimeSlider_sliderReleased();

    void on_adczoom_sliderReleased();

    void on_otherzoom_sliderReleased();

    void on_pushButton_2D_accel_clicked();

    void on_comboBoxSignalA_activated(int index);

    void on_pushButton_spectrogram_clicked();

private:
    Ui::signalviewer *ui;
    void setUpTotalScene();
    void reset();
    void switchdraw(QGraphicsView *view,QGraphicsScene *scene,float gainvalue,int index_begin,int *changeablemiddle,int samplerate,int index);
    void draw(QGraphicsView *view,QGraphicsScene *scene,int max,int min,QVector <qint16> *database,float gainvalue,int index_begin,int *changablemiddle,int samplerate);
    void checktimeandline(QGraphicsView *view,int index_begin,int samplerate);
    void showTag(QGraphicsView *view,QGraphicsScene *scene,QVector <qint64> *database,QVector <QString> *textbase,int index_begin,int samplerate);
    //0 show generic
    //1 show ecg
    //2 show eeg
    void showlabelcontrol(int choose);

    void updateTag(QVector<QString> *Textqvectp);


protected:
    void closeEvent(QCloseEvent *);
    void showEvent(QShowEvent *);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *r);
    void mouseMoveEvent(QMouseEvent *m);
};

#endif // FILEVIEW_H
