#ifndef STARTWINDOWBF_H
#define STARTWINDOWBF_H

#include <QtGui>
#include "opi_win.h"
#include "biofeedback.h"
#include "../edflib/edflib.h"

#define BFWORNNINGWORDSIZE 10
//get package time (ms)
#define startwindow_fresh_time_BF 20
#define NEWDATAGETCODE_BF 1
#define OPIWANT_MAX_BF 4 //how many opi pdn we want


//comport open number
#define BFCOMPORTUSER 3
#define TGCOMPORTUSER 4
#define BFCOMPORTFREE 0

#define TEMPGAPTHRESHOLD_BF 7 //define the acceptable value of  absolute value of (old temp-new temp)

namespace Ui {
class startwindowbf;
}

class startwindowbf : public QMainWindow
{
    Q_OBJECT
    //to count the number of pdn device you want to show
    int Show_count;
    //for loop counter
    int i;
    
	//file pointer to the output OPI file
    FILE *opiFile;

	//file pointer to the output EDF file
    FILE *edfFile;

	// (edflib) file handle of the EDF file
	int edfFileHandle;

	//for showing message
    QMessageBox show_message;
    
	//save the word of pdn number to show on the screen
    QString pdntemp;
    
	// for COM port operation
    HANDLE com;
    
	//the showdatawindow array
    biofeedback *sdw[OPIWANT_MAX_BF+1];

    //decide the pdn has been choose or not true for being chosen
    bool PDNBOX[OPIWANT_MAX_BF+1];
    
	//if true means the user want to show that pdn
    bool pdnOn[OPIWANT_MAX_BF+1] ;
    
	//save the pdn number can use
    int  PDN_NUMBER[5];
    
	//save the location of pdnnumber in array[index] (ex 0 1 2 3 4)
    int  PDN_LOCATION[256];
    
	//save the total numbers of pdn can use
    int  get_opipkt_total;
    
	//save the timer id
    int timeid;

    //bool  portOn;  //if opiconnect=-1 and portON=false
    
	//save the timer is the first time using or not -- NOTE: this is true if the timer is not used
    //bool firsttimer;
    
	//save the opiucd code and zbchan
    int  ucd[5],zbchan;
          
	//check if the pdn is the first uie
    bool firstpdn[5]; //initial true
    
	//check the comport
    qint32 *comPortUserp;
    
	//check the numbers of showing pdn
    qint32 pdnshowcountp;

    bool gamemode;
public:
    explicit startwindowbf(qint32 *comPortUser,bool setgamemode, QWidget *parent = 0);
    ~startwindowbf();
    void borrowCOM();    //stop the comport,let others to use comport
    void returnCOM();    //return the comport


private:
    Ui:: startwindowbf *ui;
    //get how many pdn can use(return it) and get pdn number(save it)
    int opiucd_getPDN(void);
    //hide every pdn
    void hide_check_pdn(void);
    //show number of pdn can use
    void show_check_pdn(void);
    //get the new package and refresh the structure
    int fresh(void);

    int opipkt_put_file(OPIPKT_t* pktptr, FILE* fileptr);

	void writeDataToEDFFile(OPIPKT_DC01_SDC01_t package, int edfFileHandle);

	// kill the timer
	void killUIRefreshTimer();

	// close the COM port user for communication with the Controller
	void closeComPort();

	// close both OPI and EDF files
	void closeFiles();
protected:
     void timerEvent(QTimerEvent *);
     void closeEvent(QCloseEvent *);
     void showEvent(QShowEvent *);

private slots:
     void on_btnCBP1_clicked();
     void on_btnCBP2_clicked();
     void on_btnCBP3_clicked();
     void on_btnCBP4_clicked();
     void on_btnRefresh_clicked();
     void on_btnStart_clicked();
};

#endif // STARTWINDOWBF_H
