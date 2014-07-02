#ifndef TUTORIALGAME_H
#define TUTORIALGAME_H

#include <QtGui>
#include "../converter/twodaccelviewer.h"


#define TG_NUMPOINTSAMPLE_X 1
#define TG_NUMPOINTSAMPLE_Y 1
#define TG_NUMPOINTSAMPLE_Z 4
#define TG_NUMPOINTSAMPLE_TEMP 1
#define TG_MAX_TEMP 4500
#define TG_FFTSIGNALMIN 1     //the min value of the signal in db
#define TG_FFTSIGNALMAX 254   //the max value of the signal in db

//define how to calculate fft

//define the background gap
#define MG_GAP 50
#define GB_GAP 80
#define BA_GAP 70
#define BALL_DIAMETER 40


//define the threshold
#define Z_MOVE_GAIN 8
#define X_MOVE_GAIN 8

#define TG_DM_THRESHOLD 3000
#define TG_FIRE_THRESHOLD 7000
#define TG_CALMDOWN_THRESHOLD 8 //in db 8*327.67=2600 difficult if too low

//define the hole diameter
#define MHOLE_DIAMETER 55
#define GHOLE_DIAMETER 55
#define BHOLE_DIAMETER 55
#define AHOLE_DIAMETER 55

//define the level
#define LEVEL_M 0
#define LEVEL_G 1
#define LEVEL_B 2
#define LEVEL_A 3
#define LEVEL_DONE -1
namespace Ui {
class tutorialgame;
}

class tutorialgame : public QMainWindow
{
    Q_OBJECT
    twoDaccelviewer *TDV;
    QWidget *bioparent;
    qint16 RRnewvalue,RRoldvalue;
    qint16 peaknewvalue,peakoldvalue;
    qint16 M2newvalue,M2oldvalue;
    qint16 M1newvalue,M1oldvalue;
    qint16 G2newvalue,G2oldvalue;
    qint16 G1newvalue,G1oldvalue;
    qint16 UPnewvalue,UPoldvalue;
    qint16 Betanewvalue,Betaoldvalue;
    qint16 Sigmanewvalue,Sigmaoldvalue;
    qint16 alphanewvalue,alphaoldvalue;
    qint16 thetanewvalue,thetaoldvalue;
    qint16 deltanewvalue,deltaoldvalue;
    float  x_calm,y_calm,z_calm; //center straight position
    float  x_new_value,y_new_value,z_new_value;
    float  x_old_value,y_old_value,z_old_value;   //initial 0
    float  dx,dy,dz,xyzpowermagnitude,xyzpowersum;
    float  ballx,bally,ballxmin,ballxmax,ballymin,ballymax;
    bool   FFTflag;
    //for background draw color
    double rr,gg,bb;
    QPen   fftPen;
    QColor ffthsl;
    double fftnowvalue;
    QGraphicsEllipseItem *GB,*BB,*AB;//*BALL;
    //QGraphicsEllipseItem *MHoleOne;
    //QGraphicsEllipseItem *MHoleTwo;//,*MHole3,*MHole4;
    QGraphicsRectItem *MB;
    //set scene
    QGraphicsScene *sceneMain;

    bool calmdown;  //initial false
    bool checked; //initial false, need to know the direction,//not yet program
    int holex0,holex1,holex2,holex3;
    int holey0,holey1,holey2,holey3;
    int HOLE_DIAMETER;
    int LEVEL; //initial 0
    bool gamedone; //initial false
public:
    explicit tutorialgame(QWidget *parent = 0);
    ~tutorialgame();
    void setpdnwindowtitle(int pdn);
    void setdata(unsigned char EDM,
                 qint16 RR,qint16 peak,qint16 M2,
                 qint16 M1,qint16 G2,
                 qint16 G1,qint16 UP,
                 qint16 Beta,qint16 Sigma,
                 qint16 alpha,qint16 theta,
                 qint16 delta,float x_new,
                 float y_new,float z_new,
                 bool FFTflag);
    void reset();
    void setpostureacc(QVector <qint16>*accxQVp,
                       QVector <qint16>*accyQVp,QVector <qint16>*acczQVp);
private slots:
    void on_pushButton_Start_clicked();

    void on_HIT_clicked();

    void on_pushButton_2D_accel_clicked();

private:
    Ui::tutorialgame *ui;
    float floatToAbs(float num);
    void  processData();
    void  setbackgroundfftcolor();
    void  dumpdata();
    QGraphicsEllipseItem *  checkBallIn(QGraphicsEllipseItem *Hole,int hx,int hy,int holediameter,int bx,int by,int balldiameter);
    QGraphicsEllipseItem *  checkBallInGroup();
    void  moveball();
    void  gamestart();
    void  beforecalm();
    bool  checknextlevel();
    void  fireit();
    void  changelevel(int choose);
protected:
    void closeEvent(QCloseEvent *);
};

#endif // TUTORIALGAME_H
