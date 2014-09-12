#include "biofeedback.h"
#include "ui_biofeedback.h"

QPen bfredPen(Qt::red);
QPen bfwhitePen(Qt::white);
QBrush bfwhiteBrush(Qt::white);
QBrush bfredBrush(Qt::red);
QPen bfblackPen(Qt::black);

#define  BFGUIDELINECOLOR  bfblackPen  //define the color of guide line
#define  BFSIGNALCOLOR     bfredPen    //define the color of signal line


biofeedback::biofeedback(qint32 *pdnshowcountp,QWidget *parent) :
    ui(new Ui::biofeedback)
{   already_open=false;
    pdnshowcount = pdnshowcountp;
    ui->setupUi(this);
    TDV = NULL;
    //set screen size
    this->setMinimumSize(this->width(),this->height());
    this->setMaximumSize(this->width(),this->height());
    //set all scenes for graphics view
    setUpTotalScene();
}


void biofeedback::setUpTotalScene()
{
    //new all scenes for graphics view
    sceneSignalA = new QGraphicsScene(ui->drawSignalA->x(),ui->drawSignalA->y(),ui->drawSignalA->width(),ui->drawSignalA->height(),this);
    sceneSignalB = new QGraphicsScene(ui->drawSignalB->x(),ui->drawSignalB->y(),ui->drawSignalB->width(),ui->drawSignalB->height(),this);
    sceneSignalC = new QGraphicsScene(ui->drawSignalC->x(),ui->drawSignalC->y(),ui->drawSignalC->width(),ui->drawSignalC->height(),this);
    sceneSignalD = new QGraphicsScene(ui->drawSignalD->x(),ui->drawSignalD->y(),ui->drawSignalD->width(),ui->drawSignalD->height(),this);
    sceneSignalE = new QGraphicsScene(ui->drawSignalE->x(),ui->drawSignalE->y(),ui->drawSignalE->width(),ui->drawSignalE->height(),this);
    //set scene to graphicsScene
    ui->drawSignalA->setScene(sceneSignalA);
    ui->drawSignalB->setScene(sceneSignalB);
    ui->drawSignalC->setScene(sceneSignalC);
    ui->drawSignalD->setScene(sceneSignalD);
    ui->drawSignalE->setScene(sceneSignalE);
}


void biofeedback::closeEvent(QCloseEvent *)
{
    *pdnshowcount-=1;
    if(pdnshowcount<0)
        *pdnshowcount=0;
    if(already_open)
        already_open=false;
    if(TDV!=NULL)
    TDV->setVisible(false);
}


void biofeedback::showEvent(QShowEvent *)
{
    if(!already_open)
    {
        *pdnshowcount+=1;
        already_open=true;
    }
}


void biofeedback::secretstart(bool setgamemode)
{
    if(setgamemode)
    {
        if(!already_open)
        {
            *pdnshowcount+=1;
            already_open=true;
        }
    }
}


void biofeedback::showDataControl(void)
{
    //RF
    ui->EDM->setText(QString("%1").arg(PACKAGE_t.payload[144]));
}

// process the data in PACKAGE_t and fill the temp_new_data, x_new_data, y_new_data, z_new_data, adc_new_data
int biofeedback::calNewPack(void)
{
	OPIPKT_DC01_SDC01_t packet = buildDC01SDC01(PACKAGE_t);

	int i,k;
    QVector <qint16> accxQV,accyQV,acczQV;

    
	//temperature
    //from here to moving-8 x100
    float temp_last_data = temp_new_data * 0.875;
    temp_new_data = temp_last_data + 0.125 * packet.temperatureData; // moving8 x100
    //end temperature
    
	x_new_data = packet.accelerometerX;
	y_new_data = packet.accelerometerY;
	//z_new_data = packet.accelerometerZs;
	z_new_data_average = packet.accelerometerZ;

    accxQV.append(wirelessaccscale(packet.accelerometerX));
    accyQV.append(wirelessaccscale(packet.accelerometerY));    
    acczQV.append(wirelessaccscale(packet.accelerometerZs[0]));
	acczQV.append(wirelessaccscale(packet.accelerometerZs[1]));
	acczQV.append(wirelessaccscale(packet.accelerometerZs[2]));
	acczQV.append(wirelessaccscale(packet.accelerometerZs[3]));

	packet.accelerometerZ = wirelessaccscale(packet.accelerometerZ);

    TDV->livedisplayroutine(&accxQV,&accyQV,&acczQV);

    if(gamemode)
    {
        tg->setpostureacc(&accxQV,&accyQV,&acczQV);
    }
    //from here  //default NUMPOINTSAMPLE=64 for full FFT
    int max_min_pair;
    max_min_pair=ADCLEN/BFNUMPOINTSAMPLE;  //skip range
    for(i=0; i<BFNUMPOINTSAMPLE; i++)//skip range
    {    
		adc_new_data[adc_data_index_count]=(packet.adcValues[i])*(ui->CheckInvertShow->isChecked()?-1:1);
        adc_fft_new_data[adc_fft_data_index_count]=adc_new_data[adc_data_index_count];
        if(!gamemode && ecgmode)  adc_data_index_count++;  //ECG AND !gamemode
        if(gamemode || eegmode) adc_fft_data_index_count++; //EEG or gamemode

        if(adc_data_index_count>=BFSAMPLERATE_HR*BFTOTALSECOND_HR) //
        {
            missBeatFlagQV.clear();
            adcECGindQV.clear();
            adcECGindQV = ConvertWindow::findHRmax(&adc_new_data,TSRTCFREQ, adc_new_data.size()/TSRTCFREQ, &missBeatFlagQV,RRreset,false,&RRPavg,&RRDavg,&RRDlast);

            if(adcECGindQV.size() != 0)  //only one peak
            {
                RRreset=false; //for next beat
                int adcmoveindex=0;
                tempRRcount=adcECGindQV.at(0); //read peak count
                tempampsum=adc_new_data[tempRRcount]; //read peak ADC value
                if(((tempRRcount+difsizeandoldpeak)<=RRDavg*1.5)&&((tempRRcount+difsizeandoldpeak)>=RRDavg*0.75)) //set valid range
                {
                    RRvaluenew = (tempRRcount+difsizeandoldpeak)*19.53125; //in msec, one peak
                    ampvaluenew = tempampsum; //one peak
                }
                if(tempRRcount+16<adc_new_data.size())
                {
                    for( k=tempRRcount+16;k<adc_new_data.size();k++) //skip forward 16
                    {
                        adc_new_data[adcmoveindex]=adc_new_data[k];
                        adcmoveindex++;
                    }
                    difsizeandoldpeak=16;
                }
                else
                {
                    difsizeandoldpeak=adc_new_data.size()-tempRRcount;
                    adcmoveindex=0;
                }
                adc_data_index_count = adcmoveindex;
            }
            else  // size=0
            {
                adc_data_index_count=0;
                RRreset=true; //start 1st peak
                adc_new_data.clear();
                adc_new_data.resize(BFSAMPLERATE_HR*BFTOTALSECOND_HR);
            }
        }//if end

        if(adc_fft_data_index_count>=FFTSIZE) //EEG calculations
        {
            FFTFLAG=true; //new FFT data
            adc_fft_data_index_count=FFTOVERLAP;
            LivetoMeegQVs(&adc_fft_new_data,BFTOTALSECOND_HR,
                          &eegM2QV,&eegM1QV,
                          &eegG2QV,&eegG1QV,
                          &eegUPQV,&eegBetaQV,
                          &eegSigmaQV,&eegalphaQV,
                          &eegthetaQV,&eegdeltaQV);
            for( k=0; k < FFTOVERLAP;k++)
            {
                adc_fft_new_data[k]=adc_fft_new_data[k+FFTOVERLAP];
            }
        }
        else FFTFLAG=false; //no new FFT data
    }
    //end adc
    return 1;
}


