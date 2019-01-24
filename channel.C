#include <cstdlib>
#include <math.h>
#include <iostream>
#include <assert.h>
#include <random>
#include "channel.h"

using namespace std;
std::default_random_engine generator;

// input channel
Channel::Channel(int step_analog, int step_spikeT, int index):
    _step_analog(step_analog),
    _step_spikeT(step_spikeT),
    _index(index),
    _mode(INPUTCHANNEL)
{} 

// reservoir channel
Channel::Channel(int index):
    _index(index),
    _mode(RESERVOIRCHANNEL)
{
    _step_analog = 10;
    _step_spikeT = 1;
}

void Channel::AddAnalog(double signal){
    _analog.push_back(signal);
}

void Channel::AddSpike(int spikeT){
    int size = _spikeT.size();
    if(size > 0){
        if(spikeT <= _spikeT[size-1])
            cout<<spikeT<<"\t"<<_spikeT[size-1]<<endl;
        assert(spikeT > _spikeT[size-1]);
    }else{
        assert(spikeT > 0);
    }
    _spikeT.push_back(spikeT);
}

int Channel::FirstSpikeT(){
    if(_spikeT.empty() == true) return -1;
    _iter_spikeT = _spikeT.begin();
    return *_iter_spikeT;
}

int Channel::NextSpikeT(){
    _iter_spikeT++;
    if(_iter_spikeT == _spikeT.end()) return -1;
    else return *_iter_spikeT;
}

void Channel::BSA(){
    int length_kernel = 24;
    int length_signal = (_analog.size()*_step_analog)/_step_spikeT + 24;
    double threshold = 0.6;
    double error1, error2, temp;
    double * kernel = new double[length_kernel];
    double * signal = new double[length_signal];
    //double sig, noi;

    for(int i = 0; i < length_kernel; i++) kernel[i] = exp(-(i-double(length_kernel)/2)*(i-double(length_kernel)/2)/25);
    temp = 0;
    for(int i = 0; i < length_kernel; i++) temp += kernel[i];
    for(int i = 0; i < length_kernel; i++) kernel[i] /= temp;

    int index = 0;
    for(int i = 0; i < _analog.size(); i++)
        for(; index < (i+1)*_step_analog/_step_spikeT; index++)
            signal[index] = _analog[i]*2e3;
    for(; index < length_signal; index++) signal[index] = 0;

    int j;
    for(int i = 0; i < length_signal-24; i++){
        error1 = 0;
        error2 = 0;
        for(j = 0; j < length_kernel; j++){
            temp = signal[i+j] - kernel[j];
            error1 += (temp<0) ? -temp : temp;
            temp = signal[i+j];
            error2 += (temp<0) ? -temp : temp;
        }
        if(error1 < (error2-threshold)){
            _spikeT.push_back(i+1);
            for(j = 0; j < length_kernel; j++) signal[i+j] -= kernel[j];
        }
    }

    delete [] kernel;
    delete [] signal;
}

//* clear the channel:
void Channel::Clear(){
    // Note that this is going to invalid the iterator!!
    // So when you are using the iterator, please call FirstSpike() at first!!
    _spikeT.clear();
}


void Channel::Print(ofstream& f_out){
    assert(f_out.is_open());
    if(_spikeT.empty() == true) return;
    for(_iter_spikeT = _spikeT.begin(); _iter_spikeT != _spikeT.end(); _iter_spikeT++) 
        f_out<<_index<<"\t"<<*_iter_spikeT<<endl;
}

void Channel::Read(FILE * fp){
	char linestring[8192];
	char * token;
	int spike_t;
	int index;
	while(1){
		if(fgets(linestring,8191,fp)==NULL||linestring[0]=='\n'){
			assert(0);
		}
		token=strtok(linestring," \t\n,");
		index=atoi(token);
		if(index==-1){
			break;
		}
		if(index!=_index)
			assert(0);
		token=strtok(NULL," \t\n,");
		spike_t=atoi(token);
		AddSpike(atoi(token));
	}
}
