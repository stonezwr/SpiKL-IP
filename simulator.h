#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "network.h"
#include <vector>

class Simulator{
private:
    double _t;
    double _t_end;
    double _t_step;

    Network * _network;
public:
    Simulator(Network*);
    void SetEndTime(double);
    void SetStepSize(double);

    void LSMRun(long tid);
    void InitializeFile(const char * filename);
};

#endif

