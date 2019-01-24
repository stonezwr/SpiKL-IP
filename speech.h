#ifndef SPEECH
#define SPEECH

#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include "def.h"

class Channel;

class Speech{
private:
    std::vector<Channel*> _channels;
    std::vector<Channel*> _rChannels;
    std::vector<Channel*> _oChannels;
    std::unordered_map<int, std::vector<Channel*>* > _channel_map;
    int _class;
    int _index;
    int _num_input_channels;
    int _input_connections;
    double _weight; // boosting weight associated with each sample
    int _file_index;
public:
    Speech(int);
    ~Speech();
    void SetNumChannel(int size, channelmode_t mode);
    Channel * AddChannel(int step_analog, int step_spikeT, int index);
    Channel * GetChannel(int,channelmode_t);
    void ClearChannel(channelmode_t channelmode);
    void AnalogToSpike();
    int NumChannels(channelmode_t);
    int Class(){return _class;}
    void SetIndex(int index) {_index = index;}
    int Index() {return _index;}
    double Weight(){return _weight;}
    void SetWeight(double w){_weight = w;}
    void SetFileIndex(int i){_file_index=i;}
    int GetFileIndex(){return _file_index;}

    void AddInputNumInfo(int num_inputs){ _num_input_channels = num_inputs;}
    void AddInputConnectionInfo(int input_connections){ _input_connections = input_connections;}
    int NumInputs(){return _num_input_channels;}
    int NumConnections(){return _input_connections;}

    void Info();
    void PrintSpikesPerChannels(const std::vector<Channel*>& channels, const std::string& filename);
    void PrintSpikes(int info, const std::string& channel_name);
    int PrintSpikeFreq(const char * type, std::ofstream & f_out);
    void SpikeFreq(std::ofstream & f_out, const std::vector<Channel*> & channels);
    int  EndTime();
    void CollectFreq(synapsetype_t syn_t, std::vector<double>& fs, int end_t);
};
#endif


