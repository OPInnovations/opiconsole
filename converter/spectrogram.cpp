#include "spectrogram.h"
#include "ui_spectrogram.h"

Spectrogram::Spectrogram(quint8 pdn,QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Spectrogram)
{
    ui->setupUi(this);
    //set screen size
    this->setMinimumSize(this->width(),this->height());
    this->setMaximumSize(this->width(),this->height());
    this->setWindowTitle(QString("Spectrogram Viewer [%1]").arg((int)pdn));
}

Spectrogram::~Spectrogram()
{
    delete ui;
}

void Spectrogram::setData(QVector<qint16> *adcqvectp ,QVector<qint64> *mytsp,
                          qint64 firstts, qint64 lastts)
{
    reset();
    adcqvectQVpp = adcqvectp;
    mytspQVpp = mytsp;
    firsttime = firstts;
    lasttime = lastts;
    drawfftroutine();
}

void Spectrogram::reset()
{
     int i=0;
     qqscenep = new qqwidget(this);
     ui->gridLayout->addWidget(qqscenep,0,0);
     fft_is_ready=false;
     countfftcolumn = 0; 
     numpathfftcol = 720; //???manually here???
     //Setting a fixed size for the transformation
     transformer.setSize(NUMPATHFFT);
     //the screen is not full,so dont clean
     fft_clean=false;
     //setup FFT increase_log[i] table here for Equalization and draw length
     for( i=0; i<=NUMPATHFFT/2; i++){
         qqscenep->increase_log[i]=10*log10((i+1));//10db/dec equalization && draw segment length
     }
     fftk=8;  // set initial gain multiplier 8=32db 6=43db dynamic range
     fftco=0;   // (-)fftco value 2 => -70-2=-72 db for 1024FFT (-70db built-in)
     ui->fftkfactor->setText(QString::number(fftk));
     ui->fftcofactor->setText(QString::number(fftco));
     numScalefft=NUMPATHFFT/4; //256 bins, half of 256Hz BW
     ui->FFT_TimeSlider->setValue(numScalefft);
}

void Spectrogram::on_FFT_TimeSlider_valueChanged(int value)
{
    numScalefft=ui->FFT_TimeSlider->value();
    QString temp;
    temp.setNum(value*512/NUMPATHFFT).append("Hz");
    ui->label_3->setText(temp);
}

void Spectrogram::on_fftbutton_clicked()
{
    fftk =  ui->fftkfactor->toPlainText().toDouble();
    fftco=  ui->fftcofactor->toPlainText().toDouble();
    fft_is_ready=false;
    countfftcolumn = 0;   
    drawfftroutine();
}

void Spectrogram::drawfftroutine(){
    long countfft = 0, countadc=0 ;
    double ffttemp;
    int i=0,j=0;
    float skipfft=1;
    long counteffected = 0, ifirst=0;
    //to save the data for drawing
    float saveFFT[NUMPATHFFT];//to save raw data,uncaculated to fft,just adc
    float calculatedFFT[NUMPATHFFT]; //to save the data after FFT calculation
    QVector <QComplexFloat> fftresult; // to save the result fft data uneffected by ko co
    for(j=0; j<mytspQVpp->size(); j++)
    {
        if(mytspQVpp->at(j)>=firsttime) break; //j*64=i firsttime
    }
    ifirst = j*64;
    for(i=ifirst; i<adcqvectQVpp->size(); i++)
    {
       if((mytspQVpp->at(i/64)>lasttime)) break;
//       else if((mytspQVpp->at(i/64)>=firsttime))
        countadc++;
    }
    skipfft=(countadc*2/NUMPATHFFT);
    skipfft=(skipfft-1)/(numpathfftcol); //skip factor
    if(skipfft<=1) skipfft=1;
    //fft
    //from here
    i=0;
    while (i<countadc)  //for full FFT
    {
        if(countfft>=0&&countfft<NUMPATHFFT){
            saveFFT[countfft]=adcqvectQVpp->at(i+ifirst); //firsttime shift
            countfft++;
            i++;
        }else{ //full sample
            if(countfftcolumn>=0&&countfftcolumn<numpathfftcol)
            {
                countfftcolumn++;
                for(j=0;j<NUMPATHFFT;j++) //add Hanning window
                    saveFFT[j]=0.5*(1-qCos((2*M_PI*j)/(NUMPATHFFT-1)))*saveFFT[j]; //add Hanning window
                transformer.forwardTransform(saveFFT,calculatedFFT);
                fftresult=transformer.toComplex(calculatedFFT);
                //effected by fftk and fftco
                for(j=0;j<NUMPATHFFT/2;j++)
                {
                    re=(double)fftresult[j].real()*(double)fftresult[j].real();
                    im=(double)fftresult[j].imaginary()*(double)fftresult[j].imaginary();
                    ffttemp=re+im;
                    if(ffttemp<=0)  //can't log10(0)
                        qqscenep->effected_by_ko_co_fft[counteffected][j]= 0;//in db
                    else
                        qqscenep->effected_by_ko_co_fft[counteffected][j]=fftk*((10*log10(ffttemp))-70-fftco+qqscenep->increase_log[j]);//in db + equalization
                    //equalization 10db/dec added
                }//for(int j=0;j<NUMPATHFFTROW/2;J++) end
                counteffected++;
                if(skipfft>1) //with FFT skip
                {
                    countfft=0;
                    i += (skipfft-2.0) * (NUMPATHFFT/2); //skip forward
                    if(i>=countadc) break;
                }
                else // no skip
                {
                    countfft=NUMPATHFFT/2; //halfway
                    for(j=0;j<countfft;j++)
                        saveFFT[j]=saveFFT[countfft+j];
                }
                saveFFT[countfft]=adcqvectQVpp->at(i+ifirst); //1st new data point
                countfft++;
                i++;
            }
            else{ //screen full
                break;
            }//else end
        }//if(countfft>=0&&countfft<NUMPATHFFT) end
    }//while end
    qqscenep->setAttribute(Qt::WA_OpaquePaintEvent,false);
    qqscenep->shifteindex = 0;
    qqscenep->setData(numScalefft,counteffected);
    qqscenep->todraw = true;
    qqscenep->countdraw = 0;
    qqscenep->repaint();
}
