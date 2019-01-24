#include "def.h"
#include "neuron.h"
#include "synapse.h"
#include "speech.h"
#include "network.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <climits>
#include <cmath>
#include <string>
#include <unistd.h>

#define _PRINT_SPIKE_COUNT

using namespace std;

extern double Rate;
extern int file[500];
int COUNTER_LEARN = 0;

Network::Network():
_readout_correct(vector<int>(NUM_ITERS, 0)),
_readout_wrong(vector<int>(NUM_ITERS, 0)),
_readout_even(vector<int>(NUM_ITERS, 0)),
_readout_correct_breakdown(vector<vector<int> >(NUM_ITERS, vector<int>(CLS, 0))),
_readout_test_error(vector<double>(NUM_ITERS, 0.0)),
_sp(NULL),
_fold(0),
_fold_ind(-1),
_lsm_input_layer(NULL),
_lsm_reservoir_layer(NULL),
_lsm_output_layer(NULL),
_lsm_input(0),
_lsm_reservoir(0),
_lsm_t(0),
_network_mode(VOID)
{
}

Network::~Network(){
    for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
        delete (*iter);
    for(auto & p : _groupNeurons)
        delete (p.second);
    for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++)
        delete (*iter);
    for(list<Synapse*>::iterator iter = _synapses.begin(); iter != _synapses.end();++iter)
        delete (*iter);
    for(vector<Speech*>::iterator iter = _speeches.begin(); iter != _speeches.end();++iter)
        delete (*iter);
}

bool Network::CheckExistence(char * name){
    for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
        if(strcmp((*iter)->Name(),name) == 0) return true;

    if(_groupNeurons.find(string(name)) != _groupNeurons.end()) return true;

    return false;
}

void Network::AddNeuron(char * name, bool excitatory, double v_mem){
    for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
        assert(strcmp((*iter)->Name(),name) != 0);

    assert(_groupNeurons.find(string(name)) == _groupNeurons.end());

    Neuron * neuron = new Neuron(name, excitatory, this, v_mem);
    _individualNeurons.push_back(neuron);
    _allNeurons.push_back(neuron);
    if(excitatory == true){
        _allExcitatoryNeurons.push_back(neuron);
        neuron->SetExcitatory();
    }else _allInhibitoryNeurons.push_back(neuron);
}

void Network::AddNeuronGroup(char * name, int num, bool excitatory, double v_mem){
    assert(num >= 1);
    for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
        assert(strcmp((*iter)->Name(),name) != 0);

    assert(_groupNeurons.find(string(name)) == _groupNeurons.end());

    NeuronGroup * neuronGroup = new NeuronGroup(name, num, this, excitatory, v_mem);
    _groupNeurons[string(name)] = neuronGroup;

    string name_str(name);
    if(name_str.substr(0, 6) == "hidden")   _hidden_layer_names.push_back(name_str);
   
    for(Neuron * neuron = neuronGroup->First(); neuron != NULL; neuron = neuronGroup->Next()){
        _allNeurons.push_back(neuron);
        if(strcmp(name,"input") == 0)
            _inputNeurons.push_back(neuron);
        if(strcmp(name,"output") == 0)
            _outputNeurons.push_back(neuron);
    }
    if(excitatory == true) for(Neuron * neuron = neuronGroup->First(); neuron != NULL; neuron = neuronGroup->Next()){
        _allExcitatoryNeurons.push_back(neuron);
        neuron->SetExcitatory();
    }
    else for(Neuron * neuron = neuronGroup->First(); neuron != NULL; neuron = neuronGroup->Next()) _allInhibitoryNeurons.push_back(neuron);
}

//* construct the reservoir given 3 dimensional grid information
void Network::LSMAddNeuronGroup(char * name, int dim1, int dim2, int dim3){
    assert((dim1>0)&&(dim2>0)&&(dim3>0));
    for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
        assert(strcmp((*iter)->Name(),name) != 0);

    assert(_groupNeurons.find(string(name)) == _groupNeurons.end());

    NeuronGroup * neuronGroup = new NeuronGroup(name, dim1, dim2, dim3, this);

    _groupNeurons[string(name)] = neuronGroup;

    for(Neuron * neuron = neuronGroup->First(); neuron != NULL; neuron = neuronGroup->Next()){
        _allNeurons.push_back(neuron);
        if(neuron->IsExcitatory()) _allExcitatoryNeurons.push_back(neuron);
        else _allInhibitoryNeurons.push_back(neuron);
    }
}

//* construct the brain like topology of the neuron group. Not used now!
void Network::LSMAddNeuronGroup(char * name, char * path_info_neuron, char * path_info_synapse){
    for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
        assert(strcmp((*iter)->Name(),name) != 0);
    
    assert(_groupNeurons.find(string(name)) == _groupNeurons.end());

    NeuronGroup * neuronGroup = new NeuronGroup(name, path_info_neuron, path_info_synapse,this); //This function helps to generate the reservoir with brain-like connectivity
    _groupNeurons[string(name)] = neuronGroup;

    for(Neuron * neuron = neuronGroup->First(); neuron != NULL; neuron = neuronGroup->Next()){
        _allNeurons.push_back(neuron);
        if(neuron->IsExcitatory()) _allExcitatoryNeurons.push_back(neuron);
        else _allInhibitoryNeurons.push_back(neuron);
    }
}


