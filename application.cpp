#include "application.h"


TApplication::TApplication(int argc, char *argv[])
            : QCoreApplication(argc,argv)
{
    TCommParams pars = { QHostAddress("127.0.0.1"), 10001,
                         QHostAddress("127.0.0.1"), 10000};
    comm = new TCommunicator(pars, this);
    stat = new stats;

    watch = new QTime;
    watch->setHMS(0,0,0);

    //таймеры
    timer = new QTimer;
    timer->setInterval(1000/speed);

    hour = new QTimer;
    hour->setInterval(60*1000/speed);

    broke = new QTimer;

    //сигналы таймеров
    connect(timer, SIGNAL(timeout()), this, SLOT(timedSend()));
    connect(hour, SIGNAL(timeout()), this, SLOT(createIntervals()));

    connect(broke, SIGNAL(timeout()), this, SLOT(changeBroke()));

    connect(comm,SIGNAL(recieved(QByteArray)),this,SLOT(fromCommunicator(QByteArray)));
    connect(this,SIGNAL(toCommunicator(QByteArray)),comm,SLOT(send(QByteArray)));
    connect(this,SIGNAL(timerActive(QByteArray)),comm,SLOT(send(QByteArray)));

    connect(this,SIGNAL(vectorReady()),this,SLOT(createBreakInterval()));
}

void TApplication::fromCommunicator(QByteArray msg)
{
    int pos = msg.indexOf(";");
    if (pos > 0)
    {
        QByteArray arr;
        TInterfaceMessage data;
        //decode
        data.DoubleData = QString(msg.left(pos));
        data.ChrData = QString(msg.right(msg.size()-pos-1));

        if(data.DoubleData == "stop")
        {
            remTimer = timer->remainingTime();
            timer->stop();
            remHour = hour->remainingTime();
            hour->stop();
            for (unsigned long i = 0; i < busy.size(); i++)
            {
                remBusy.push_back(busy.at(i)->remainingTime());
                busy.at(i)->stop();
            }
            remBroke = broke->remainingTime();
            broke->stop();
        }
        else if (data.DoubleData == "start")
        {
            if (!begin)
            {
                if(remTimer != -1) timer->setInterval(remTimer);
                timer->start();
                if(remHour != -1) hour->setInterval(remHour);
                hour->start();
                for (unsigned long i = 0; i < busy.size(); i++)
                {
                    if( remBusy.at(i) != -1)  busy.at(i)->setInterval(remTimer);
                    busy.at(i)->start();
                }
                if(remBroke != -1) broke->setInterval(remBroke);
                broke->start();
            }
        }
        else
        {
            begin = true;
            createData(data.DoubleData);
            createIntervals();
            hour->setInterval(60*1000/speed);
            timer->setInterval(1000/speed);
            remTimer = -1;
            remHour = -1;
            remBroke = -1;
            remBusy.clear();
        }
    }
}



void TApplication::createData(QString data)
{
    QStringList a = data.split(" ");
    freeworker = a[0].toInt();
    okayWorker = a[1].toInt();
    breakAm = a[2].toInt();
    repairInt = a[3].toInt();
    busyWorker = 0;
    brokenWorkers = 0;
    onRepair = 0;

}

void TApplication::createIntervals()
{
    std::exponential_distribution<> distribution(1.0);
    breakIntervals.clear();
    breakIntervals.push_back(0);
    int breakAmTemp = breakAm * 2 + 1;
    while (breakAmTemp > breakAm * 2)
    {
        breakAmTemp = static_cast<int>(round(1 + (breakAm * 2 - 1) * distribution(generator)));
    }

    for (int i = 0; i < breakAmTemp; i++)
    {
        double r = 1 + (60 - 1) * distribution(generator);
        while(std::find(breakIntervals.begin(), breakIntervals.end(), r) != breakIntervals.end() || r == 0 || r > 60.0)
        {
            r = 1 + (60 - 1) * distribution(generator);
        }
        breakIntervals.push_back(r);
    }
    sort(breakIntervals.begin(), breakIntervals.end());

    for(int i = 0; i < breakAmTemp; i++)
    {
        breakIntervals.at(i) = breakIntervals.at(i+1) - breakIntervals.at(i);
    }
    breakIntervals.pop_back();

    stat->findStat(breakIntervals);

    if(begin)
    {
        free.clear();
        busy.clear();
        for (int i = 0; i < freeworker; i++)
        {
            QTimer *a = new QTimer;
            free.push_back(a);
            connect(free.at(i), SIGNAL(timeout()), this, SLOT(changeRepaired()));
        }
    }
    hour->setInterval(60*1000/speed);
    emit vectorReady();
}

void TApplication::createBreakInterval()
{
    broke->stop();
    if(!breakIntervals.empty())
    {
        broke->setInterval(static_cast<int>(breakIntervals.at(0)*1000/speed));
        stat->currentInt = breakIntervals.at(0);
        breakIntervals.erase(breakIntervals.begin());
        if (!begin) broke->start();
        else begin = false;
    }
}


void TApplication::changeRepaired()
{
    busy.front()->stop();
    free.push_back(busy.front());
    busy.erase(busy.begin());
    okayWorker++;
    onRepair--;
    busyWorker--;
    if(brokenWorkers == 0)
    {
        freeworker++;
    }
    else
    {
        busy.push_back(free.front());
        free.erase(free.begin());
        std::exponential_distribution<> distribution(1.0);
        double repairTemp = 1 + (2*repairInt - 1) * distribution(generator);
        busy.back()->setInterval(static_cast<int>(repairTemp*1000/speed));
        busy.back()->start();
        brokenWorkers--;
        busyWorker++;
        onRepair++;
    }
}

void TApplication::changeBroke()
{
    if(okayWorker != 0)
    {
        okayWorker--;
        if(freeworker == 0)
        {
            brokenWorkers++;
        }
        else
        {
            busy.push_back(free.front());
            free.erase(free.begin());
            std::exponential_distribution<> distribution(1.0);
            double repairTemp = 1 + (2*repairInt - 1) * distribution(generator);
            busy.back()->setInterval(static_cast<int>(repairTemp*1000/speed));
            busy.back()->start();
            freeworker--;
            busyWorker++;
            onRepair++;
        }
    }
    emit vectorReady();
}

void TApplication::timedSend()
{
    QByteArray arr;
    arr.append(watch->toString("mm:ss")+" ");
    arr.append(QString::number(freeworker)+" ");
    arr.append(QString::number(busyWorker)+" ");
    arr.append(QString::number(onRepair)+" ");
    arr.append(QString::number(brokenWorkers)+" ");
    arr.append(QString::number(okayWorker)+" ");
    int minutes = static_cast<int>(floor(stat->currentInt));
    int seconds = static_cast<int>(round(((stat->currentInt - minutes)) * 60));
    arr.append(QString::number(minutes)+"м"+QString::number(seconds)+"с"+" ");
    minutes = static_cast<int>(floor(stat->interval_average));
    seconds = static_cast<int>(round(((stat->interval_average - minutes)) * 60));
    arr.append(QString::number(minutes)+"м"+QString::number(seconds)+"с"+" ");
    minutes = static_cast<int>(floor(stat->sko));
    seconds = static_cast<int>(round(((stat->sko - minutes)) * 60));
    arr.append(QString::number(minutes)+"м"+QString::number(seconds)+"с"+" ");
    arr.append(";");
    arr.append(" ");
    *watch = watch->addSecs(1);
    timer->setInterval(1000/speed);
    emit timerActive(arr);
}



