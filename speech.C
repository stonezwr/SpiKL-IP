#include "speech.h"
#include "channel.h"
#include <assert.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <algorithm>


using namespace std;

Speech::Speech(int cls):
    _index(-1),
    _class(cls),
    _weight(1.0)
{
    _channel_map[INPUTCHANNEL] = &_channels;
    _channel_map[RESERVOIRCHANNEL] = &_rChannels;
    _channel_map[OUTPUTCHANNEL] = &_oChannels;
}

Speech::~Speech(){
    for(vector<Channel*>::iterator iter = _channels.begin(); iter != _channels.end(); iter++) delete *iter;
    for(vector<Channel*>::iterator iter = _rChannels.begin(); iter != _rChannels.end(); iter++) delete *iter;
    for(vector<Channel*>::iterator iter = _oChannels.begin(); iter != _oChannels.end(); iter++) delete *iter;
}

Channel * Speech::AddChannel(int step_analog, int step_spikeT, int index){
    Channel * channel = new Channel(step_analog, step_spikeT, index);
    _channels.push_back(channel);
    return channel;
}

void Speech::SetNumChannel(int size, channelmode_t mode){
    assert(size >= 0);
    while(_channel_map[mode]->size() < size){
        Channel * channel = new Channel(_channel_map[mode]->size());
        channel->SetMode(mode);
        _channel_map[mode]->push_back(channel);
    }
}


Channel * Speech::GetChannel(int index, channelmode_t channelmode){
    if(index < 0 && index >= _channel_map[channelmode]->size()){
        cout<<"Invalid channel index: "<<index
            <<" seen in aquiring "<<channelmode<<" channels!\n"
            <<"Total number of channels: "<<_channel_map[channelmode]->size()
            <<endl;
        exit(EXIT_FAILURE);
    }
    return (*_channel_map[channelmode])[index]; 
}

//* clear the targeted channels:
void Speech::ClearChannel(channelmode_t channelmode){
    if(_channel_map.find(channelmode) == _channel_map.end()){
        cout<<"Invalid channel type: "<<channelmode<<endl;
        exit(EXIT_FAILURE);
    }

    for(size_t i = 0; i < _channel_map[channelmode]->size(); ++i){
        Channel * channel = (*_channel_map[channelmode])[i]; 
        assert(channel);
        channel->Clear();
    }

}

void Speech::AnalogToSpike(){
    for(int i = 0; i < _channels.size(); i++){
        assert(_channels[i]->SizeAnalog() == _channels[0]->SizeAnalog());
        _channels[i]->BSA();
    }
}

int Speech::NumChannels(channelmode_t channelmode){
    if(channelmode == INPUTCHANNEL) return _channels.size();
    else if(channelmode == RESERVOIRCHANNEL) return _rChannels.size();
    else return _oChannels.size();
}

void Speech::Info(){
    cout<<"# of input channels = "<<_channels.size()<<endl;
    for(int i = 0; i < _channels.size(); i++) cout<<_channels[i]->SizeSpikeT()<<"\t";
    cout<<endl;
    cout<<"# of reservoir channels = "<<_rChannels.size()<<endl;
    for(int i = 0; i < _rChannels.size(); i++) cout<<_rChannels[i]->SizeSpikeT()<<"\t";
    cout<<endl;
}

//* dump the spike times for each channel in the speech
void Speech::PrintSpikesPerChannels(const vector<Channel*>& channels, const string & filename){
    ofstream f_out(filename.c_str());
    if(!f_out.is_open()){
        cout<<"Cannot open file : "<<filename<<" for dumping the spikes!"<<endl;
        assert(f_out.is_open());
    }
    f_out<<"-1\t-1"<<endl;
    for(int i = 0; i < channels.size(); i++){ 
        channels[i]->Print(f_out);
    }
    f_out<<"-1\t-1"<<endl;
    f_out.close();
}

void Speech::PrintSpikes(int info, const string& channel_name){
    if(channel_name == "input" || channel_name == "all"){
        string input = "spikes/Input_Response/"+ to_string(_class)+"/input_spikes_" + to_string(info) + "_" + to_string(_class) +".dat";
        PrintSpikesPerChannels(_channels, input);
    }
    
    if(channel_name == "reservoir" || channel_name == "all"){
        string reservoir = "spikes/Reservoir_Response/"+to_string(_class)+"/reservoir_spikes_" + to_string(info) + "_" + to_string(_class)+ ".dat";
        PrintSpikesPerChannels(_rChannels, reservoir);
    }
}

//* this function read each channel and output the firing frequency into a matrix
//* defined by the output stream: f_out.
//* there are two type of channels to be considered.
//* @return: the associated class label.
int Speech::PrintSpikeFreq(const char * type, ofstream & f_out){
    assert(type);
    bool f;
    if(strcmp(type, "input") == 0) f = false;
    else if(strcmp(type, "reservoir") == 0) f = true;
    else assert(0);

    if(!f){
        SpikeFreq(f_out, _channels);	
    }
    else{
        SpikeFreq(f_out, _rChannels);
    }
    return _class;
}

//* Print the spike rate to the target file:
void Speech::SpikeFreq(ofstream & f_out, const vector<Channel*> & channels){
    // find out the stop time for each speech:
    int stop_t = EndTime();

    for(int i = 0; i < channels.size(); i++){ 
        //f_out<<(double)channels[i]->SizeSpikeT()/stop_t<<"\t";
        f_out<<channels[i]->SizeSpikeT()<<"\t";
    }
    f_out<<endl;
}

//* find out the stop time for each speech:
int Speech::EndTime(){
    int stop_t = INT_MIN;
    for(int i = 0; i < _channels.size(); i++){
        assert(_channels[i]);
        stop_t = max(stop_t, _channels[i]->LastSpikeT());
    }
    if(stop_t == INT_MIN){
        cout<<"The speech:  "<<_index<<" contains "<<_channels.size()<<" input channels."<<endl;
        assert(stop_t != INT_MIN);
    }
    return stop_t;
}

//* Collect the firing fq into a vector:
void Speech::CollectFreq(synapsetype_t syn_t, vector<double>& fs, int end_t){
    vector<Channel*> tmp;
    vector<Channel*> & channels = syn_t == INPUT_SYN ? _channels : 
        syn_t == RESERVOIR_SYN ? _rChannels :
        syn_t == READOUT_SYN ? _oChannels : tmp;

    if(channels.empty()){
        cout<<"In Speech::CollectFreq(), you are reading: "<<syn_t<<" channels.\n "
            <<"Is the synapse type undefined or the channels are empty ?"<<endl;
        assert(!channels.empty());
    }
    if(end_t == 0){
        cout<<"The ending time of the speech is zero!!"<<endl;
        assert(end_t != 0);
    }
    for(size_t i = 0; i < channels.size(); ++i){
        assert(channels[i]);
        fs.push_back(((double)(channels[i]->SizeSpikeT()))/end_t);
    }
}


