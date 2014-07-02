#ifndef GLWIDGET_H
#define GLWIDGET_H
#include <QDebug>
#include "../PDC/showdatawindow.h"
QT_BEGIN_NAMESPACE
class QPaintEvent;
class QWidget;
QT_END_NAMESPACE
class qqwidget : public QWidget
{
    int numScalefft;
    double fft_total;
    double fftnowvalue;
    QColor ffthsl;
    QPen   fftPen;
    double FFTPEN;
    int counteffectedcolum;
    bool firstdraw; //default true

public:
    qqwidget(QWidget *parent);
    void setData(int numScalefftp,int colum);
    bool todraw;
    int countdraw;
    int shifteindex ;
    double effected_by_ko_co_fft[720][NUMPATHFFT/2];  // to save the result fft data effected by ko co for drawFFT Function
    float increase_log[NUMPATHFFT/2+1]; //decide the gap between every points(x-axis)in LOG scale & as FFT_equalization
protected:
    void paintEvent(QPaintEvent *event);
    void closeEvent(QCloseEvent *);
};

#endif // GLWIDGET_H
