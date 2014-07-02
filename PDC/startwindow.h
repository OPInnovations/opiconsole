#ifndef STARTWINDOW_H
#define STARTWINDOW_H

#include <QtGui>
#include "showdatawindow.h"
#include "opi_win.h"

#define WORNNINGWORDSIZE 10
//get package time (ms)
#define startwindow_fresh_time 20
#define NEWDATAGETCODE 1
#define OPIWANT_MAX 4 //how many opi pdn we want
//comport open number
#define PDCCOMPORTUSER 2
#define PDCCOMPORTFREE 0
#define TEMPGAPTHRESHOLD 7 //define the acceptable value of  absolute value of (old temp-new temp)

namespace Ui {
class startwindow;
}

class  startwindow : public QMainWindow
{
    Q_OBJECT

    QDateTime str,sdt;
    //to count the number of pdn device you want to show
    int Show_count;
    //for loop counter
    int i;
    //file pointer
    FILE *file;
    //for showing message
    QMessageBox show_message;
    //save the word of pdn number to show on the screen
    QString pdntemp;  
    // for com port operation
    HANDLE com;
    //the showdatawindow array
    showdatawindow *sdw[OPIWANT_MAX+1];

    //the struct of opipkt
    OPIPKT_t PACKAGE_tp[OPIWANT_MAX+1];  //opipkt_t[0] is for saving temp package
    //decide the pdn has been choose or not true for being chosen
    bool PDNBOX[OPIWANT_MAX+1];
    //if true means the user want to show that pdn
    bool pdnOn[OPIWANT_MAX+1] ;
    //save the pdn number can use
    int  PDN_NUMBER[5];
    //save the location of pdnnumber in array[index] (ex 0 1 2 3 4)
    int  PDN_LOCATION[256];
    //save the total numbers of pdn can use
    int  get_opipkt_total;
    //save the timer id
    int timeid;
    bool  portOn;  //if opiconnect=-1 and portON=false
    //save the timer is the first time using or not
    bool firsttimer;
    //save the opiucd code and zbchan
    int  ucd[5],zbchan;
    //save the filename of the user want
    QString userfilename,regulatefilename;
    //check if file is the first time open
    bool firstfile; //true for first time open
    //check if the pdn is the first uie
    bool firstpdn[5]; //initial true
    //check the comport
    qint32 *comPortUserp;
    //check the numbers of showing pdn
    qint32 pdnshowcountp;
    int freshpdnindex;
public:
    explicit  startwindow(qint32 *comPortUser, QWidget *parent = 0);
    ~ startwindow();
    //stop the comport,let others to use comport
    void borrowCOM();
    //return the comport
    void returnCOM();
private slots:
    void on_pushButton_clicked();

    void on_CBP1_clicked();

    void on_CBP2_clicked();

    void on_CBP3_clicked();

    void on_CBP4_clicked();

    void on_pushButton_2_clicked();


private:
    Ui:: startwindow *ui;
    //get how many pdn can use(return it) and get pdn number(save it)
    int opiucd_getPDN(void);
    //hide every pdn
    void hide_check_pdn(void);
    //show number of pdn can use
    void show_check_pdn(void);
    //get the new package and refresh the structure
    int fresh(void);

    int opipkt_put_file(OPIPKT_t* pktptr, FILE* fileptr);
protected:
     void timerEvent(QTimerEvent *);
     void closeEvent(QCloseEvent *);
     void showEvent(QShowEvent *);
};


#endif // STARTWINDOW_H
