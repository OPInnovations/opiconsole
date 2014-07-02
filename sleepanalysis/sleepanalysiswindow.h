#ifndef SLEEPANALYSISWINDOW_H
#define SLEEPANALYSISWINDOW_H

#include <QtGui>
#include "../converter/twodaccelviewer.h"

namespace Ui {
class SleepAnalysisWindow;
}

class SleepAnalysisWindow : public QMainWindow
{
    Q_OBJECT


public:
    explicit SleepAnalysisWindow(QWidget *parent = 0);
    ~SleepAnalysisWindow();

private slots:
    void on_inBrwsPB_clicked();

    void on_cnvtPB_clicked();

private:
    Ui::SleepAnalysisWindow *ui;

    twoDaccelviewer *TDVp;
    float   Sigma,alpha;
    void sleepanalyzeD();
    bool edfMslphdropiwrite(QDataStream *out, QString *lpidp, QString *lridp,
                            QDateTime *startDTp, qint32 numDataRecs);
    void edfMslpwrite(QDataStream *outstrp, QVector<qint16> *sleephypQVp,
                      qint32 numDataRecs, qint64 firstFrmTS, QVector<qint64> *tagTSQVp,
                      QVector<QString> *tagTextQVp);

};

#endif // SLEEPANALYSISWINDOW_H
