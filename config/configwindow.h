#ifndef CONFIGWINDOW_H
#define CONFIGWINDOW_H

#include <QtGui>
#include "opi_win.h"


namespace Ui {
class ConfigWindow;
}

class ConfigWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit ConfigWindow(QWidget *parent = 0);
    ~ConfigWindow();
    
private slots:
    void on_ucStatusPB_clicked();

    void on_ucModOnOffCB_activated(int index);

    void on_ucSetTimePB_clicked();

    void on_ucWLMeasurePB_clicked();

    void on_ucSetZBPB_clicked();

    void on_ucReadEventsPB_clicked();

    void on_ucEraseEventsPB_clicked();

    void on_ucCopySensPB_clicked();

    void on_ucForgetSensPB_clicked();

    void on_tsStatusPB_clicked();

    void on_tsSetTimePB_clicked();

    void on_tsTogMMWritePB_clicked();

    void on_tsSetPDNPB_clicked();

    void on_tsSetZBPB_clicked();

    void on_tsSetRFModePB_clicked();

    void on_tsSetRFTOPB_clicked();

    void on_mmReadPktsPB_clicked();

    void on_mmChipErasePB_clicked();

    void on_ucShutdownPB_clicked();

private:
    Ui::ConfigWindow *ui;
    void ErrorMsgBox(QString errMsg);
    void opipkt_put_stream(OPIPKT_t *pktptr, QDataStream *osptr);
};

#endif // CONFIGWINDOW_H