void biofeedback::LivetoMeegQVs(QVector<qint16> *adcqvectQvp, qint32 numDataRecs,
                                QVector<qint16> *M2Qvp, QVector<qint16> *M1Qvp,
                                QVector<qint16> *G2Qvp, QVector<qint16> *G1Qvp,
                                QVector<qint16> *UPQvp, QVector<qint16> *BetaQvp,
                                QVector<qint16> *SigmaQvp, QVector<qint16> *alphaQvp,
                                QVector<qint16> *thetaQvp, QVector<qint16> *deltaQvp)
{
    float saveFFTstd[FFTSIZE];
    int countfftdata=0;
    long k,j;
    QFourierTransformer transformer;  //should Setting a fixed size for the transformation
    transformer.setSize(FFTSIZE);
    QVector <QComplexFloat> fftresult;
    QVector <float>  amplitudeFFT(FFTSIZE);  //save the final fft result
    double re,im;
    float calculatedFFTstd[FFTSIZE];
    qint16 tempfftdata;  //save the fft value
    float  tempsavetotalfft; //save the data
    countfftdata=0;
    for(k=0; k<FFTSIZE; k++)  //count every samples
    {
        if(k<adcqvectQvp->size())
        {
            saveFFTstd[countfftdata]=0.5*(1.0-qCos((2.0*M_PI*countfftdata)/(FFTSIZE-1)))*(((double)adcqvectQvp->at(k)));
        }
        else
        {
            saveFFTstd[countfftdata]=(0);
        }
        countfftdata++;
    }//count every samples end

    //start calculate
    //calculation for fft
    transformer.forwardTransform(saveFFTstd,calculatedFFTstd);
    fftresult=transformer.toComplex(calculatedFFTstd);
    for(j=0; j<=FFTSIZE/2; j++)
    {
        re=(double)fftresult[j].real()*(double)fftresult[j].real();
        im=(double)fftresult[j].imaginary()*(double)fftresult[j].imaginary();
        amplitudeFFT[j]=re+im;
    }//for(int j=0;j<=FFTSIZE/2;J++) end

    //M2 start
    tempsavetotalfft=0;
    for( k=M2A/0.5; k<=M2B/0.5; k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    for( k=M2C/0.5; k<=M2D/0.5; k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    for( k=M2E/0.5; k<=M2F/0.5; k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    for( k=M2G/0.5; k<=M2H/0.5; k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
    (*M2Qvp)[0]=(tempfftdata);
    //M2 END

    //M1 start
    tempsavetotalfft=0;
    for( k=M1A/0.5;k<=M1B/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    for( k=M1C/0.5;k<=M1D/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
    (*M1Qvp)[0]=(tempfftdata);
    //M1 END

    //G2 start
    tempsavetotalfft=0;
    for( k=G2A/0.5;k<=G2B/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
    (*G2Qvp)[0]=(tempfftdata);
    //G2 END

    //G1 start
    tempsavetotalfft=0;
    for( k=G1A/0.5;k<=G1B/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    for( k=G1C/0.5;k<=G1D/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
    (*G1Qvp)[0]=(tempfftdata);
    //G1 END

    //UP start
    tempsavetotalfft=0;
    for( k=UPA/0.5;k<=UPB/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    for( k=UPC/0.5;k<=UPD/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    for( k=UPE/0.5;k<=UPF/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    for( k=UPG/0.5;k<=UPH/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    for( k=UPI/0.5;k<=UPJ/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    for(k=UPK/0.5;k<=UPL/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    for(k=UPM/0.5;k<=UPN/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    for(k=UPO/0.5;k<=UPP/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
    (*UPQvp)[0]=(tempfftdata);
    //UP END

    //Beta start
    tempsavetotalfft=0;
    for( k=BetaA/0.5;k<=BetaB/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
    (*BetaQvp)[0]=(tempfftdata);
    //Beta END

    //Sigma start
    tempsavetotalfft=0;
    for( k=(Sigma-SigmaGap)/0.5;k<=(Sigma+SigmaGap)/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
    (*SigmaQvp)[0]=(tempfftdata);
    //Sigma END

    //alpha start
    tempsavetotalfft=0;
    for(k=(alpha-alphaGap)/0.5;k<=(alpha+alphaGap)/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
    (*alphaQvp)[0]=(tempfftdata);
    //alpha END

    //theta start
    tempsavetotalfft=0;
    for( k=thetaA/0.5;k<=thetaB/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
    (*thetaQvp)[0]=(tempfftdata);
    //theta END

    //delta start
    tempsavetotalfft=0;
    for( k=deltaA/0.5;k<=deltaB/0.5;k++)
    {
        tempsavetotalfft+=amplitudeFFT[k];
    }
    tempfftdata = fftDoubleQInt16Conversion(tempsavetotalfft);
    (*deltaQvp)[0]=(tempfftdata);

}


qint16 biofeedback::fftDoubleQInt16Conversion(double fftAmpSq)
{
    double tempdoub;
    qint16 retVal;

    tempdoub = FFTDBSCALE*(10.0*log10(fftAmpSq)-70-FFTDBOFFSET); //-72db default
    if(tempdoub > 32767) retVal = 32767;
    else if(tempdoub < 0) retVal = 0; //remove negative spikes
    else retVal = (qint16) tempdoub;
    return retVal;
}


int biofeedback::routinedrawgroup()
{
    if(newdata&&(!ui->CheckPauseShow->isChecked()))
    {
        calNewPack();
        //draw start
        if(!gamemode)   //BIOFEEDBACK routine
        {
            //check SignalA
            switch(ui->checkSignalA->currentIndex())
            {
            case RR:
            {
                ecgmode=true;
                eegmode=false;
                drawRR(true,ui->drawSignalA,sceneSignalA,BFRRMAX,BFRRMIN,&countlineRR,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalA,&RRvalueold,RRvaluenew,zoomrate);
                break;
            }
            case PEAK:
            {
                ecgmode=true;
                eegmode=false;
                drawpeak(true,ui->drawSignalA,sceneSignalA,BFPEAKMAX,BFPEAKMIN,&countlinepeak,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalA,&ampvalueold,ampvaluenew,zoomrate);
                break;
            }
            case G1:
            {
                eegmode=true;
                ecgmode=false;
                draweeg(true,ui->drawSignalA,sceneSignalA,BFG1MAX,BFG1MIN,&countlineG1,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalA,&G1oldvalue,eegG1QV.at(0),zoomrate);
                break;
            }
            case G2:
            {
                eegmode=true;
                ecgmode=false;
                draweeg(true,ui->drawSignalA,sceneSignalA,BFG2MAX,BFG2MIN,&countlineG2,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalA,&G2oldvalue,eegG2QV.at(0),zoomrate);
                break;
            }
            case M1:
            {
                eegmode=true;
                ecgmode=false;
                draweeg(true,ui->drawSignalA,sceneSignalA,BFM1MAX,BFM1MIN,&countlineM1,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalA,&M1oldvalue,eegM1QV.at(0),zoomrate);
                break;
            }
            case M2:
            {
                eegmode=true;
                ecgmode=false;
                draweeg(true,ui->drawSignalA,sceneSignalA,BFM2MAX,BFM2MIN,&countlineM2,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalA,&M2oldvalue,eegM2QV.at(0),zoomrate);
                break;
            }
            case UP:
            {
                eegmode=true;
                ecgmode=false;
                draweeg(true,ui->drawSignalA,sceneSignalA,BFUPMAX,BFUPMIN,&countlineUP,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalA,&UPoldvalue,eegUPQV.at(0),zoomrate);
                break;
            }
            case BETA:
            {
                eegmode=true;
                ecgmode=false;
                draweeg(true,ui->drawSignalA,sceneSignalA,BFBETAMAX,BFBETAMIN,&countlineBeta,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalA,&Betaoldvalue,eegBetaQV.at(0),zoomrate);
                break;
            }
            case SIGMA:
            {
                eegmode=true;
                ecgmode=false;
                draweeg(true,ui->drawSignalA,sceneSignalA,BFSIGMAMAX,BFSIGMAMIN,&countlineSigma,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalA,&Sigmaoldvalue,eegSigmaQV.at(0),zoomrate);
                break;
            }
            case ALPHA:
            {
                eegmode=true;
                ecgmode=false;
                draweeg(true,ui->drawSignalA,sceneSignalA,BFALPHAMAX,BFALPHAMIN,&countlinealpha,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalA,&alphaoldvalue,eegalphaQV.at(0),zoomrate);
                break;
            }
            case THETA:
            { eegmode=true;
                ecgmode=false;
                draweeg(true,ui->drawSignalA,sceneSignalA,BFTHETAMAX,BFTHETAMIN,&countlinetheta,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalA,&thetaoldvalue,eegthetaQV.at(0),zoomrate);
                break;
            }
            case DELTA:
            {
                eegmode=true;
                ecgmode=false;
                draweeg(true,ui->drawSignalA,sceneSignalA,BFDELTAMAX,BFDELTAMIN,&countlinedelta,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalA,&deltaoldvalue,eegdeltaQV.at(0),zoomrate);
                break;
            }
            }
            //check SignalB
            switch(ui->checkSignalB->currentIndex())
            {
            case RR:
                drawRR(true,ui->drawSignalB,sceneSignalB,BFRRMAX,BFRRMIN,&countlineRR,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalB,&RRvalueold,RRvaluenew,other_zoomrate);
                break;
            case PEAK:
                drawpeak(true,ui->drawSignalB,sceneSignalB,BFPEAKMAX,BFPEAKMIN,&countlinepeak,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalB,&ampvalueold,ampvaluenew,other_zoomrate);
                break;
            case G1:
                draweeg(true,ui->drawSignalB,sceneSignalB,BFG1MAX,BFG1MIN,&countlineG1,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalB,&G1oldvalue,eegG1QV.at(0),other_zoomrate);
                break;
            case G2:
                draweeg(true,ui->drawSignalB,sceneSignalB,BFG2MAX,BFG2MIN,&countlineG2,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalB,&G2oldvalue,eegG2QV.at(0),other_zoomrate);
                break;
            case M1:
                draweeg(true,ui->drawSignalB,sceneSignalB,BFM1MAX,BFM1MIN,&countlineM1,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalB,&M1oldvalue,eegM1QV.at(0),other_zoomrate);
                break;
            case M2:
                draweeg(true,ui->drawSignalB,sceneSignalB,BFM2MAX,BFM2MIN,&countlineM2,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalB,&M2oldvalue,eegM2QV.at(0),other_zoomrate);
                break;
            case UP:
                draweeg(true,ui->drawSignalB,sceneSignalB,BFUPMAX,BFUPMIN,&countlineUP,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalB,&UPoldvalue,eegUPQV.at(0),other_zoomrate);
                break;
            case BETA:
                draweeg(true,ui->drawSignalB,sceneSignalB,BFBETAMAX,BFBETAMIN,&countlineBeta,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalB,&Betaoldvalue,eegBetaQV.at(0),other_zoomrate);
                break;
            case SIGMA:
                draweeg(true,ui->drawSignalB,sceneSignalB,BFSIGMAMAX,BFSIGMAMIN,&countlineSigma,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalB,&Sigmaoldvalue,eegSigmaQV.at(0),other_zoomrate);
                break;
            case ALPHA:
                draweeg(true,ui->drawSignalB,sceneSignalB,BFALPHAMAX,BFALPHAMIN,&countlinealpha,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalB,&alphaoldvalue,eegalphaQV.at(0),other_zoomrate);
                break;
            case THETA:
                draweeg(true,ui->drawSignalB,sceneSignalB,BFTHETAMAX,BFTHETAMIN,&countlinetheta,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalB,&thetaoldvalue,eegthetaQV.at(0),other_zoomrate);
                break;
            case DELTA:
                draweeg(true,ui->drawSignalB,sceneSignalB,BFDELTAMAX,BFDELTAMIN,&countlinedelta,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalB,&deltaoldvalue,eegdeltaQV.at(0),other_zoomrate);
                break;
            }
            //check SignalC
            switch(ui->checkSignalC->currentIndex())
            {
            case G1:
                draweeg(true,ui->drawSignalC,sceneSignalC,BFG1MAX,BFG1MIN,&countlineG1,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalC,&G1oldvalue,eegG1QV.at(0),other_zoomrate);
                break;
            case G2:
                draweeg(true,ui->drawSignalC,sceneSignalC,BFG2MAX,BFG2MIN,&countlineG2,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalC,&G2oldvalue,eegG2QV.at(0),other_zoomrate);
                break;
            case M1:
                draweeg(true,ui->drawSignalC,sceneSignalC,BFM1MAX,BFM1MIN,&countlineM1,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalC,&M1oldvalue,eegM1QV.at(0),other_zoomrate);
                break;
            case M2:
                draweeg(true,ui->drawSignalC,sceneSignalC,BFM2MAX,BFM2MIN,&countlineM2,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalC,&M2oldvalue,eegM2QV.at(0),other_zoomrate);
                break;
            case UP:
                draweeg(true,ui->drawSignalC,sceneSignalC,BFUPMAX,BFUPMIN,&countlineUP,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalC,&UPoldvalue,eegUPQV.at(0),other_zoomrate);
                break;
            case BETA:
                draweeg(true,ui->drawSignalC,sceneSignalC,BFBETAMAX,BFBETAMIN,&countlineBeta,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalC,&Betaoldvalue,eegBetaQV.at(0),other_zoomrate);
                break;
            case SIGMA:
                draweeg(true,ui->drawSignalC,sceneSignalC,BFSIGMAMAX,BFSIGMAMIN,&countlineSigma,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalC,&Sigmaoldvalue,eegSigmaQV.at(0),other_zoomrate);
                break;
            case ALPHA:
                draweeg(true,ui->drawSignalC,sceneSignalC,BFALPHAMAX,BFALPHAMIN,&countlinealpha,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalC,&alphaoldvalue,eegalphaQV.at(0),other_zoomrate);
                break;
            case THETA:
                draweeg(true,ui->drawSignalC,sceneSignalC,BFTHETAMAX,BFTHETAMIN,&countlinetheta,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalC,&thetaoldvalue,eegthetaQV.at(0),other_zoomrate);
                break;
            case DELTA:
                draweeg(true,ui->drawSignalC,sceneSignalC,BFDELTAMAX,BFDELTAMIN,&countlinedelta,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalC,&deltaoldvalue,eegdeltaQV.at(0),other_zoomrate);
                break;
            }
            //check SignalD
            switch(ui->checkSignalD->currentIndex())
            {
            case G1:
                draweeg(true,ui->drawSignalD,sceneSignalD,BFG1MAX,BFG1MIN,&countlineG1,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalD,&G1oldvalue,eegG1QV.at(0),other_zoomrate);
                break;
            case G2:
                draweeg(true,ui->drawSignalD,sceneSignalD,BFG2MAX,BFG2MIN,&countlineG2,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalD,&G2oldvalue,eegG2QV.at(0),other_zoomrate);
                break;
            case M1:
                draweeg(true,ui->drawSignalD,sceneSignalD,BFM1MAX,BFM1MIN,&countlineM1,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalD,&M1oldvalue,eegM1QV.at(0),other_zoomrate);
                break;
            case M2:
                draweeg(true,ui->drawSignalD,sceneSignalD,BFM2MAX,BFM2MIN,&countlineM2,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalD,&M2oldvalue,eegM2QV.at(0),other_zoomrate);
                break;
            case UP:
                draweeg(true,ui->drawSignalD,sceneSignalD,BFUPMAX,BFUPMIN,&countlineUP,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalD,&UPoldvalue,eegUPQV.at(0),other_zoomrate);
                break;
            case BETA:
                draweeg(true,ui->drawSignalD,sceneSignalD,BFBETAMAX,BFBETAMIN,&countlineBeta,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalD,&Betaoldvalue,eegBetaQV.at(0),other_zoomrate);
                break;
            case SIGMA:
                draweeg(true,ui->drawSignalD,sceneSignalD,BFSIGMAMAX,BFSIGMAMIN,&countlineSigma,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalD,&Sigmaoldvalue,eegSigmaQV.at(0),other_zoomrate);
                break;
            case ALPHA:
                draweeg(true,ui->drawSignalD,sceneSignalD,BFALPHAMAX,BFALPHAMIN,&countlinealpha,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalD,&alphaoldvalue,eegalphaQV.at(0),other_zoomrate);
                break;
            case THETA:
                draweeg(true,ui->drawSignalD,sceneSignalD,BFTHETAMAX,BFTHETAMIN,&countlinetheta,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalD,&thetaoldvalue,eegthetaQV.at(0),other_zoomrate);
                break;
            case DELTA:
                draweeg(true,ui->drawSignalD,sceneSignalD,BFDELTAMAX,BFDELTAMIN,&countlinedelta,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalD,&deltaoldvalue,eegdeltaQV.at(0),other_zoomrate);
                break;
            }
            //check SignalE
            switch(ui->checkSignalE->currentIndex())
            {
            case G1:
                draweeg(true,ui->drawSignalE,sceneSignalE,BFG1MAX,BFG1MIN,&countlineG1,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalE,&G1oldvalue,eegG1QV.at(0),other_zoomrate);
                break;
            case G2:
                draweeg(true,ui->drawSignalE,sceneSignalE,BFG2MAX,BFG2MIN,&countlineG2,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalE,&G2oldvalue,eegG2QV.at(0),other_zoomrate);
                break;
            case M1:
                draweeg(true,ui->drawSignalE,sceneSignalE,BFM1MAX,BFM1MIN,&countlineM1,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalE,&M1oldvalue,eegM1QV.at(0),other_zoomrate);
                break;
            case M2:
                draweeg(true,ui->drawSignalE,sceneSignalE,BFM2MAX,BFM2MIN,&countlineM2,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalE,&M2oldvalue,eegM2QV.at(0),other_zoomrate);
                break;
            case UP:
                draweeg(true,ui->drawSignalE,sceneSignalE,BFUPMAX,BFUPMIN,&countlineUP,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalE,&UPoldvalue,eegUPQV.at(0),other_zoomrate);
                break;
            case BETA:
                draweeg(true,ui->drawSignalE,sceneSignalE,BFBETAMAX,BFBETAMIN,&countlineBeta,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalE,&Betaoldvalue,eegBetaQV.at(0),other_zoomrate);
                break;
            case SIGMA:
                draweeg(true,ui->drawSignalE,sceneSignalE,BFSIGMAMAX,BFSIGMAMIN,&countlineSigma,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalE,&Sigmaoldvalue,eegSigmaQV.at(0),other_zoomrate);
                break;
            case ALPHA:
                draweeg(true,ui->drawSignalE,sceneSignalE,BFALPHAMAX,BFALPHAMIN,&countlinealpha,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalE,&alphaoldvalue,eegalphaQV.at(0),other_zoomrate);
                break;
            case THETA:
                draweeg(true,ui->drawSignalE,sceneSignalE,BFTHETAMAX,BFTHETAMIN,&countlinetheta,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalE,&thetaoldvalue,eegthetaQV.at(0),other_zoomrate);
                break;
            case DELTA:
                draweeg(true,ui->drawSignalE,sceneSignalE,BFDELTAMAX,BFDELTAMIN,&countlinedelta,ui->timeSlider->value()*TIMEMORESTEP,ui->label_SignalE,&deltaoldvalue,eegdeltaQV.at(0),other_zoomrate);
                break;}
            showDataControl();
        } //end of BIOFEEDBACK routine
        else   //GAMEMODE routine
        {
            tg->setdata(PACKAGE_t.payload[144],RRvaluenew,ampvaluenew,
                        eegM2QV.at(0),eegM1QV.at(0),
                        eegG2QV.at(0),eegG1QV.at(0),
                        eegUPQV.at(0),eegBetaQV.at(0),
                        eegSigmaQV.at(0),eegalphaQV.at(0),
                        eegthetaQV.at(0),eegdeltaQV.at(0),
                        x_new_data, y_new_data, z_new_data_average, FFTFLAG);
        } //end of GAMEMODE routine
        newdata=false;
        return 1;
    }
    return 0;
}


int biofeedback::drawpeak(bool validHRmax, QGraphicsView *view, QGraphicsScene *scene, int max, int min,
                          int *countline, int num_of_points,
                          QLabel *label, qint16 *showvalue,qint16 peaknowvalue,float gain)
{    
    //prepare
    //calculate where to put the x-axis(the middle of the scene)
    middlepeak=view->y()+(view->height()-2)/2;
    //design what is the middle value of your input siganl
    //from here
    signalmiddlepeak = (max+min)/2; //should be addition, not subtraction
    //end
    //decide where to begin to draw
    beginpeak=view->x();
    //decide the gap between every points(x-axis)
    increasepeak=((float)view->width()-2)/(float)num_of_points;
    //decide the scale of the signal(y-axis),scalerate=(scene'height)/(signal range you want to show)
    scaleratepeak=((float)(view->height()-2))/(float)(max-min); //ratio to max-min
    //prepareend

    //count the data position
    (*countline)++;
    if((*countline)>num_of_points+1)
        (*countline)=0;
    //end
    //drawing peak value
    if(validHRmax)//validHRmax,show new value
    {
        if((*countline)>=2)
        {
            scene->addLine(beginpeak+((*countline)-2)*increasepeak,middlepeak-((*showvalue)*gain-signalmiddlepeak)*scaleratepeak,beginpeak+((*countline)-1)*increasepeak,middlepeak-(peaknowvalue*gain-signalmiddlepeak)*scaleratepeak ,BFSIGNALCOLOR);
            if(((*countline)-1)*increasepeak>=scene->width()-3)
            {
                scene->clear();
            }
        }

        (*showvalue)=peaknowvalue;
        label->setText(QString("%1 uV").arg((float)peaknowvalue*((float)800/32767)));

        return 1;
    }//validHRmax
    else
    { //invalidHRmax,old to old
        label->setText(QString("%1 uV").arg((float)peaknowvalue*((float)800/32767)));
        if((*countline)>=2)
        {
            scene->addLine(beginpeak+((*countline)-2)*increasepeak,middlepeak-((*showvalue)*gain-signalmiddlepeak)*scaleratepeak,beginpeak+((*countline)-1)*increasepeak,middlepeak-((*showvalue)*gain-signalmiddlepeak)*scaleratepeak ,BFSIGNALCOLOR);
            if(((*countline)-1)*increasepeak>=scene->width()-3)
            {
                scene->clear();
            }
        }
    }

    return 0;
}


int biofeedback::drawecg(bool validHRmax, QGraphicsView *view, QGraphicsScene *scene, int max, int min,
                         int *countline,
                         int num_of_points, QLabel *label,  qint16 *showvalue, qint16 ecgnowvalue,float gain)
{
    //prepare
    //calculate where to put the x-axis(the middle of the scene)
    middlepeak=view->y()+(view->height()-2)/2;
    //design what is the middle value of your input siganl
    //from here
    signalmiddlepeak = (max+min)/2; //should be addition, not subtraction
    //end
    //decide where to begin to draw
    beginpeak=view->x();
    //decide the gap between every points(x-axis)
    increasepeak=((float)view->width()-2)/(float)num_of_points;
    //decide the scale of the signal(y-axis),scalerate=(scene'height)/(signal range you want to show)
    scaleratepeak=((float)(view->height()-2))/(float)(max-min); //ratio to max-min
    //prepareend

    //count the data position
    (*countline)++;
    if((*countline)>num_of_points+1)
        (*countline)=0;
    //end

    //drawing peak value
    if(validHRmax)//validHRmax,show new value
    {
        if((*countline)>=2)
        {
            scene->addLine(beginpeak+((*countline)-2)*increasepeak,middlepeak-((*showvalue)*gain-signalmiddlepeak)*scaleratepeak,beginpeak+((*countline)-1)*increasepeak,middlepeak-(ecgnowvalue*gain-signalmiddlepeak)*scaleratepeak ,BFSIGNALCOLOR);
            if(((*countline)-1)*increasepeak>=scene->width()-3)
            {
                scene->clear();
            }
        }

        (*showvalue)=ecgnowvalue;
        label->setText(QString("%1%").arg((float)(*showvalue)*100/32767));

        return 1;
    }//validHRmax
    else
    {
        //invalidHRmax,old to old
        label->setText(QString("%1%").arg((float)(*showvalue)*100/32767));
        if((*countline)>=2)
        {
            scene->addLine(beginpeak+((*countline)-2)*increasepeak,middlepeak-((*showvalue)*gain-signalmiddlepeak)*scaleratepeak,beginpeak+((*countline)-1)*increasepeak,middlepeak-((*showvalue)*gain-signalmiddlepeak)*scaleratepeak ,BFSIGNALCOLOR);
            if(((*countline)-1)*increasepeak>=scene->width()-3)
            {
                scene->clear();
            }
        }
    }

    return 0;
}

int biofeedback::draweeg(bool validHRmax, QGraphicsView *view, QGraphicsScene *scene, int max, int min,
                         int *countline,
                         int num_of_points, QLabel *label,  qint16 *showvalue,qint16 eegnowvalue,float gain)
{

    //prepare
    //calculate where to put the x-axis(the middle of the scene)
    middlepeak=view->y()+(view->height()-2)/2;
    //design what is the middle value of your input siganl
    //from here
    signalmiddlepeak = (max+min)/2; //should be addition, not subtraction
    //end
    //decide where to begin to draw
    beginpeak=view->x();
    //decide the gap between every points(x-axis)
    increasepeak=((float)view->width()-2)/(float)num_of_points;
    //decide the scale of the signal(y-axis),scalerate=(scene'height)/(signal range you want to show)
    scaleratepeak=((float)(view->height()-2))/(float)(max-min); //ratio to max-min
    //prepareend

    //count the data position
    (*countline)++;
    if((*countline)>num_of_points+1)
        (*countline)=0;
    //end

    //drawing peak value
    if(validHRmax)//validHRmax,show new value
    {
        if((*countline)>=2)
        {
            scene->addLine(beginpeak+((*countline)-2)*increasepeak,middlepeak-((*showvalue-fftco*327.7)*gain-signalmiddlepeak)*scaleratepeak,beginpeak+((*countline)-1)*increasepeak,middlepeak-((eegnowvalue-fftco*327.7)*gain-signalmiddlepeak)*scaleratepeak ,BFSIGNALCOLOR);
            if(((*countline)-1)*increasepeak>=scene->width()-3)
            {
                scene->clear();
            }
        }

        (*showvalue)=eegnowvalue; //in dB/330 => 2X=3dB=1000 and 10X=10dB=3300
        label->setText(QString("%1 dB").arg((*showvalue/327.7)));

        return 1;
    }//validHRmax
    else
    {
        //invalidHRmax,old to old
        label->setText(QString("%1 dB").arg((*showvalue/327.7)));
        if((*countline)>=2)
        {
            scene->addLine(beginpeak+((*countline)-2)*increasepeak,middlepeak-((*showvalue-fftco*327.7)*gain-signalmiddlepeak)*scaleratepeak,beginpeak+((*countline)-1)*increasepeak,middlepeak-((*showvalue-fftco*327.7)*gain-signalmiddlepeak)*scaleratepeak ,BFSIGNALCOLOR);
            if(((*countline)-1)*increasepeak>=scene->width()-3)
            {
                scene->clear();
            }
        }
    }

    return 0;
}

int biofeedback::drawRR(bool validHRmax,QGraphicsView *view,QGraphicsScene *scene,int max,int min,
                          int *countline,int num_of_points ,
                          QLabel *label, qint16 *showvalue,qint16 RRnowvalue,float gain)
{
    //prepare
    //calculate where to put the x-axis(the middle of the scene)
    middleRR=view->y()+(view->height()-2)/2;
    //design what is the middle value of your input siganl
    //from here
    signalmiddleRR = (max+min)/2; //should be addition, not subtraction
    //end
    //decide where to begin to draw
    beginRR=view->x();
    //decide the gap between every points(x-axis)
    increaseRR=((float)view->width()-2)/(float)num_of_points;
    //decide the scale of the signal(y-axis),scalerate=(scene'height)/(signal range you want to show)
    scalerateRR=((float)(view->height()-2))/(float)(max-min); //ratio to max-min
    //prepareend
    scene->update();
    //count the data position
    (*countline)++;
    if((*countline)>num_of_points+1)
        (*countline)=0;
    //end

    //drawing RR value
    if(validHRmax)//validHRmax,show new value
    {

        if((*countline)>=2)
        {
            scene->addLine(beginRR+((*countline)-2)*increaseRR,middleRR-((*showvalue)*gain-signalmiddleRR)*scalerateRR,beginRR+((*countline)-1)*increaseRR,middleRR-(RRnowvalue*gain-signalmiddleRR)*scalerateRR ,BFSIGNALCOLOR);
            if(((*countline)-1)*increaseRR>=scene->width()-3)
            {
                scene->clear();
            }
        }

        (*showvalue)=RRnowvalue;
        label->setText(QString("RR: %1ms; %2 bpm").arg((RRnowvalue)/10).arg(600000/RRnowvalue));//in msec
        return 1;
    }//validHRmax
    else
    { //invalidHRmax,old to old
        label->setText(QString("RR: %1ms; %2 bpm").arg(RRnowvalue/10).arg(600000/RRnowvalue));// in msec
        if((*countline)>=2)
        {
            scene->addLine(beginRR+((*countline)-2)*increaseRR,middleRR-((*showvalue)*gain-signalmiddleRR)*scalerateRR,beginRR+((*countline)-1)*increaseRR,middleRR-((*showvalue)*gain-signalmiddleRR)*scalerateRR ,BFSIGNALCOLOR);
            if(((*countline)-1)*increaseRR>=scene->width()-3)
            {
                scene->clear();
            }
        }
    }

    return 0;
}


void biofeedback::showgamewindow(int pdnnumber)
{
    tg = new tutorialgame(this);
    tg->show();
    tg->setpdnwindowtitle(pdnnumber);
}


void biofeedback::reset(bool setgamemode)
{
    ui->EDM->setText("");
    gamemode = setgamemode;
    eegmode = false; //initial OFF
    ecgmode = false; //initial OFF
    PDN_NUMBER=-1;
    newdata=false;
    already_open=false;
    temp_new_data=TG_MAX_TEMP; //initialize to high value
    adc_data_index_count=0;
    adc_fft_data_index_count=0;
    RRvaluenew=8000; //800msec=75bpm
    RRvalueold=8000;
    difsizeandoldpeak=16;
    RRreset=true;

    ampvaluenew=5000;
    ampvalueold=5000;

    G1oldvalue = 100;
    G2oldvalue = 100;
    M1oldvalue = 100;
    M2oldvalue = 100;
    UPoldvalue = 100;
    Betaoldvalue = 100;
    Sigmaoldvalue = 100;
    alphaoldvalue = 100;
    thetaoldvalue = 100;
    deltaoldvalue = 100;

    //clear all scene
    sceneSignalA->clear();
    sceneSignalB->clear();
    sceneSignalC->clear();
    sceneSignalD->clear();
    sceneSignalE->clear();

    //reset the drawing count
    countlineRR=0;
    countlinepeak=0;
    countlineG1=0;
    countlineG2=0;
    countlineM1=0;
    countlineM2=0;
    countlineUP=0;
    countlineBeta=0;
    countlineSigma=0;
    countlinealpha=0;
    countlinetheta=0;
    countlinedelta=0;

    adc_new_data.resize(BFSAMPLERATE_HR*BFTOTALSECOND_HR);
    adc_fft_new_data.resize(FFTSIZE);

    Sigma = Sigma_Default;
    alpha = alpha_Default;
    ui->alphatextEdit->setText(QString("%1").arg(alpha));
    ui->SigmatextEdit->setText(QString("%1").arg(Sigma));

    eegM2QV.resize(1);
    eegM1QV.resize(1);
    eegG2QV.resize(1);
    eegG1QV.resize(1);
    eegUPQV.resize(1);
    eegBetaQV.resize(1);
    eegSigmaQV.resize(1);
    eegalphaQV.resize(1);
    eegthetaQV.resize(1);
    eegdeltaQV.resize(1);

    ui->timeSlider->setValue(1);
    ui->timeSlider->setMinimum(1);
    ui->timemorelabel->setText(QString("%1s").arg((float)ui->timeSlider->value()*TIMEMORESTEP*0.125));

    //for zoom ctrl initialize value
    //from here
    ui->zoomctrl->setValue(0); //assume when value equals middle value of the zoomctrl then the zoomctrl has unit gain
    zoom_in_step=(float)BF_MAIN_MAX_SCALE/(float)ui->zoomctrl->maximum();
    zoom_out_step=(1-(float)BF_MAIN_MIN_SCALE)/(float)ui->zoomctrl->minimum();
    zoomrate= 1;  // set initial vertical zoom rate 1
    //end
    //for other zoom ctrl initialize value
    //from here
    ui->other_zoomctrl->setValue(0); //assume when value equals middle value of the zoomctrl then the zoomctrl has unit gain
    other_zoom_in_step=(float)BF_OTHER_MAX_SCALE/(float)ui->other_zoomctrl->maximum();
    other_zoom_out_step=(1-(float)BF_OTHER_MIN_SCALE)/(float)ui->other_zoomctrl->minimum();
    other_zoomrate= 1;  // set initial vertical zoom rate 1
    //end

    fftco=0; //??? not parameterized ???
    ui->fftcofactor->setText(QString("%1").arg(fftco));
}


void biofeedback::setPdnNum(int num)
{
    PDN_NUMBER=num;
    //set window title
    QString title;
    title.sprintf("[%d]",num);
    this->setWindowTitle(title);
    if(TDV==NULL)
        TDV = new twoDaccelviewer(true,(quint8) PDN_NUMBER, this);
    if(TDV!=NULL)
    {
        if(!gamemode)
        {
            TDV->livedisplaylayout();
            TDV->show();
            TDV->setGeometry(this->x()+this->width()+25, this->y()+30, TDV->width(), TDV->height());
        }
    }
}


biofeedback::~biofeedback()
{
    delete ui;
}


int biofeedback::getStruct(OPIPKT_t* opipointer)
{
    int i;

    PACKAGE_t.dataCode=opipointer->dataCode;
    PACKAGE_t.length=opipointer->length;
    for(i=0;i<opipointer->length;i++)
        PACKAGE_t.payload[i]=opipointer->payload[i];
     newdata=true;
    return this->routinedrawgroup();
}


void biofeedback::on_pushButton_5_clicked()
{
    //alpha
    //remainder
    int alphaIntTemp;
    alphaIntTemp = (int)(ui->alphatextEdit->toPlainText().toFloat()*EEGCONVERT05ACCURACY)%EEGCONVERT05ACCURACY;
    //left decimal point
    alpha = (int)(ui->alphatextEdit->toPlainText().toFloat()*EEGCONVERT05ACCURACY)/EEGCONVERT05ACCURACY;
    //choose
    if(alphaIntTemp>=75) (alpha) += 1;
    else if(alphaIntTemp<75&&alphaIntTemp>=25) (alpha) += 0.5;

    if(((alpha)<0) || ((alpha)>alpha_MAX))
    {
        alpha = alpha_Default;
    }

    //remainder
    int SigmaIntTemp;
    SigmaIntTemp = (int)(ui->SigmatextEdit->toPlainText().toFloat()*EEGCONVERT05ACCURACY)%EEGCONVERT05ACCURACY;
    //left decimal point
    Sigma = (int)(ui->SigmatextEdit->toPlainText().toFloat()*EEGCONVERT05ACCURACY)/EEGCONVERT05ACCURACY;
    //choose
    if(SigmaIntTemp>=75) (Sigma) += 1;
    else if(SigmaIntTemp<75&&SigmaIntTemp>=25) (Sigma) += 0.5;

    if(((Sigma)<0) || ((Sigma)>Sigma_MAX))
    {
        Sigma = Sigma_Default;
    }

    ui->alphatextEdit->setText(QString("%1").arg(alpha));
    ui->SigmatextEdit->setText(QString("%1").arg(Sigma));
}


void biofeedback::on_checkSignalA_currentIndexChanged(int index)
{
    sceneSignalA->clear();
    switch(index)
    {
    case STOP:
        ui->label_SignalA->setText("finding");
        break;
    case RR:
        countlineRR=0;
        break;
    case PEAK:
        countlinepeak=0;
        break;
    case G1:
        countlineG1=0;
        break;
    case G2:
        countlineG2=0;
        break;
    case M1:
        countlineM1=0;
        break;
    case M2:
        countlineM2=0;
        break;
    case UP:
        countlineUP=0;
        break;
    case BETA:
        countlineBeta=0;
        break;
    case SIGMA:
        countlineSigma=0;
        break;
    case ALPHA:
        countlinealpha=0;
        break;
    case THETA:
        countlinetheta=0;
        break;
    case DELTA:
        countlinedelta=0;
        break;
    }

    if(ui->checkSignalB->currentIndex() == index)
    {
        ui->label_SignalB->setText("finding");
        ui->checkSignalB->setCurrentIndex(STOP);
    }
    if(ui->checkSignalC->currentIndex() == index)
    {
        ui->label_SignalC->setText("finding");
        ui->checkSignalC->setCurrentIndex(STOP);
    }
    if(ui->checkSignalD->currentIndex() == index)
    {
        ui->label_SignalD->setText("finding");
        ui->checkSignalD->setCurrentIndex(STOP);
    }
    if(ui->checkSignalE->currentIndex() == index)
    {
        ui->label_SignalE->setText("finding");
        ui->checkSignalE->setCurrentIndex(STOP);
    }
}


void biofeedback::on_checkSignalB_currentIndexChanged(int index)
{
    sceneSignalB->clear();
    switch(index)
    {
    case STOP:
        ui->label_SignalB->setText("finding");
        break;
    case RR:
        countlineRR=0;
        break;
    case PEAK:
        countlinepeak=0;
        break;
    case G1:
        countlineG1=0;
        break;
    case G2:
        countlineG2=0;
        break;
    case M1:
        countlineM1=0;
        break;
    case M2:
        countlineM2=0;
        break;
    case UP:
        countlineUP=0;
        break;
    case BETA:
        countlineBeta=0;
        break;
    case SIGMA:
        countlineSigma=0;
        break;
    case ALPHA:
        countlinealpha=0;
        break;
    case THETA:
        countlinetheta=0;
        break;
    case DELTA:
        countlinedelta=0;
        break;
    }
    if(ui->checkSignalA->currentIndex() == index)
    {
        ui->label_SignalA->setText("finding");
        ui->checkSignalA->setCurrentIndex(STOP);
    }
    if(ui->checkSignalC->currentIndex() == index)
    {
        ui->label_SignalC->setText("finding");
        ui->checkSignalC->setCurrentIndex(STOP);
    }
    if(ui->checkSignalD->currentIndex() == index)
    {
        ui->label_SignalD->setText("finding");
        ui->checkSignalD->setCurrentIndex(STOP);
    }
    if(ui->checkSignalE->currentIndex() == index)
    {
        ui->label_SignalE->setText("finding");
        ui->checkSignalE->setCurrentIndex(STOP);
    }
}


void biofeedback::on_checkSignalC_currentIndexChanged(int index)
{
    sceneSignalC->clear();
    switch(index)
    {
    case STOP:
        ui->label_SignalC->setText("finding");
        break;
    case RR:
        countlineRR=0;

        break;
    case PEAK:
        countlinepeak=0;

        break;

    case G1:
        countlineG1=0;

        break;
    case G2:
        countlineG2=0;

        break;
    case M1:
        countlineM1=0;

        break;
    case M2:
        countlineM2=0;

        break;
    case UP:
        countlineUP=0;

        break;
    case BETA:
        countlineBeta=0;

        break;
    case SIGMA:
        countlineSigma=0;

        break;
    case ALPHA:
        countlinealpha=0;

        break;
    case THETA:
        countlinetheta=0;

        break;
    case DELTA:
        countlinedelta=0;

        break;
    }
    if(ui->checkSignalA->currentIndex() == index)
    {
        ui->label_SignalA->setText("finding");
        ui->checkSignalA->setCurrentIndex(STOP);
    }
    if(ui->checkSignalB->currentIndex() == index)
    {
        ui->label_SignalB->setText("finding");
        ui->checkSignalB->setCurrentIndex(STOP);
    }
    if(ui->checkSignalD->currentIndex() == index)
    {
        ui->label_SignalD->setText("finding");
        ui->checkSignalD->setCurrentIndex(STOP);
    }
    if(ui->checkSignalE->currentIndex() == index)
    {
        ui->label_SignalE->setText("finding");
        ui->checkSignalE->setCurrentIndex(STOP);
    }
}


void biofeedback::on_checkSignalD_currentIndexChanged(int index)
{
    sceneSignalD->clear();
    switch(index)
    {
    case STOP:
        ui->label_SignalD->setText("finding");
        break;
    case RR:
        countlineRR=0;

        break;
    case PEAK:
        countlinepeak=0;

        break;
    case G1:
        countlineG1=0;

        break;
    case G2:
        countlineG2=0;

        break;
    case M1:
        countlineM1=0;

        break;
    case M2:
        countlineM2=0;

        break;
    case UP:
        countlineUP=0;

        break;
    case BETA:
        countlineBeta=0;

        break;
    case SIGMA:
        countlineSigma=0;

        break;
    case ALPHA:
        countlinealpha=0;

        break;
    case THETA:
        countlinetheta=0;

        break;
    case DELTA:
        countlinedelta=0;
        break;
    }
    if(ui->checkSignalA->currentIndex() == index)
    {
        ui->label_SignalA->setText("finding");
        ui->checkSignalA->setCurrentIndex(STOP);
    }
    if(ui->checkSignalB->currentIndex() == index)
    {
        ui->label_SignalB->setText("finding");
        ui->checkSignalB->setCurrentIndex(STOP);
    }
    if(ui->checkSignalC->currentIndex() == index)
    {
        ui->label_SignalC->setText("finding");
        ui->checkSignalC->setCurrentIndex(STOP);
    }
    if(ui->checkSignalE->currentIndex() == index)
    {
        ui->label_SignalE->setText("finding");
        ui->checkSignalE->setCurrentIndex(STOP);
    }
}


void biofeedback::on_checkSignalE_currentIndexChanged(int index)
{
    sceneSignalE->clear();
    switch(index)
    {
    case STOP:
        ui->label_SignalE->setText("finding");
        break;
    case RR:
        countlineRR=0;

        break;
    case PEAK:
        countlinepeak=0;

        break;
    case G1:
        countlineG1=0;

        break;
    case G2:
        countlineG2=0;

        break;
    case M1:
        countlineM1=0;

        break;
    case M2:
        countlineM2=0;

        break;
    case UP:
        countlineUP=0;

        break;
    case BETA:
        countlineBeta=0;

        break;
    case SIGMA:
        countlineSigma=0;

        break;
    case ALPHA:
        countlinealpha=0;

        break;
    case THETA:
        countlinetheta=0;

        break;
    case DELTA:
        countlinedelta=0;

        break;
    }
    if(ui->checkSignalA->currentIndex() == index)
    {
        ui->label_SignalA->setText("finding");
        ui->checkSignalA->setCurrentIndex(STOP);
    }
    if(ui->checkSignalB->currentIndex() == index)
    {
        ui->label_SignalB->setText("finding");
        ui->checkSignalB->setCurrentIndex(STOP);
    }
    if(ui->checkSignalC->currentIndex() == index)
    {
        ui->label_SignalC->setText("finding");
        ui->checkSignalC->setCurrentIndex(STOP);
    }
    if(ui->checkSignalD->currentIndex() == index)
    {
        ui->label_SignalD->setText("finding");
        ui->checkSignalD->setCurrentIndex(STOP);
    }
}


void biofeedback::on_timeSlider_valueChanged(int value)
{
    ui->timemorelabel->setText(QString("%1s").arg((float)ui->timeSlider->value()*TIMEMORESTEP*0.125));
    countlineRR=0;
    countlinepeak=0;
    countlineG1=0;
    countlineG2=0;
    countlineM1=0;
    countlineM2=0;
    countlineUP=0;
    countlineBeta=0;
    countlineSigma=0;
    countlinealpha=0;
    countlinetheta=0;
    countlinedelta=0;
    sceneSignalA->clear();
    sceneSignalB->clear();
    sceneSignalC->clear();
    sceneSignalD->clear();
    sceneSignalE->clear();
}


void biofeedback::on_zoomctrl_valueChanged(int value)
{
    zoomctrlvalue=ui->zoomctrl->value();
    if(zoomctrlvalue>0)
    zoomrate=1+zoomctrlvalue*zoom_in_step;
    else if(zoomctrlvalue<0)
    zoomrate=1-zoomctrlvalue*zoom_out_step;
    else
    zoomrate=1;
}


void biofeedback::on_other_zoomctrl_valueChanged(int value)
{
    other_zoomctrlvalue=ui->other_zoomctrl->value();
    if(other_zoomctrlvalue>0)
    other_zoomrate=1+other_zoomctrlvalue*other_zoom_in_step;
    else if(other_zoomctrlvalue<0)
    other_zoomrate=1-other_zoomctrlvalue*other_zoom_out_step;
    else
    other_zoomrate=1;
}


void biofeedback::on_fftbutton_clicked()
{
    fftco=ui->fftcofactor->toPlainText().toFloat();
}


void biofeedback::on_pushButton_2D_accel_clicked()
{
    if(TDV!=NULL)
    {
        if(!TDV->isVisible())
        {
            TDV->livedisplaylayout();
            TDV->show();
        }
    }
}