// Please find the fuction to to add the synapses for LSM in the header file: network.h. 
//Function: template<T>  void Network::LSMAddSynapse(Neuron * pre, Neuron * post, T weight, bool fixed, T weight_limit,bool liquid)
// This function is written as a template function.


void Network::LSMAddSynapse(Neuron * pre, NeuronGroup * post, int npost, int value, int random, bool fixed){
    Neuron * neuron;
    double factor;
    double weight_limit;
    bool liquid = false;
    int D_weight_limit;
    if(fixed == true){
        /***** this is for the case of input synapses: *****/
        weight_limit = value;
        D_weight_limit = value;
    }else{
        weight_limit = value;
        D_weight_limit = value;
    }

    if(npost < 0){
        assert(npost == -1);
        for(neuron = post->First(); neuron != NULL; neuron = post->Next()){
            if(random == 2){
                /*** This is for the case of continuous readout weights: ***/
                /*** Remember to change the netlist to                   ***/
                /*** lsmsynapse reservoir output -1 -1 8 2, 2 is random  ***/
                /*** The initial weights are in W_max*(-1 to 1)          ***/
                factor = rand()%100000;
                factor = factor*2/99999-1;
            }else if(random == 0) factor = 0;
            else assert(0);
            double weight = value*factor/weight_limit;
            // set the sign of the readout weight the same as pre-neuron E/I might be hampful for the performance
            // if((!pre->IsExcitatory() && weight > 0) ||(pre->IsExcitatory() && weight < 0))  weight = -1*weight; 
            //if(weight > 0)  weight = weight*0.5;
            LSMAddSynapse(pre,neuron,weight,fixed,weight_limit,liquid);
        }
    }else{
        /*** This is for adding input synapses!! ***/
        assert(random == 1);
        for(int i = 0; i < npost; i++){
            neuron = post->Order(rand()%post->Size());
            factor = rand()%2;
            factor = factor*2-1;
            LSMAddSynapse(pre,neuron,value*factor,fixed,weight_limit,liquid);
        }
    }
}

//* add the lateral inhibition inside the layer
void Network::LSMAddSynapse(NeuronGroup * pre, int npre, int npost, int value, int random, bool fixed){
    assert(npre == -1 && npost == -1);
    assert(fixed);
    assert(value > 0);
    pre->SetLateral();
    pre->SetLateralWeight(double(value));
    for(Neuron * pre_neuron = pre->First(); pre_neuron != NULL; pre_neuron = pre->Next()){
        for(int i = 0; i < pre->Size(); ++i){
            Neuron * post_neuron = pre->Order(i);
            if(pre_neuron == post_neuron)   continue;
            // weight_value, fixed, weight_limit, is_in_liquid
            LSMAddSynapse(pre_neuron, post_neuron, double(-value), fixed, double(value), false);
        }
    }
}

void Network::LSMAddSynapse(NeuronGroup * pre, NeuronGroup * post, int npre, int npost, int value, int random, bool fixed){
    assert((npre == 1)||(npre == -1));
    for(Neuron * neuron = pre->First(); neuron != NULL; neuron = pre->Next()){
        LSMAddSynapse(neuron,post,npost,value, random, fixed);
    }
    if(pre->GetBiasNeuron() != NULL){
        BiasNeuron * b_neuron = pre->GetBiasNeuron();
        LSMAddSynapse(b_neuron, post, npost, value, 0, fixed);
    }
}

void Network::LSMAddSynapse(char * pre, char * post, int npre, int npost, int value, int random, bool fixed){
    NeuronGroup * preNeuronGroup = SearchForNeuronGroup(pre);
    NeuronGroup * postNeuronGroup = SearchForNeuronGroup(post);
    assert((preNeuronGroup != NULL)&&(postNeuronGroup != NULL));
    
    // add synapses
    if(strcmp(pre, post) != 0) // between layer connections
        LSMAddSynapse(preNeuronGroup, postNeuronGroup, npre, npost, value, random, fixed);
    else{ // laterial inhibition
        assert(preNeuronGroup == postNeuronGroup);
        LSMAddSynapse(preNeuronGroup, npre, npost, value, random, fixed);
    } 
}


void Network::PrintSize(){
    cout<<"Total Number of neurons: "<<_allNeurons.size()<<endl;
    cout<<"Total Number of synapses: "<<_synapses.size()<<endl;
    int numExitatory = 0;
    int numLearning = 0;
    for(list<Synapse*>::iterator iter = _synapses.begin(); iter != _synapses.end(); iter++){
        if((*iter)->Excitatory()==true) numExitatory++;
        if((*iter)->Fixed()==false) numLearning++;
    }
    cout<<"\tNumber of excitatory synapses: "<<numExitatory<<endl;
    cout<<"\tNumber of learning synapses: "<<numLearning<<endl;
}

int Network::LSMSizeAllNeurons(){
    return _allNeurons.size();
}

//****************************************************************************************
//
// This is a function wrapper for running the transient simulation
// @param2: is set to "train_sample", samples for training and cv, 

