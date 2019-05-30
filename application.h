#ifndef APPLICATION_H
#define APPLICATION_H

#include "communicator.h"
#include "stats.h"

#include <QObject>
#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QCoreApplication>

#include <ctime>
#include <random>
#include <cstdlib>
#include <cmath>

struct TInterfaceMessage
{
    QString DoubleData;
    QString ChrData;
};



class TApplication : public QCoreApplication
{
    Q_OBJECT

    TCommunicator *comm;

public:
    TApplication(int, char**);
    void createData(QString);

signals:
    void toCommunicator(QByteArray);

public slots:
    void fromCommunicator(QByteArray);
    void timedSend();
    void createIntervals();
    void createBreakInterval();
    void changeRepaired();
    void changeBroke();

private:
   int speed = 2;
   //timers
   QTime *watch;
   QTimer *timer;
   std::vector<QTimer *> free;
   std::vector<QTimer *> busy;
   QTimer *broke;
   QTimer *hour;
   //changing variables
   int freeworker;
   int busyWorker = 0;
   int okayWorker;
   int brokenWorkers = 0;
   int repairInt;
   int breakAm;
   int onRepair = 0;
   std::vector<double> breakIntervals;
   bool begin = true;
   std::default_random_engine generator;
   //for pause
   int remTimer = -1;
   int remHour = -1;
   int remBroke = -1;
   std::vector<int> remBusy;
   stats *stat;

signals:
   void timerActive(QByteArray);
   void vectorReady();
};

#endif // APPLICATION_H
