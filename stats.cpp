#include "stats.h"
typedef unsigned long ulong;
stats::stats()
{

}

void stats::findStat(std::vector<double> &workersIntervals)
{
    // среднее значение интервала между проходами рабочих
    interval_average = double(accumulate(workersIntervals.begin(), workersIntervals.end(), 0)) / workersIntervals.size();

    //среднеквадратическое отклонение
    sko = 0;
        for (ulong i = 0; i < workersIntervals.size(); i++)
    {
        sko += pow((workersIntervals.at(i) - interval_average),2);
    }

    sko = sqrt(sko / workersIntervals.size());
}