//****************************************************************************************
void Network::LSMTransientSim(networkmode_t networkmode, int  tid, const string sample_type){
    int count = 0;
	int IP_iter;
#ifdef	SPIKL_IP
	IP_iter=5;
#else
	IP_iter=1;
#endif
    LSMClearSignals();
    int info = this->LoadFirstSpeech(false, networkmode, sample_type);
    while(info != -1){
        count++;
		int iter_count=0;
		while(iter_count!=IP_iter){
			iter_count++;
			if(iter_count==IP_iter)
				SearchForNeuronGroup("reservoir")->SetGenerateTransient(true);
			else
				SearchForNeuronGroup("reservoir")->SetGenerateTransient(false);
				
			int time = 0, end_time = this->SpeechEndTime();
		
			while(!LSMEndOfSpeech(networkmode, end_time)){
				LSMNextTimeStep(++time,false,1, end_time, NULL);
			}
			if(iter_count==IP_iter&&tid == 0){
					SpeechPrint(info, "reservoir"); // dump the reservoir response for quick simulation
			}
			LSMClearSignalsIP();
			if(iter_count!=IP_iter)
				info=LoadSpeechAgain(false, networkmode, sample_type);
		}
		LSMClearSignals();
		info = LoadNextSpeech(false, networkmode, sample_type);
    }

}


//****************************************************************************************
//
// This is a function wrapper for training the readout synapses supervisedly! 
// Two approaches: 1. Yong's old rule.
// Note that the readout training is implemented through changing network stat.
// @param3: the # of iterations
//****************************************************************************************
void Network::LSMSupervisedTraining(networkmode_t networkmode, int tid, int iteration){
    int info = -1;
    int count = 0;
    vector<pair<int, int> > correct, wrong, even;
    vector<pair<Speech*, bool> > predictions; // the predictions for each speech
    FILE * Foutp = NULL;
    char filename[64];
    vector<double> each_sample_errors;    

    // 1. Clear the signals
    LSMClearSignals();
   
    // 2. Load the first speech:
    info = LoadFirstSpeechTrainCV(networkmode);
    double cost = 0;
    // 3. Load the speech for training
    while(info != -1){
        count++;
        int time = 0, end_time = this->SpeechEndTime();
        Foutp = NULL;
        while(!LSMEndOfSpeech(networkmode, end_time)){
            LSMNextTimeStep(++time, true, iteration, end_time, NULL); 
        }

        LSMClearSignals();

        info = LoadNextSpeechTrainCV(networkmode);
    }
    LSMClearSignals();

    // 4. Load the speech for testing
    info = LoadFirstSpeechTestCV(networkmode);
    count = 0;
    while(info != -1){
        Foutp = NULL;
        count++;
        int time = 0, end_time = this->SpeechEndTime();
        while(!LSMEndOfSpeech(networkmode, end_time)){
            LSMNextTimeStep(++time, false, 1, end_time, Foutp);
        }

        ReadoutJudge(correct, wrong, even); // judge the readout output here

        LSMClearSignals();

        info = LoadNextSpeechTestCV(networkmode);
    }
    // 5. Push the recognition result:
    LSMPushResults(correct, wrong, even, iteration);

}


//* next simulation step; the last parameter should be the ptr to reservoir or NULL.
void Network::LSMNextTimeStep(int t, bool train,int iteration,int end_time, FILE * Foutp){
    _lsm_t = t;
	vector<int> state; 

    _lsmActiveSyns.clear();

    // Please remember that the neuron firing activity will change the list: 
    // _lsmActiveSyns, _lsmActiveSTDPLearnSyns, and _lsmActiveLearnSyns:
    for(Neuron * neuron : _allNeurons){
		neuron->LSMNextTimeStep(t, Foutp, train, end_time);
	}

    // Update the previous delta effect with the current delta effect
    for(Neuron * neuron : _allNeurons)  neuron->UpdateDeltaEffect();
 
    if(train == true && _network_mode == READOUT){
        for(Synapse * synapse : _lsmActiveLearnSyns)    
			synapse->LSMLearn(t, iteration);
        _lsmActiveLearnSyns.clear();
    }
}


Neuron * Network::SearchForNeuron(const char * name){
    for(list<Neuron*>::iterator iter = _individualNeurons.begin(); iter != _individualNeurons.end(); iter++)
        if(strcmp((*iter)->Name(),name) == 0){
            return (*iter);
        }

    return NULL;
}

Neuron * Network::SearchForNeuron(const char * group_name, const char * index){
    int ind = atoi(index);    

    NeuronGroup * group = SearchForNeuronGroup(group_name);
    assert(group); 
    
    Neuron * neuron = group->Order(ind);
    assert(neuron);

    return neuron;
}


NeuronGroup * Network::SearchForNeuronGroup(const char * name){
    string group_name(name);
    if(_groupNeurons.find(group_name) == _groupNeurons.end()){
        cout<<"Warning::Search for neuron group: "<<group_name<<" not found!"<<endl;
        return NULL;
    }
    return _groupNeurons[group_name];
}

// inquiry to know the ending time of the current speech being loaded:
int Network::SpeechEndTime(){
    if(_sp == NULL){
        cout<<"In Network::EndSpeechTime(), find out the attached _sp in network"
            <<" is valid!!"<< endl;
        assert(_sp != NULL);
    }
    return _sp->EndTime();
}

