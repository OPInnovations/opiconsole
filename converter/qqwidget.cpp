#include "qqwidget.h"
#include <QtGui>

qqwidget::qqwidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(722,226); //hardwired?
    setAutoFillBackground(true);
    todraw = false;
    int i=0;
    for( i=0; i<=NUMPATHFFT/2; i++)
    {
       increase_log[i]=10*log10((i+1));//10db/dec equalization && draw segment length
    }
    countdraw = 0;
    firstdraw = true;
    setAttribute(Qt::WA_OpaquePaintEvent,false);
}

void qqwidget::setData(int numScalefftp,int colum){
    numScalefft = numScalefftp;
    fft_total  = increase_log[numScalefft]-increase_log[1]; //start from 2=1Hz
    FFTPEN = 1;
    counteffectedcolum = colum;
    fftPen.setWidthF(1);
    todraw = false;
    countdraw = 0;
}


void qqwidget::paintEvent(QPaintEvent *event){

    if((todraw&&(countdraw==0))){
 QTime nowtime = QTime::currentTime();
    countdraw++;
    int j,i;
    float begin,button,tempy;
    float increase;
    double rr,gg,bb;//decide the color in drawFFT Function
    begin=this->x();
    button=this->y()+this->height();
    tempy=button;
    //decide the gap between every points(y-axis)
    if(numScalefft>NUMPATHFFT/2)  //display to max of 256Hz
        numScalefft=NUMPATHFFT/2;
    increase = (float)this->height();
    increase=increase/(fft_total);  //now normalized to display section

    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.save();
    for(i=shifteindex;i<counteffectedcolum;i++){
      tempy=button;
      for( j=2;j<=numScalefft;j++){
            fftnowvalue=effected_by_ko_co_fft[i][j];
            //clamp
            if (fftnowvalue>FFTSIGNALMAX)  fftnowvalue=FFTSIGNALMAX;
            if (fftnowvalue<FFTSIGNALMIN)  fftnowvalue=FFTSIGNALMIN;
            //convert to pseudo-color
            //draw
            gg=long (fftnowvalue);
            if(gg<32){
                rr=255;
                bb=32+gg*3;
            }else if (gg<224){
                rr=298-gg*4/3;
                bb=128;
            }else{
                rr=0;
                bb=gg*4-765;
            }
            ffthsl.setHsl(rr,255,bb);   //for using HSL model
            fftPen.setColor(ffthsl);
            painter.setPen(fftPen);
            painter.setBrush(QBrush(ffthsl));
            painter.drawRect(QRectF(begin+(i-shifteindex)*FFTPEN,
                                    button-(increase_log[j]-increase_log[1])*increase,
                                    FFTPEN,
                                    tempy-(button-(increase_log[j]-increase_log[1])*increase)));
            tempy=button-(increase_log[j]-increase_log[1])*increase;
        }//for(int j=2;j<=numScalefft;j++) end
    }
    painter.restore();
    painter.end();
    if(firstdraw)
    firstdraw = false;
    todraw = false;
    setAttribute(Qt::WA_OpaquePaintEvent,true);
  QTime newtime = QTime::currentTime();
  qDebug()<< counteffectedcolum <<nowtime.toString("hhmmsszzz")<<newtime.toString("hhmmsszzz");
    }
}


void qqwidget::closeEvent(QCloseEvent *){
    this->deleteLater();
}
