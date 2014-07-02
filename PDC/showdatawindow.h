#ifndef SHOWDATAWINDOW_H
#define SHOWDATAWINDOW_H

#include <QtGui>
#include "../FFT/qfouriertransformer.h"
#include "../FFT/qcomplexnumber.h"
#include "opi_linux.h"
#include "../converter/twodaccelviewer.h"


//initial the adc sample
#define INITIALADC 2048
#define INITIALFFT 512
//define how many path(Points) you want to display
#define NUMPATHX 90
#define NUMPATHY 90
#define NUMPATHZ 360
#define NUMPATHTEMP 90
#define NUMPATHFFT 1024  //the value also means the sum of data you want to do FFT,do fft once every N times
#define FFTPENWIDTH 2 //set width of pen for fft
#define NUMPATHADC 30720 //initialize the max value of the timeSlider to be this to make sure the function work
#define NUMPOINTSAMPLE  64 //it has to be a even number 4 8 16 32 64 //default NUMPOINTSAMPLE=64 for full FFT
#define NUMPOINTSAMPLE_X 1
#define NUMPOINTSAMPLE_Y 1
#define NUMPOINTSAMPLE_Z 4
#define NUMPOINTSAMPLE_TEMP 1
#define PENWIDTH 1     //define the pen width
#define MAX_SCALE 25    //the max scale rate for adc you want
#define MIN_SCALE 1/5  //the min scale rate for adc you want
#define FFT_MAX_SCALE 5  //the max scale rate for fft you want
#define FFT_MIN_SCALE 1/5   //the max scale rate for fft you want
#define MAX_MIN_X 120  //the absolute value of max and min should be the same
#define MAX_MIN_Y 120  //the absolute value of max and min should be the same
#define MAX_MIN_Z 120  //the absolute value of max and min should be the same
#define MAX_TEMP 4500 // x100 temperature value
#define MIN_TEMP 1500 // x100 temperature value
#define MAX_MIN_ADC 8000 //the absolute value of max and min should be the same 5000
#define FFTSIGNALMIN 1     //the min value of the signal in db
#define FFTSIGNALMAX 255   //the max value of the signal in db
//define the min points to show on the adc graphicsView,and the max points be defined by NUMPATHADC
#define TIMESLIDER_MIN_SAMPLE_POINTS 256 //should be times of NUMPOINTSAMPLE
#define MIDDLELINEPIXELACCURATE 1//is to control if signal is at middle ,if any signal in the screen is between middle+-MIDDLELINEPIXELACCURATEpixels we hide the middle line


namespace Ui {
class showdatawindow;
}

class showdatawindow : public QMainWindow
{
    int tempcount;
    Q_OBJECT
    twoDaccelviewer *TDV;
    //let the final data can reach the next start data
    bool first_Time_Samplez; //initial true;
    bool first_Time_Sampleadc;
    QMessageBox show_message;
    //to do the fft calculate job
    QFourierTransformer transformer;  //should Setting a fixed size for the transformation
    //struct of opipkt
    OPIPKT_t PACKAGE_t;