void Network::AddSpeech(Speech * speech){
    _speeches.push_back(speech);
}

//* function wrapper
//  1. Given a iter pointed to speech, load this speech to both input & reservoir layer.
//  2. No need to load the channels of speech to the reservoir during STDP training.
//  3. Alway need to set the neuron mode
void Network::LoadSpeeches(Speech      * sp,
        neuronmode_t neuronmode_input,
        neuronmode_t neuronmode_reservoir,
        neuronmode_t neuronmode_readout,
        bool train
        )
{
    // assign the speech ptr here!
    assert(sp);
    _sp = sp;

    assert(_lsm_input_layer);
    _lsm_input_layer->LSMLoadSpeech(sp,&_lsm_input,neuronmode_input,INPUTCHANNEL);

    assert(_lsm_reservoir_layer);
    _lsm_reservoir_layer->LSMLoadSpeech(sp,&_lsm_reservoir,neuronmode_reservoir,RESERVOIRCHANNEL);
    
    // just set the neuron mode of the output neurons
    assert(_lsm_output_layer);
    _lsm_output_layer->LSMSetNeurons(neuronmode_readout);

    // if train the readout
    if(train == true){
        if(neuronmode_readout == NORMAL){ // original Yong's rule
            _lsm_output_layer->LSMSetTeacherSignal(sp->Class());
        }     
        else	  
			assert(0);
    }
}


//* function wrapper for removing speeches in the network 
void Network::LSMNetworkRemoveSpeech(){
    _sp = NULL;
    assert(_lsm_input_layer || _lsm_output_layer);

    if(_lsm_input_layer)
        _lsm_input_layer->LSMRemoveSpeech();
    _lsm_input_layer = NULL;

    if(_lsm_output_layer)
        _lsm_output_layer->LSMRemoveSpeech();
    _lsm_output_layer = NULL;

    if(_lsm_reservoir_layer == NULL)
        _lsm_reservoir_layer->LSMRemoveSpeech();

    _lsm_reservoir_layer = NULL;
}

//* function wrapper for load the input, reservoir and output layers/neurongroups:
void Network::LSMLoadLayers(){
    assert(_lsm_input_layer == NULL);
    _lsm_input_layer = SearchForNeuronGroup("input");
    assert(_lsm_input_layer != NULL);

    assert(_lsm_reservoir_layer == NULL);
    _lsm_reservoir_layer = SearchForNeuronGroup("reservoir");
    assert(_lsm_reservoir_layer != NULL);

    _lsm_output_layer = SearchForNeuronGroup("output");
    assert(_lsm_output_layer != NULL);
}

/************************************************************************
 * The function implement the mapping: networkmode-> neuron modes
 * Implement the different neuron mode here for STDP/Supervised Training
 ***********************************************************************/
void Network::DetermineNetworkNeuronMode(const networkmode_t & networkmode, neuronmode_t & neuronmode_input, neuronmode_t & neuronmode_reservoir, neuronmode_t & neuronmode_readout){
    if(networkmode == TRANSIENTSTATE){
        neuronmode_input = READCHANNEL;
        neuronmode_reservoir = WRITECHANNEL;
        neuronmode_readout = DEACTIVATED;
    }else if(networkmode == READOUT){
        neuronmode_input = DEACTIVATED;
        neuronmode_reservoir = READCHANNEL;
        neuronmode_readout = NORMAL;
    }else{
        cout<<"Unrecognized network mode!"<<endl;
        exit(EXIT_FAILURE);
    }

}

int Network::LoadFirstSpeech(bool train, networkmode_t networkmode, const string sample_type){
    LSMLoadLayers();

    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout); 
    auto sp_end_ptr = _speeches.end();
    _sp_iter = _speeches.begin();

    if(_sp_iter != sp_end_ptr){
        LoadSpeeches(*_sp_iter, neuronmode_input, neuronmode_reservoir, neuronmode_readout, train);
        return (*_sp_iter)->Index();
    }else{
        _sp = NULL;
        return -1;
    }
}

int Network::LoadNextSpeech(bool train, networkmode_t networkmode, const string sample_type){
    assert(_lsm_input_layer != NULL);
    assert(_lsm_reservoir_layer != NULL);
    assert(_lsm_output_layer != NULL);

    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout);

    if(train == true) _lsm_output_layer->LSMRemoveTeacherSignal((*_sp_iter)->Class());
    _sp_iter++;

    auto sp_end_ptr = _speeches.end();
    
    if(_sp_iter != sp_end_ptr){
        LoadSpeeches(*_sp_iter, neuronmode_input, neuronmode_reservoir, neuronmode_readout, train);
        return (*_sp_iter)->Index();
    }else{
        LSMNetworkRemoveSpeech();
        return -1;
    }
}

int Network::LoadSpeechAgain(bool train, networkmode_t networkmode, const string sample_type){
    assert(_lsm_input_layer != NULL);
    assert(_lsm_reservoir_layer != NULL);
    assert(_lsm_output_layer != NULL);

    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout);

    if(train == true) _lsm_output_layer->LSMRemoveTeacherSignal((*_sp_iter)->Class());

    LoadSpeeches(*_sp_iter, neuronmode_input, neuronmode_reservoir, neuronmode_readout, train);
    return (*_sp_iter)->Index();
}


