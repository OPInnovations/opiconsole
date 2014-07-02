#ifndef CONSOLEWINDOW_H
#define CONSOLEWINDOW_H

#include <QMainWindow>
#include "config/configwindow.h"
#include "converter/convertwindow.h"
#include "PDC/startwindow.h"
#include "biofeedback/startwindowbf.h"
#include "TutorialGame/tutorialgame.h"
#include "sleepanalysis/sleepanalysiswindow.h"
#include "medanalysis/medanalysiswindow.h"

#define CONCOMPORTUSER 1    // number for console using comport
#define COMPORTFREE 0
#define TSCTSUPP  4   // Number of truesense sensors supported in this application
#define DEFRFTXPWR 7    // default rf tx power set when pairing

namespace Ui {
class ConsoleWindow;
}

class ConsoleWindow : public QMainWindow
{
    Q_OBJECT
    // for com port operation
    HANDLE com;
public:
    explicit ConsoleWindow(QWidget *parent = 0);
    ~ConsoleWindow();
    
private slots:
    void on_convertPB_clicked();

    void on_rtDisplayPB_clicked();

    void on_profMgrPB_clicked();

    void on_contRefreshPB_clicked();

    void on_sensShutdownPB_clicked(bool checked);

    void on_contShutdownPB_clicked();

    void on_sensAUnpairPB_clicked();

    void on_sensBUnpairPB_clicked();

    void on_sensCUnpairPB_clicked();

    void on_sensDUnpairPB_clicked();

    void on_sensUPairPB_clicked();

    void on_sensARFRB_clicked(bool checked);

    void on_sensAMMRB_clicked(bool checked);

    void on_sensADTRB_clicked(bool checked);

    void on_sensBRFRB_clicked(bool checked);

    void on_sensBMMRB_clicked(bool checked);

    void on_sensBDTRB_clicked(bool checked);

    void on_sensCRFRB_clicked(bool checked);

    void on_sensCMMRB_clicked(bool checked);

    void on_sensCDTRB_clicked(bool checked);

    void on_sensDRFRB_clicked(bool checked);

    void on_sensDMMRB_clicked(bool checked);

    void on_sensDDTRB_clicked(bool checked);

    void on_memUploadPB_clicked();

    void on_bioFBPB_clicked();

    void on_contTagUploadPB_clicked();

    void on_memErasePB_clicked();

    void on_aboutPB_clicked();

    void on_slpAnalyzePB_clicked();

    void on_tgPB_clicked();

    void on_medAnalyzePB_clicked();

private:
    Ui::ConsoleWindow *ui;

    ConfigWindow *cfgWin;
    ConvertWindow *cnvtWin;
    startwindow   *pdc;
    startwindowbf   *bf;
    startwindowbf   *tg;
    SleepAnalysisWindow *slpWin;
    MedAnalysisWindow *medWin;
    qint32 comPortUser;
    qint32 comPortLender;
    quint8 tsSlotPdns[TSCTSUPP];    // for remembering what ts is in what slot in display

    qint32 conBorrowCom();
    qint32 conReturnCom();
    void defaultConsoleState();
    qint32 ucRefresh(HANDLE *comportptr);
    qint32 ucPair(HANDLE *comportptr);
    void hideAllSensConfigElements();
    void disableAllConfigElements();
    void restoreColorAllSensConfigElements();

protected:
    void closeEvent(QCloseEvent *);
};

#endif // CONSOLEWINDOW_H
