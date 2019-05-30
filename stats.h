#ifndef STATS_H
#define STATS_H

#include <vector>
#include <cmath>
#include <numeric>


class stats
{
public:
    stats();
    double currentInt;
    double interval_average;
    double sko;
    void findStat(std::vector<double> &breakIntervals);
};

#endif // STATS_H