int Network::LoadFirstSpeechTrainCV(networkmode_t networkmode){
    assert((_fold_ind>=0)&&(_fold_ind<_fold));

    LSMLoadLayers();
    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout);


    if(_fold_ind == 0) _train_fold_ind = 1;
    else _train_fold_ind = 0;
    assert(!_CVspeeches.empty());
    _cv_train_sp_iter = _CVspeeches[_train_fold_ind].begin();
    if(_cv_train_sp_iter != _CVspeeches[_train_fold_ind].end()){
        LoadSpeeches(*_cv_train_sp_iter, neuronmode_input, neuronmode_reservoir, neuronmode_readout,true); 
        return (*_cv_train_sp_iter)->Index();
    }else{
        cout<<"Warning, no training speech is specified!!"<<endl;
        _sp = NULL;
        return -1;
    }
}

int Network::LoadNextSpeechTrainCV(networkmode_t networkmode){
    assert(_lsm_input_layer != NULL);
    assert(_lsm_reservoir_layer != NULL);
    assert(_lsm_output_layer != NULL);

    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout); 

    _lsm_output_layer->LSMRemoveTeacherSignal((*_cv_train_sp_iter)->Class());
    _cv_train_sp_iter++;
    assert(!_CVspeeches.empty());
    if(_cv_train_sp_iter != _CVspeeches[_train_fold_ind].end()){
        LoadSpeeches(*_cv_train_sp_iter, neuronmode_input, neuronmode_reservoir, neuronmode_readout,true);
        return (*_cv_train_sp_iter)->Index();
    }else{
        ++_train_fold_ind;
        if(_train_fold_ind == _fold_ind)  ++_train_fold_ind;

        if(_train_fold_ind < _fold){
            _cv_train_sp_iter = _CVspeeches[_train_fold_ind].begin();
            if(_cv_train_sp_iter != _CVspeeches[_train_fold_ind].end()){
                LoadSpeeches(*_cv_train_sp_iter, neuronmode_input, neuronmode_reservoir,neuronmode_readout, true);
                return (*_cv_train_sp_iter)->Index();
            }else{
                assert(0);
                return -1;
            }
        }else{
            LSMNetworkRemoveSpeech();
            return -1;
        }
    }
}

int Network::LoadFirstSpeechTestCV(networkmode_t networkmode){
    assert((_fold_ind>=0)&&(_fold_ind<_fold));

    LSMLoadLayers();
    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout); 


    assert(!_CVspeeches.empty());
    _cv_test_sp_iter = _CVspeeches[_fold_ind].begin();
    if(_cv_test_sp_iter != _CVspeeches[_fold_ind].end()){
        LoadSpeeches(*_cv_test_sp_iter, neuronmode_input, neuronmode_reservoir, neuronmode_readout, false);
        return (*_cv_test_sp_iter)->Index();
    }else{
        _sp = NULL;
        return -1;
    }
}

int Network::LoadNextSpeechTestCV(networkmode_t networkmode){
    assert(_lsm_input_layer != NULL);
    assert(_lsm_reservoir_layer != NULL);
    assert(_lsm_output_layer != NULL);

    neuronmode_t neuronmode_input = NORMAL, neuronmode_reservoir = NORMAL, neuronmode_readout = NORMAL;
    // determine the neuron mode by the network mode:
    DetermineNetworkNeuronMode(networkmode, neuronmode_input, neuronmode_reservoir, neuronmode_readout);

    _cv_test_sp_iter++;
    assert(!_CVspeeches.empty());
    if(_cv_test_sp_iter != _CVspeeches[_fold_ind].end()){
        LoadSpeeches(*_cv_test_sp_iter, neuronmode_input, neuronmode_reservoir, neuronmode_readout, false);
        return (*_cv_test_sp_iter)->Index();
    }else{
        LSMNetworkRemoveSpeech();
        return -1;
    }
}


void Network::PrintAllNeuronName(){
    for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++) cout<<(*iter)->Name()<<endl;
}

//* count the number of speech from each class
vector<int> Network::NumEachSpeech(){
    vector<int> res(CLS, 0);
    for(auto & sp : _speeches){
        assert(sp && sp->Class() < CLS);
        res[sp->Class()]++;
    }
    return res;
}

void Network::AnalogToSpike(){
    int counter = 0;
    for(vector<Speech*>::iterator iter = _speeches.begin(); iter != _speeches.end(); iter++){
        //    cout<<"Preprocessing speech "<<counter++<<"..."<<endl;
        counter++;
        (*iter)->AnalogToSpike();
    }
}


