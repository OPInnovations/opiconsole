#ifndef TWODACCELVIEWER_H
#define TWODACCELVIEWER_H

#include <QtGui>
#include "../opi_win.h"
#include "../opi_helper.h"


#define TDV_NUMPOINTSAMPLE_Z 4
#define PVZSCALE 640 // +/-1.25G PostureViewer scaling of z axis
#define LIVEPVSHOWSECMAX 1200 //PostureViewer shows only 20 minutes worth in live mode

namespace Ui {
class twoDaccelviewer;
}

class twoDaccelviewer : public QMainWindow
{
    Q_OBJECT
    qint32 newstep, azold, axold, ayold, az4old, ax2old, ay2old, ax3old, ay3old, ax4old, ay4old, az8old, az12old, az16old;
    long lowcount,medcount,highcount,intensecount,totalcount,stepcount;
    QGraphicsScene *Mainscene, *zScene;
    QVector <qint16> accxqvect;
    QVector <qint16> accyqvect;
    QVector <qint16> acczqvect;
    QVector <qint16> acczqvectoriginal;
    QVector <qint16> activityqvect;
    QVector <long> activitylow;
    QVector <long> activitymed;
    QVector <long> activityhigh;
    QVector <long> activityintense;
    QVector <long> totalcountsave;
    QVector <long> stepcountsave;
    QGraphicsTextItem *start,*end;
    QGraphicsTextItem *metaAnalysisText;
    QGraphicsTextItem *activityText;
    QVector <qint64> mytsQV;
    qint64 myfirstFrmTS,mylastFrmTS;
    QDateTime firstFrmDT, lastFrmDT;
    float posx,posy,x_calm,y_calm,z_calm,oldposx,oldposy;
    bool firstdraw; //initial true
    qint16 accxmax,accxmin,accymax,accymin,acczmax,acczmin;
    float windowxscale,windowyscale;
    int timerid,timerindexcount;
    bool livedisplaymodebool;
    quint8 mypdn;
public:
    explicit twoDaccelviewer(bool livedisplaymode, quint8 pdn, QWidget *parent = 0);
    ~twoDaccelviewer();
    void setData(QVector <qint16> * accxQVp,QVector <qint16> * accyQVp,
                 QVector <qint16> * acczQVp,QVector <qint64> * mytsp,
                 qint64  firstts,qint64  lastts);
    void showMetaAnalysisText(QString *sleepQS);
    void appendaccxyz(QVector <qint16> * accxQVp,QVector <qint16> * accyQVp,
                      QVector <qint16> * acczQVp,QVector <qint16> * newaccxQVp,QVector <qint16> * newaccyQVp,
                      QVector <qint16> * newacczQVp);
    void activitydistribu(double act,long *lowcountp,long *medcountp,long *highcountp,long *intensecountp,long *totalcountp,long *stepcountp);
    void livedisplayroutine(QVector <qint16> * accxQVp,QVector <qint16> * accyQVp,
                            QVector <qint16> * acczQVp);
    void MMfastdisplayroutine(QVector <qint16> * accxQVp,QVector <qint16> * accyQVp,
                              QVector <qint16> * acczQVp,int datarecord);
    void reset();
    void livedisplaylayout();
    void setPDN(quint8 pdn);
private:
    Ui::twoDaccelviewer *ui;

    void calnewpos(float *posxp,float *posyp,qint16 newposx,qint16 newposy,qint16 newposz);
    void draw(float newposx,float newposy,float *oldposxp,float *oldposyp);
    void routinedraw();
    void showActivityText();
    double actscale(qint16 act);
protected:
    void closeEvent(QCloseEvent *);
    void showEvent(QShowEvent *);
    void timerEvent(QTimerEvent *);
private slots:
    void on_pushButton_trace_clicked();
    void on_genActTextPB_clicked();
};

#endif // TWODACCELVIEWER_H
