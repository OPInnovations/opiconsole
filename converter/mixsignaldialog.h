#ifndef MIXSIGNALDIALOG_H
#define MIXSIGNALDIALOG_H

#include <QtGui>


#define EEGSAMPLERATE 8
#define EEGEDFDRDURSEC 1
#define EEGLEN 1


namespace Ui {
class mixsignaldialog;
}

class mixsignaldialog : public QDialog
{
    Q_OBJECT
    QVector <qint16> MixSignal;

    QVector <qint16> M2qvect;
    QVector <qint16> M1qvect;
    QVector <qint16> G2qvect;
    QVector <qint16> G1qvect;
    QVector <qint16> UPqvect;
    QVector <qint16> Betaqvect;
    QVector <qint16> Sigmaqvect;
    QVector <qint16> alphaqvect;
    QVector <qint16> thetaqvect;
    QVector <qint16> deltaqvect;

public:
    explicit mixsignaldialog(QVector<qint16> *M2qvectp, QVector<qint16> *M1qvectp,
                             QVector<qint16> *G2qvectp, QVector<qint16> *G1qvectp,
                             QVector<qint16> *UPqvectp,
                             QVector<qint16> *Betaqvectp, QVector<qint16> *Sigmaqvectp,
                             QVector<qint16> *alphaqvectp,
                             QVector<qint16> *thetaqvectp, QVector<qint16> *deltaqvectp,
                             QWidget *parent = 0);
    ~mixsignaldialog();
    QVector <qint16> getMixSignal();

private:
    Ui::mixsignaldialog *ui;
    void reset();
};

#endif // MIXSIGNALDIALOG_H