void Network::LSMClearSignals(){
    for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++) (*iter)->LSMClear();
    for(list<Synapse*>::iterator iter = _synapses.begin(); iter != _synapses.end(); iter++) (*iter)->LSMClear();
    // Remember to clear the list to store the synapses:      
    _lsmActiveLearnSyns.clear();                              
    _lsmActiveSyns.clear();
    _sp = NULL;
}
void Network::LSMClearSignalsIP(){
    for(list<Neuron*>::iterator iter = _allNeurons.begin(); iter != _allNeurons.end(); iter++) (*iter)->LSMClearIP();
    for(list<Synapse*>::iterator iter = _synapses.begin(); iter != _synapses.end(); iter++) (*iter)->LSMClear();
    // Remember to clear the list to store the synapses:      
    _lsmActiveLearnSyns.clear();                              
    _lsmActiveSyns.clear();
    _sp = NULL;
}

void Network::LSMClearWeights(){
    for(list<Synapse*>::iterator iter = _synapses.begin(); iter != _synapses.end(); iter++) (*iter)->LSMClearLearningSynWeights();
}
bool Network::LSMEndOfSpeech(networkmode_t networkmode, int end_time){

    if((networkmode == TRANSIENTSTATE)&&(_lsm_input>0)){
        return false;
    }
    if((networkmode != VOID)&&(_lsm_reservoir > 0 || _lsm_input > 0)){
        return false;
    }
    return true;
    
}

void Network::SpeechInfo(){
    if(_sp_iter != _speeches.end())
        (*_sp_iter)->Info();
}

void Network::SpeechPrint(int info, const string& channel_name){
    // prepare the paths
    string s;
    for(int i=0;i<CLS;i++){
        s="spikes/Input_Response/"+to_string(i);
        if(access(s.c_str(),0)!=0)
            MakeDirs(s);
        s="spikes/Reservoir_Response/"+to_string(i);
        if(access(s.c_str(),0)!=0)
            MakeDirs(s);
        s="spikes/Readout_Response_Trans/"+to_string(i);
        if(access(s.c_str(),0)!=0)
            MakeDirs(s);
    }
    if(_sp_iter != _speeches.end())
        (*_sp_iter)->PrintSpikes(info, channel_name);
}

//* print the fire spike count of a specific layer 
void Network::PrintSpikeCount(string layer){
    NeuronGroup * ng = SearchForNeuronGroup(layer.c_str());
    assert(ng);
#ifdef CV
    assert(*_cv_test_sp_iter);
    ng->PrintSpikeCount((*_cv_test_sp_iter)->Class());
#else
    assert(*_sp_iter);
    ng->PrintSpikeCount((*_sp_iter)->Class());
#endif

}

//* judge the readout result:
void Network::ReadoutJudge(vector<pair<int, int> >& correct, vector<pair<int, int> >& wrong, vector<pair<int, int> >& even){
    pair<int, int> res = this->LSMJudge(); // +/- 1, true cls

    if(res.first == 1) correct.push_back(res);
    else if(res.first == -1) wrong.push_back(res);
    else if(res.first == 0) even.push_back(res);
    else{
        cout<<"In Network::ReadoutJudge(vector<pair<int, int> >&, vector<pair<int, int> >&, vector<pair<int, int> >&)\n"
            <<"Undefined return type: "<<res.first<<" returned by Network::LSMJudge()"
            <<endl;
    }
}

/*************************************************************************************
 * this function is to find the readout neuron with maximum firing frequency and see 
 * if it is the desired one with correct label. 
 * @ret: pair <first, second>; first: +1 ->correct,-1 -> wrong, 0: even
 *                             second: the true class index 
 **************************************************************************************/
pair<int, int> Network::LSMJudge(){
    assert(_lsm_output_layer); 
    assert(_sp);
    int cls = _sp->Class();
    return {_lsm_output_layer->Judge(cls), cls};
}

void Network::CrossValidation(int fold){
    int i, j, k, cls, sample;
    std::srand(0);

    assert(fold>1);
    _fold = fold;
    _CVspeeches = vector<vector<Speech*> >(fold, vector<Speech*>());
    cls = CLS;
    sample = _speeches.size();
    cout<<"sample: "<<sample<<endl;
    vector<int> ** index;
    index = new vector<int>*[cls];
    for(i = 0; i < cls; i++){
        index[i] = new vector<int>;
        for(j = 0; j < sample/cls; j++) index[i]->push_back(i+j*cls);
        random_shuffle(index[i]->begin(),index[i]->end());
    }

    for(k = 0; k < fold; k++)
        for(j = (k*sample)/(cls*fold); j < ((k+1)*sample)/(cls*fold); j++)
            for(i = 0; i < cls; i++){
                _CVspeeches[k].push_back(_speeches[(*index[i])[j]]);
            }

    for(i = 0; i < cls; i++) delete index[i];
    delete [] index;
}


void Network::IndexSpeech(){
    for(int i = 0; i < _speeches.size(); i++) _speeches[i]->SetIndex(i);
}

//* this function is used to print out the recognition rate at the current iteration
void Network::CurrentPerformance(int iter_n)
{
    int total = _readout_correct[iter_n] + _readout_wrong[iter_n] + _readout_even[iter_n];
    if(_tid == 0){
        cout<<"The performance @"<<iter_n<<" = "<<_readout_correct[iter_n]<<"/"<<total<<" = "<<double(_readout_correct[iter_n])*100/ total<<'%'<<endl;
    }
}

