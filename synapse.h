#ifndef SYNAPSE_H
#define SYNAPSE_H

#include <cmath>
#include <stdio.h>
#include <iomanip>
#include <fstream>
#include <vector>
#include <assert.h>
#include <utility>
#include "def.h"


/** unit system defined for digital lsm **/
extern const int one;

class Neuron;
class Network;


// based on the model of an memrister synapse
class Synapse{
private:
    Neuron * _pre;
    Neuron * _post;
    bool _excitatory;
    bool _fixed;
    bool _liquid;

    // _disable is the switch to disable this synapse:
    bool _disable;

    // _lsm_active is to indicate whether or not 
    // there is readout synapse to be trained
    bool _lsm_active;

    double _mem_pos;  // those two variables are current not used
    double _mem_neg;

    int _lsm_spike;

    int _lsm_delay;

    double _lsm_c;
    double _lsm_weight;

    double _lsm_weight_limit;

    std::vector<std::pair<int, double> > _firing_stamp;

    /* ONLY FOR PRINT PURPOSE */
    /* keep track of the weights and firing activity */
    std::vector<int> _t_pre_collection;
    std::vector<int> _t_post_collection;
    std::vector<int> _simulation_time;
    std::vector<double> _weights_collection;


public:
    Synapse(Neuron*,Neuron*,double,bool,double,bool,bool); // only for non-digital liquid state machine
    Synapse(Neuron*,Neuron*,int,bool,int,bool,bool); // only for digital liquid state machine (including initializing synapse in liquid)

    void DisableStatus(bool disable){ _disable = disable; }
    bool DisableStatus(){ return _disable; }

    //* Get the _lsm_active of the synapse, which is the flag indicating whether or not the synapse is activated:
    bool GetActiveStatus(){return _lsm_active;}

    //* Set the _lsm_active of the synapse.  
    void SetActiveStatus(bool status){_lsm_active = status;}

    //* Determine whether or not the synapse is the readout synapse:
    bool IsReadoutSyn();
    //* Determine whether or not the synapse is the input synapse:
    bool IsInputSyn();
    //* Determine whether or not the synapse is valid (connected to no deactivated neurons)
    bool IsValid();

    //* Get the synaptic weights:
    double Weight(){ return _lsm_weight;}
    //* Set the synaptic weights:
    void Weight(double weight){_lsm_weight = weight;}

    bool IsWeightZero(){
        return _lsm_weight == 0;
    }

    Neuron * PreNeuron();
    Neuron * PostNeuron();
    void SetPreNeuron(Neuron * pre);
    void SetPostNeuron(Neuron * post);

    void LSMPrint(FILE*);
    bool Excitatory();
    bool Fixed();
    void LSMDeliverSpike(); 
    void LSMNextTimeStep();
    void LSMFiringStamp(int time);
    void LSMClear();
    void LSMClearLearningSynWeights();
    void LSMLearn(int t, int iteration);
    void LSMActivate(Network * network, bool stdp_flag, bool train);
    void LSMDeactivate(){ _lsm_active = false;}

    // determine whether or not this synapse is in the liquid?
    bool IsLiquidSyn(){ return _liquid;};

    void CheckReadoutWeightOutBound();


    /** Definition for inline functions: **/
    inline 
    void LSMStaticCurrent(int * pos, double * value){
        *pos = _excitatory ? 1 : -1;
        *value = _lsm_spike == 0 ? 0 : _lsm_weight;
        _lsm_spike = 0;
    }

};


#endif

