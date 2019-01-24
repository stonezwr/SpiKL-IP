#ifndef CHANNEL_H
#define CHANNEL_H

#include <vector>
#include "def.h"
#include <cstdio>
#include <cstring>
#include <fstream>

class Channel{
private:
    std::vector<double> _analog;
    std::vector<int> _spikeT;
    std::vector<int>::iterator _iter_spikeT;
    int _step_analog;
    int _step_spikeT;
    int _index;
    channelmode_t _mode;
public:
    Channel(int step_analog, int step_spikeT, int index);
    Channel(int index);
    void AddAnalog(double);
    void AddSpike(int);
    int FirstSpikeT();
    int NextSpikeT();
    void BSA();
    int SizeAnalog(){return _analog.size();}
    int SizeSpikeT(){return _spikeT.size();}
    int LastSpikeT(){return _spikeT.empty() ? 0 : _spikeT.back();}
    void SetMode(channelmode_t channelmode){_mode = channelmode;}
    void GetAllSpikes(std::vector<int>& v){v = _spikeT;}
    channelmode_t Mode(){return _mode;}
    void Clear();
    void Print(std::ofstream& f_out);
    void Read(FILE * fp);
};

#endif