//* push the readout results into the corresponding vector:
void Network::LSMPushResults(const vector<pair<int, int> >& correct, const vector<pair<int, int> >& wrong, const vector<pair<int, int> >& even, int iter_n){
    assert(iter_n < _readout_correct.size());
    _readout_correct[iter_n] += correct.size();
    _readout_wrong[iter_n] += wrong.size();
    _readout_even[iter_n] += even.size();

    for(auto & p : correct){
        assert(p.second < _readout_correct_breakdown[iter_n].size());
        _readout_correct_breakdown[iter_n][p.second]++;
    }
}

//* log the test error into the corresponding vector:
//* test error defined: \sum_j 1/2*(o_j - y_j)^2, where o_j = spike_j /max_k(spike_k)
void Network::LogTestError(const vector<double>& each_sample_errors, int iter_n){
    assert(iter_n < _readout_test_error.size());
    double error_sum = 0;
    for(double e : each_sample_errors)  error_sum += e;
    _readout_test_error[iter_n] = error_sum;
}

//* merge the readout results from the network
void Network::MergeReadoutResults(vector<int>& r_correct, vector<int>& r_wrong, vector<int>& r_even, vector<vector<int> >& r_correct_bd){
    assert(!r_correct.empty() &&
            r_correct.size() == _readout_correct.size() &&
            r_wrong.size() == _readout_wrong.size() &&
            r_even.size() == _readout_even.size() &&
            _readout_correct.size() == _readout_wrong.size() &&
            _readout_correct.size() == _readout_correct_breakdown.size());

    for(size_t i = 0; i < r_correct.size(); ++i){
        r_correct[i] += _readout_correct[i];
        r_wrong[i] += _readout_wrong[i];
        r_even[i] += _readout_even[i];
        r_correct_bd[i] = r_correct_bd[i] + _readout_correct_breakdown[i];  
    }

}

//* merge the test error from the network:
void Network::MergeTestErrors(vector<double>& test_errors){
    assert(!test_errors.empty() && test_errors.size() == _readout_test_error.size());
    for(size_t i = 0; i < test_errors.size(); ++i)
        test_errors[i] += _readout_test_error[i];
}

//* random shuffle the training samples:
void Network::ShuffleTrainingSamples(){
#ifdef CV
    for(int i = 0; i < _CVspeeches.size(); ++i){
        if(i == _fold_ind)  continue;
        random_shuffle(_CVspeeches[i].begin(), _CVspeeches[i].end());
    }    
#else
    random_shuffle(_speeches.begin(), _speeches.end());
#endif
}


//* This is a supporting function. Given a synapse type (r/o synapses)
//* determine the syn_type to be returned (ret_syn_type).
//* The third parameter is the function name used to handle exception.
void Network::DetermineSynType(const char * syn_type, synapsetype_t & ret_syn_type, const char * func_name){
    if(strcmp(syn_type, "reservoir") == 0){
        // Write reservoir syns into the file
        ret_syn_type = RESERVOIR_SYN;
    }else if(strcmp(syn_type, "readout") == 0){
        ret_syn_type = READOUT_SYN;
    }else if(strcmp(syn_type, "input") == 0){
        ret_syn_type = INPUT_SYN;
    }else{
        cout<<"In Network::"<<func_name<<", undefined synapse type: "<<syn_type<<endl;
        exit(EXIT_FAILURE);
    } 
}

/***********************************************************************************
 * This function is to implement the weight normalization used by Oja rule.
 * Beacause of the usage of Hebbian learning rule, the weight saturation is possible.
 * The normalization is helpful for Heb-learn. For more details, please see the wiki.
 * @param1: the synapse type.
 * Note: after normalization, the weight of each synapse should be within 0~1.
 * This function should be called only for continuous weight and the w is changed !!
 **********************************************************************************/
void Network::NormalizeContinuousWeights(const char * syn_type){
    synapsetype_t  tar_syn_type = INVALID;
    DetermineSynType(syn_type, tar_syn_type, "NormalizeContinuousWeights()");
    assert(tar_syn_type != INVALID);
    vector<Synapse*> tmp;

    vector<Synapse*> & synapses = tar_syn_type == RESERVOIR_SYN ? _rsynapses : 
        (tar_syn_type == READOUT_SYN ? _rosynapses : tmp);
    assert(!synapses.empty()); 

    // right now the function should be run for reservoir synapses!
    double g_sum = 0.0;
    for(size_t i = 0; i < synapses.size(); ++i){
        assert(synapses[i]);
        double w = synapses[i]->Weight();
        g_sum += w*w;
    }

    // normalize the weight:
    assert(g_sum > 0);
    g_sum = sqrt(g_sum);
    for(size_t i = 0; i < synapses.size(); ++i){
        // compute the normalized weight:
        double new_w = synapses[i]->Weight()/g_sum;
        // set the new weight:
        synapses[i]->Weight(new_w);
    }
}

//* this function is to remove the synapses with zero weights:
void Network::RemoveZeroWeights(const char * type){
    synapsetype_t  syn_type = INVALID;
    DetermineSynType(type, syn_type, "RemoveZeroWeights()");
    assert(syn_type != INVALID);

    if(syn_type == RESERVOIR_SYN){
        NeuronGroup * reservoir = SearchForNeuronGroup("reservoir");
        reservoir->RemoveZeroSyns(syn_type);
    }
    else if(syn_type == READOUT_SYN){
        NeuronGroup * output = SearchForNeuronGroup("output");
        if(!output){
            cout<<"In Network::RemoveZeroWeights(), no 'output' layer is found!!"<<endl;
            assert(output);
        }
        output->RemoveZeroSyns(syn_type);
    }
    else{
        assert(0); // your code shoule never go here.
    }
}