    //to save the window title words
    QString title;
    //skip range
    int max_min_pair;
    //to temporary save the data in showDataControl(void)
    float temp_new_data, temp_last_data;
    int x_new_data,y_new_data;
    int adc_new_data[NUMPOINTSAMPLE]; //sample the max and min form 0~8 8~16 16~24 24~32 32~40 40~48 48~56 56~63 and even number of index means min otherwise means max
    int z_new_data[NUMPOINTSAMPLE_Z];
    QString showstring,showtemp;
    //to calculate for calNewPack Function
    int adctempcount; //for temporary saving the ADC ?tempeture
    //to count the location to save in saveData(void) Function
    int countx,county,countz,countadc,countfft,countfftcolumn,counttemp; //initial 0
    int countguidelinez,countguidelineadc; //initial 0
    //to save the value form the zoomctrl bar
    float zoomctrlvalue; //save the value on zoomctrl
    float zoomrate; //initialized as  1
    float zoom_out_step; //initialized as the value that every gap you have to take for every step
    float zoom_in_step;  //initialized as the value that every gap you have to take for every step
    //to save the parameter for fft
    float fftk,fftco; //initial fftk=8 (-)fftco=2 (-70 built-in => -72db)
    QColor ffthsl;
    QPen   fftPen;
    //to save the data for drawing
    int saveTemp[NUMPATHTEMP];//integer must use x10 to avoid rounding
    float saveFFT[NUMPATHFFT];//to save raw data,uncaculated to fft,just adc
    float calculatedFFT[NUMPATHFFT]; //to save the data after FFT calculation
    int saveX[NUMPATHX],saveY[NUMPATHY],saveZ[NUMPATHZ];
    int nowvalue,beforevalue; //to temperory save the data to draw in drawData Function
    int beforevalueadc;
    QVector <QComplexFloat> fftresult; // to save the result fft data uneffected by ko co
    double effected_by_ko_co_fft[NUMPATHFFT/2];  // to save the result fft data effected by ko co for drawFFT Function
    float increase_log[NUMPATHFFT/2+1]; //decide the gap between every points(x-axis)in LOG scale & as FFT_equalization
    bool   fft_is_ready; //initial false , if true means that the fft row is ready to calculate
    double re,im,ffttemp; //real and imiage
    //the following paramater are used in darwfft Function
    double fftnowvalue;
    double fftscreenmiddle,fftsignalmiddle;
    bool   fft_clean; //if the screen is full then clean it,false means donot clean //initial false,
    int numpathfftcol;//define the how many collumns in fft screen
    //to save the value form the FFTGainSlider bar
    float fft_zoomctrlvalue; //save the value on FFTGainSlider
    float fft_zoomrate; //initialized as  1
    float fft_zoom_out_step; //initialized as the value that every gap you have to take for every step
    float fft_zoom_in_step;  //initialized as the value that every gap you have to take for every step
    float fft_total; //total display width in LOG scale
    //to save the parameter for fft
    //build the scene for QGraphicsView
    QGraphicsScene *sceneTemp,*sceneX,*sceneY,*sceneZ,*sceneADC,*scenefft;
    //defining the scaled show points size for adc and save the value form the timeSlider bar
    int numScaleADC; //be initialized as timeSlider current value and be used in timeSlider triggle Function "on_timeSlider_valueChanged(int value)"
    //the following for drawData Function
    //from here
    int begin; //decide where to begin to draw
    float fft_each_row_height_increase; //decide the height  of each row in fft screen,be used in drawFFT Function
    int button; //decide the x-axis of fft screen
    double rr,gg,bb;//decide the color in drawFFT Function
    float increase; //decide the gap between every points(x-axis)
    float scalerate; //decide the scale of the signal(y-axis),scalerate=(scene'height)/(signal range you want to show)
    int middle; //calculate where to put the x-axis(the middle of the scene)
    int signalmiddle; //design what is the middle value of your input siganl
    int numScalefft; //decide the number of bins you want to show ,initial
    //end
    //remember the guide line location
    int before_guide; //initial -1 to  reduce the param we use
    //temporarily save the number of showing pdn
    qint32 *pdnshowcount;
    //test dump
    float tempavg,adcavg,accxavg,accyavg,acczavg,ffgavg;
public:
    explicit showdatawindow(qint32 *pdnshowcountp, QWidget *parent = 0);
    ~showdatawindow();
    // control all the  draw data Function
    int routinedrawgroup(void);
    //set up all scene for graphics view
    void setUpTotalScene(void);
    //check the window is already open or not true for being opened
    bool already_open;
    //check whether get a new package
    bool newdata;
    //save the pdn number can use
    int  PDN_NUMBER;
    //initialize the value
    void reset(void);
    //get whole pdn struct
    int  getStruct(OPIPKT_t* opipointer);
    // set pdn number
    void setPdnNum(int num);
    //to show the datas on the screen
    void showDataControl(void);
    //to calculate the new package to temperature accx accy accz adc fft
    int calNewPack(void); //call it when getting a new package ,be used in temperature accx accy accz adc
    //save the data form index 0 to NUMPATH when save one data the index point to the next,so the current data you just receive is located in (index-1)
    int saveData(void); //each time when you finish calNewPack,then call the saveData to save it,prepare to give it to drawData Function,be used in temperature accx accy accz adc
    //to draw data for temperature accx accy accz adc
    //the following will indroduce the parameter's job
    //*view  is the graphicsview you use in the showdatawindow
    //*scene is the graphicscene you stamp on the view
    //max   is the max value you want on the view
    //min   is the min value you want on the view
    //*database is the array you want to draw on the scene
    //count is to  define the number you have to draw when the *database is not full
    //num_of_points is the points you want to draw on the scene,usually use numPath we define as it
    //sample_points is to show how many points at one time
    //*line[] to remenber and destory the lines
    //*countline is to count the signal position,if it is 0 means no use it;  for adc ,z
    //first_Time_Sample is to check the drawing in the first time,if not send by parameter means no use it
    int drawData(QGraphicsView *view,QGraphicsScene *scene,int max,int min,int *database,int count,int num_of_points,int sample_points,int *countline,bool first_Time_Sample);
    //draw fft on the screen
    int drawFFT(void);

private slots:
    void on_zoomctrl_valueChanged(int value);

    void on_fftbutton_clicked();

    void on_timeSlider_valueChanged(int value);

    void on_FFT_TimeSlider_valueChanged(int value);

    void on_FFTGainSlider_valueChanged(int value);

    void on_checkReverse_toggled(bool checked);

    void on_pushButton_2D_accel_clicked();

private:
    Ui::showdatawindow *ui;
protected:
    void closeEvent(QCloseEvent *);
    void showEvent(QShowEvent *);

};

#endif // SHOWDATAWINDOW_H
