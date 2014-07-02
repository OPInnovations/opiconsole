#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H

#include <QMainWindow>
#include <QDebug>
#include <QGraphicsScene>
#include "../FFT/qfouriertransformer.h"
#include "../FFT/qcomplexnumber.h"
#include <QPainterPath>
#include <QGraphicsRectItem>
#include "qqwidget.h"
#include <QGridLayout>

namespace Ui {
class Spectrogram;
}

class Spectrogram : public QMainWindow
{
    Q_OBJECT
    //to save the parameter for fft
    float fftk,fftco; //initial fftk=8 (-)fftco=0 (-70 built-in => -70db)
    QColor ffthsl;
    QPen   fftPen;
    bool   fft_is_ready; //initial false , if true means that the fft row is ready to calculate
    double re,im; //real and imiage
    //the following paramater are used in darwfft Function
    double fftnowvalue;
    double fftscreenmiddle,fftsignalmiddle;
    bool   fft_clean; //if the screen is full then clean it,false means donot clean //initial false,
    int numpathfftcol;//define the how many collumns in fft screen
    QFourierTransformer transformer;  //should Setting a fixed size for the transformation
    int numScalefft; //decide the number of bins you want to show ,initial
    //to save the value form the FFTGainSlider bar
    float fft_total; //total display width in LOG scale
    int countfftcolumn;
    double FFTPEN;
    QVector<qint16> *adcqvectQVpp;
    QVector<qint64> *mytspQVpp;
    qint64 firsttime,lasttime;
    qqwidget *qqscenep ;

public:
    explicit Spectrogram(quint8 pdn,QWidget *parent = 0);
    ~Spectrogram();
    void setData(QVector<qint16> *adcqvectp ,QVector<qint64> *mytsp,
                 qint64 firstts, qint64 lastts);
    void reset();
private slots:
    void on_FFT_TimeSlider_valueChanged(int value);

    void on_fftbutton_clicked();
private:
    Ui::Spectrogram *ui;
    void drawfftroutine();
};

#endif // SPECTROGRAM_H
