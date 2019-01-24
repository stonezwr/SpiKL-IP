#include "def.h"
#include "synapse.h"
#include "neuron.h"
#include "network.h"
#include "util.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <numeric>


using namespace std;

extern int COUNTER_LEARN;

// for continuous model:
Synapse::Synapse(Neuron * pre, Neuron * post, double lsm_weight, bool fixed, double lsm_weight_limit, bool excitatory, bool liquid):
    _pre(pre),
    _post(post),
    _excitatory(excitatory),
    _fixed(fixed),
    _liquid(liquid),
    _disable(false),
    _lsm_active(false),
    _mem_pos(0),
    _mem_neg(0),
    _lsm_spike(0),
    _lsm_delay(0),
    _lsm_c(0),
    _lsm_weight(lsm_weight),
    _lsm_weight_limit(fabs(lsm_weight_limit))
{
    assert(pre && post);

    char * name = post->Name();
    if(name[0] == 'o')  _excitatory = _lsm_weight >= 0; 

    //cout<<pre->Name()<<"\t"<<post->Name()<<"\t"<<_excitatory<<"\t"<<post->IsExcitatory()<<"\t"<<_lsm_weight<<"\t"<<_lsm_weight_limit<<endl;

}


void Synapse::SetPreNeuron(Neuron * pre){
    assert(pre);
    _pre = pre;
}

void Synapse::SetPostNeuron(Neuron * post){
    assert(post);
    _post = post;
}

bool Synapse::IsReadoutSyn(){
    char * name_post = _post->Name();
    // if the post neuron is the readout/hidden neuron:
    if((name_post[0] == 'o' && name_post[6] == '_') || (name_post[0] == 'h' && name_post[6] == '_')){
        return true;
    }
    else{
        return false;
    }
}

bool Synapse::IsInputSyn(){
    char * name_pre = _pre->Name();

    if((name_pre[0] == 'i')&&(name_pre[5] == '_'))
        return true;
    else
        return false;
}


bool Synapse::IsValid(){
    assert(_pre && _post); 
    return !(_pre->LSMCheckNeuronMode(DEACTIVATED) ||
            _post->LSMCheckNeuronMode(DEACTIVATED) ||
            _disable);
}


void Synapse::LSMLearn(int t, int iteration){
    assert((_fixed==false)&&(_lsm_active==true));
    _lsm_active = false;

    int iter = iteration + 1;

    if(iteration <= 50) iter = 1;
    else if(iteration <= 100) iter = 2;
    else if(iteration <= 150) iter = 4;
    else if(iteration <= 200) iter = 8;
    else if(iteration <= 250) iter = 16;
    else if(iteration <= 300) iter = 32;
    else iter = iteration/4; 

    double weight_old = _lsm_weight;
    _lsm_c = _post->GetCalciumPre();
	int signal=_post->GetTeacherSignal();
    if(_lsm_c>LSM_CAL_MID){
        if((_lsm_c < LSM_CAL_MID+LSM_CAL_BOUND)){
            _lsm_weight += LSM_DELTA_POT/( 1 + iteration/ITER_SEARCH_CONV);
        }
    }else{
        if((_lsm_c > LSM_CAL_MID-LSM_CAL_BOUND)){ 
            _lsm_weight -= LSM_DELTA_DEP/( 1+ iteration/ITER_SEARCH_CONV);
        }
    }
    CheckReadoutWeightOutBound();
}

Neuron * Synapse::PreNeuron(){
    return _pre;
}

Neuron * Synapse::PostNeuron(){
    return _post;
}

bool Synapse::Excitatory(){
    return _excitatory;
}

bool Synapse::Fixed(){
    return _fixed;
}


//* delivery the spike response
//* set the curr_inputs_pos/curr_inputs_neg field of the post neuron
void Synapse::LSMDeliverSpike(){
    double effect = _lsm_weight;

    if(_excitatory){
        _post->IncreaseEP(effect);
        _post->IncreaseEN(effect);
    }
    else{
        _post->IncreaseIP(effect);
        _post->IncreaseIN(effect);
    }
}


void Synapse::LSMClear(){
    _lsm_active = false;

    _mem_pos = 0;
    _mem_neg = 0;    

    _lsm_spike = 0;
    _lsm_delay = 0;
    _lsm_c = 0;


}

void Synapse::LSMClearLearningSynWeights(){
    if(_fixed == true) return;
    _lsm_weight = 0;
}

//**  Add the active firing synapses (reservoir/readout synapses) into the network
//**  Add the active synapses into the network for learning if needed
//**  @param2: need to tune the synapse at the current step;   
//**  _lsm_active is only used for indication of the active learning synapses
void Synapse::LSMActivate(Network * network, bool need_learning, bool train){
    if(_lsm_active)
        cout<<_pre->Name()<<"\t"<<_post->Name()<<endl;

    assert(_lsm_active == false);
    // 1. Add the synapses for firing processing

    //network->LSMAddActiveSyn(this);

    // 2. For the readout synapses, if we are using stdp training in reservoir/input,
    //  just ignore the learning in the readout:
    //  But Make sure that we are going to train this readout synapse if not:
    if(need_learning == true && _fixed == false && train == true){
        network->LSMAddActiveLearnSyn(this);
        // 3. Mark the readout synapse active state:
        _lsm_active = true;
    }
}
inline void Synapse::CheckReadoutWeightOutBound(){
    if(_lsm_weight >= _lsm_weight_limit) _lsm_weight = _lsm_weight_limit;
    if(_lsm_weight < -_lsm_weight_limit) _lsm_weight = -_lsm_weight_limit;
}