//* write the weights of the selected synaptic type
void Network::WriteSelectedSynToFile(const string& syn_type, char * filename){

    ofstream f_out(filename);
    if(!f_out.is_open()){
        cout<<"In Network::WriteSelectedSynToFile(), cannot open the file : "<<filename<<endl;
        exit(EXIT_FAILURE);
    }

    synapsetype_t  wrt_syn_type = INVALID;
    DetermineSynType(syn_type.c_str(), wrt_syn_type, "WriteSelectedSynToFile()");
    assert(wrt_syn_type != INVALID);
    vector<Synapse*> tmp;

    const vector<Synapse*> & synapses = 
        wrt_syn_type == RESERVOIR_SYN ? _rsynapses : 
        wrt_syn_type == READOUT_SYN ? _rosynapses : 
        wrt_syn_type == INPUT_SYN ? _isynapses : tmp;
    for(size_t i = 0; i < synapses.size(); ++i)
        f_out<<i<<"\t"<<synapses[i]->PreNeuron()->Name()
            <<"\t"<<synapses[i]->PostNeuron()->Name()
            <<"\t"<<synapses[i]->Weight()<<endl;
    f_out.close();
}


//* write the weights of output synapses of the neurons in the group into a file:
void Network::WriteSynWeightsToFile(const string& pre_g, const string& post_g, char * filename){
    NeuronGroup * pre = SearchForNeuronGroup(pre_g.c_str());
    NeuronGroup * post = SearchForNeuronGroup(post_g.c_str());
    if(pre == NULL){
        cout<<"Warning::Cannot find neuron group named: "<<pre_g
            <<" for writing the weights"<<endl;
        return;
    }
    if(post == NULL){
        cout<<"Warning::Cannot find neuron group named: "<<post_g
            <<" for writing the weights"<<endl;
        return;
    }
    ofstream f_out(filename);
    if(!f_out.is_open()){
        cout<<"WriteSynWeightsToFile(), cannot open the file : "<<filename<<endl;
        exit(EXIT_FAILURE);
    }

    int index = 0;
    pre->WriteSynWeightsToFile(f_out, index, post_g);
    f_out.close();
}

//* Given a filename, load all the weigths recorded in the file to the network
void Network::LoadSynWeightsFromFile(const string& filename)
{
    ifstream f_in(filename);
    if(!f_in.is_open()){
        cout<<"In Network::LoadSynWeightsFromFile(), cannot open the file: "<<filename<<endl;
        assert(0);
    }
    int index;
    string pre;
    string post; 
    double weight;
    while(f_in>>index>>pre>>post>>weight){
        assert(_synapses_map.find(pre) != _synapses_map.end());
        assert(_synapses_map[pre].find(post) != _synapses_map[pre].end());
        _synapses_map[pre][post]->Weight(weight);
    }
   
}

//* This function is to load the synaptic weights from file and assign it to the synapses:
//* "syn_type" can be reservoir or readout or input
void Network::LoadSynWeightsFromFile(const char * syn_type, char * filename){
    ifstream f_in(filename);
    if(!f_in.is_open()){
        cout<<"In Network::LoadSynWeightsFromFile(), cannot open the file: "<<filename<<endl;
        assert(0);
    }

    synapsetype_t read_syn_type = INVALID;
    DetermineSynType(syn_type, read_syn_type, "LoadSynWeightsFromFile()");
    assert(read_syn_type != INVALID);
    vector<Synapse*> tmp;

    const vector<Synapse*> & synapses = 
        read_syn_type == RESERVOIR_SYN ? _rsynapses : 
        read_syn_type == READOUT_SYN ? _rosynapses :
        read_syn_type == INPUT_SYN ? _isynapses : tmp;

    assert(!synapses.empty());  
    // load the synaptic weights from file:
    // the synaptic weight is stored in the file in the same order as the corresponding synapses store in the vector
    int index;
    string pre;
    string post;
    
    double weight;
    while(f_in>>index>>pre>>post>>weight){//>>pre_ext>>post_ext){
        if(index < 0 || index >= synapses.size()){
            cout<<"In Network::LoadSynWeightsFromFile(), the index of the synapse you read : "<<index<<" is out of bound of the container stores the synapses!!"<<endl;
            exit(EXIT_FAILURE);
        }

        assert(strcmp(pre.c_str(), synapses[index]->PreNeuron()->Name()) == 0);
        assert(strcmp(post.c_str(), synapses[index]->PostNeuron()->Name()) == 0);
        synapses[index]->Weight(weight);
    }

}


void Network::LSMChannelDecrement(channelmode_t channelmode){
    if(channelmode == INPUTCHANNEL) _lsm_input--;
    else if(channelmode == RESERVOIRCHANNEL) _lsm_reservoir--;
    else assert(0); // you code should never go here!
}
