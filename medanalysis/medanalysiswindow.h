#ifndef MEDANALYSISWINDOW_H
#define MEDANALYSISWINDOW_H

#include <QtGui>
#include "../converter/twodaccelviewer.h"

namespace Ui {
class MedAnalysisWindow;
}

class MedAnalysisWindow : public QMainWindow
{
    Q_OBJECT


public:
    explicit MedAnalysisWindow(QWidget *parent = 0);
    ~MedAnalysisWindow();

private slots:
    void on_inBrwsPB_clicked();

    void on_cnvtPB_clicked();

private:
    Ui::MedAnalysisWindow *ui;

    twoDaccelviewer *TDVp;
    float   Sigma,alpha;
    void medanalyzeD();
    bool edfMmedhdropiwrite(QDataStream *out, QString *lpidp, QString *lridp,
                            QDateTime *startDTp, qint32 numDataRecs);
    void edfMmedwrite(QDataStream *outstrp, QVector<qint16> *sleephypQVp,
                      qint32 numDataRecs, qint64 firstFrmTS, QVector<qint64> *tagTSQVp,
                      QVector<QString> *tagTextQVp);

};

#endif // MEDANALYSISWINDOW_H
