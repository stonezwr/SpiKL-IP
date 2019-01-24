#include "def.h"
#include "synapse.h"
#include "neuron.h"
#include "simulator.h"
#include "network.h"
#include "util.h"
#include <sys/time.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <algorithm>

//#define _DUMP_READOUT_WEIGHTS

using namespace std;
double TSstrength;
extern int file[500];

Simulator::Simulator(Network * network){
    _network = network;
    _t = -1;
}

void Simulator::SetEndTime(double endTime){
    assert(endTime > 0);
    _t_end = endTime;
}

void Simulator::SetStepSize(double stepSize){
    assert(stepSize > 0);
    _t_step = stepSize;
}



void Simulator::LSMRun(long tid){
    //_network->PrintAllNeuronName();

    char filename[64];
    int info, count;
    int x,y,loss_neuron;
    int Tid;
    double temp;
    FILE * Fp = NULL;
    FILE * Foutp = NULL;
    networkmode_t networkmode;

    Timer myTimer;

    srand(0);
    _network->IndexSpeech();
    cout<<"Number of speeches: "<<_network->NumSpeech()<<endl;

    //for(int count = 0; count < _network->NumSpeech(); count++){
    //    // empty files
    //    sprintf(filename,"outputs/spikepattern%d.dat",count);
    //    InitializeFile(filename);
    //}

    if(tid == 0){
        sprintf(filename, "i_weights_info.txt");
        _network->WriteSynWeightsToFile("input", "reservoir", filename);
        sprintf(filename, "r_weights_recurrent_info.txt");
        _network->WriteSynWeightsToFile("reservoir", "reservoir", filename);
        sprintf(filename, "o_weights_info.txt");
        _network->WriteSynWeightsToFile("reservoir", "output", filename);
    }

#ifdef _RM_ZERO_RES_WEIGHT
    _network->RemoveZeroWeights("reservoir");
#endif  


    // produce transient state
    networkmode = TRANSIENTSTATE;
    _network->LSMSetNetworkMode(networkmode);
    myTimer.Start();
    _network->LSMTransientSim(networkmode, tid, "train_sample");
    myTimer.End("running transient");

    // train the readout layer
    myTimer.Start();
    networkmode = READOUT; // choose the readout supervised algorithm here!
    _network->LSMSetNetworkMode(networkmode);
    Tid = (int)tid;
    cout<<"Tid:"<<Tid<<endl;
    _network->Fold(Tid);

    for(int iii = 0; iii < NUM_ITERS; iii++){
        if(tid == 0)    cout<<"Run the iteration: "<<iii<<endl;
            // random shuffle the training samples for better generalization
        _network->ShuffleTrainingSamples();
        _network->LSMSupervisedTraining(networkmode, tid, iii);
            _network->CurrentPerformance(iii);
        }
    myTimer.End("supervised training the readout"); 
    if(tid == 0){
        sprintf(filename, "o_weights_info_trained_all.txt");
        _network->WriteSelectedSynToFile("readout", filename);
    }
}

void Simulator::InitializeFile(const char * filename){
    FILE * Foutp = fopen(filename,"w");
    assert(Foutp != NULL);
    fprintf(Foutp,"%d\t%d\n",-1,-1);
    fclose(Foutp);
}
