#ifndef NETWORK_H
#define NETWORK_H

#include <list>
#include <vector>
#include <string>
#include <set>
#include <stdlib.h>
#include "def.h"
#include <cstdio>
#include <fstream>
#include <utility>
#include <unordered_map>

class Neuron;
class BiasNeuron;
class Synapse;
class NeuronGroup;
class Speech;

class Network{
private:
    std::list<Neuron*> _individualNeurons;
    std::unordered_map<std::string, NeuronGroup*> _groupNeurons;
    std::unordered_map<std::string, std::unordered_map<std::string, Synapse*> > _synapses_map;
    std::list<Neuron*> _allNeurons;
    std::list<Neuron*> _allExcitatoryNeurons;
    std::list<Neuron*> _allInhibitoryNeurons;
    std::list<Neuron*> _inputNeurons;
    std::list<Neuron*> _outputNeurons;
    std::list<Synapse*> _synapses;
    std::vector<Synapse*> _rsynapses;       
    std::vector<Synapse*> _rosynapses;
    std::vector<Synapse*> _isynapses;
    std::list<Synapse*> _lsmActiveLearnSyns;
    std::list<Synapse*> _lsmActiveSyns;
    std::vector<int> _readout_correct; // sign : +1
    std::vector<int> _readout_wrong;   // sign: -1
    std::vector<int> _readout_even;    // sign: 0
    std::vector<std::vector<int> > _readout_correct_breakdown; // the break down for each class
    std::vector<double> _readout_test_error; // test error: (o - y)^2
	
    // for LSM
    std::vector<Speech*> _speeches;
    std::vector<Speech*>::iterator _sp_iter;
    int _fold;
    int _fold_ind;
    int _train_fold_ind;
    std::vector<std::vector<Speech*> > _CVspeeches;
    std::vector<Speech*>::iterator _cv_train_sp_iter, _cv_test_sp_iter;
    Speech * _sp;
    NeuronGroup * _lsm_input_layer;
    NeuronGroup * _lsm_reservoir_layer;
    std::vector<std::string> _hidden_layer_names;
    std::vector<NeuronGroup*> _lsm_hidden_layers;
    NeuronGroup * _lsm_output_layer;
    int _lsm_input;
    int _lsm_reservoir;
    int _lsm_output;
    int _lsm_t;
    networkmode_t _network_mode;
    int _tid;  // the id of the thread attach to the network
public:
    Network();
    ~Network();
    void LSMSetNetworkMode(networkmode_t networkmode){_network_mode = networkmode;}
    networkmode_t LSMGetNetworkMode(){return _network_mode;}

    int GetTid(){return _tid;}
    int SetTid(int tid){_tid = tid;}
    
    bool CheckExistence(char *);
    void AddNeuron(char* name, bool excitatory, double v_mem);
    void AddNeuronGroup(char* name, int num, bool excitatory, double v_mem);
    void LSMAddNeuronGroup(char*,int,int,int);
    void LSMAddNeuronGroup(char*,char*,char*); //reservoir, path_info_neuron, path_info_synapse

    void LSMAddSynapse(char*,char*,int,int,int,int,bool);
    void LSMAddSynapse(Neuron*,NeuronGroup*,int,int,int,bool);
    void LSMAddSynapse(NeuronGroup*,NeuronGroup*,int,int,int,int,bool);
    void LSMAddSynapse(NeuronGroup * pre, int npre, int npost, int value, int random, bool fixed);

    void PrintSize();
    void PrintAllNeuronName();

    int LSMSizeAllNeurons();
    
    void LSMTransientSim(networkmode_t networkmode, int tid, const std::string sample_type);
    void LSMSupervisedTraining(networkmode_t networkmode, int tid, int iteration);
    void LSMNextTimeStep(int t, bool train, int iteration, int end_time, FILE* Foutp);

    Neuron * SearchForNeuron(const char*);
    Neuron * SearchForNeuron(const char*, const char*);
    NeuronGroup * SearchForNeuronGroup(const char*);

    void IndexSpeech();

    // for LSM
    int  SpeechEndTime();
    void AddSpeech(Speech* speech);
    void AddTestSpeech(Speech* speech);
    void LoadSpeeches(Speech      * sp_iter, 
            neuronmode_t neuronmode_input,
            neuronmode_t neuronmode_reservoir,
            neuronmode_t neuronmode_readout,
            bool train
            );
    void LSMNetworkRemoveSpeech();
    void LSMLoadLayers();
    void DetermineNetworkNeuronMode(const networkmode_t &, neuronmode_t &, neuronmode_t &, neuronmode_t&);

    int LoadFirstSpeech(bool train, networkmode_t networkmode, const std::string sample_type = "train_sample");
    int LoadNextSpeech(bool train, networkmode_t networkmode, const std::string sample_type = "train_sample");
	int LoadSpeechAgain(bool train, networkmode_t networkmode, const std::string sample_type = "train_sample");
    int LoadFirstSpeechTrainCV(networkmode_t);
    int LoadNextSpeechTrainCV(networkmode_t);
    int LoadFirstSpeechTestCV(networkmode_t);
    int LoadNextSpeechTestCV(networkmode_t);

    int NumSpeech(){return _speeches.size();}
    int NumIteration(){return _readout_correct.size();}
    std::vector<int> NumEachSpeech();
    void AnalogToSpike();
    void LSMClearSignals();
	void LSMClearSignalsIP();
    void LSMClearWeights();
    bool LSMEndOfSpeech(networkmode_t networkmode, int end_time);
    void LSMChannelDecrement(channelmode_t);
    void PrintSpikeCount(std::string layer);
    void ReadoutJudge(std::vector<std::pair<int, int> >& correct, std::vector<std::pair<int, int> >& wrong, std::vector<std::pair<int, int> >& even);
    std::pair<int, int>  LSMJudge();
    
    //* this function is to add the active learning readout synapses:
    void LSMAddActiveLearnSyn(Synapse*synapse){_lsmActiveLearnSyns.push_back(synapse);}
    void LSMAddActiveSyn(Synapse*synapse){_lsmActiveSyns.push_back(synapse);}

    //* Add the target synapses that should be trained by STDP:
    void CrossValidation(int);
    void Fold(int fold_ind){_fold_ind = fold_ind;}

    void SpeechInfo();
    // print the spikes into the file
    void SpeechPrint(int info, const std::string& channel_name = "all");

    // supporting functions:
    // the push/view readout results:
    void CurrentPerformance(int iter_n);
    void LSMPushResults(const std::vector<std::pair<int, int> >& correct, const std::vector<std::pair<int, int> >& wrong, const std::vector<std::pair<int, int> >& even, int n_iter);
    std::vector<int> LSMViewResults();
    void MergeReadoutResults(std::vector<int>& r_correct, std::vector<int>& r_wrong, std::vector<int>& r_even, std::vector<std::vector<int> >& r_correct_bd);
    void MergeTestErrors(std::vector<double>& test_errors);

    // log the test errors:
    void LogTestError(const std::vector<double>& each_sample_errors, int iter_n);
    
    void ShuffleTrainingSamples();
    
    // this first parameter is used to indicate which neurongroup does the syn belongs to.
    void DetermineSynType(const char * syn_type, synapsetype_t & ret_syn_type, const char * func_name);
    void NormalizeContinuousWeights(const char * syn_type);
    void RemoveZeroWeights(const char * type);

    void WriteSelectedSynToFile(const std::string& syn_type, char * filename);
    void WriteSynWeightsToFile(const std::string& pre_g, const std::string& post_g, char * filename);
    void LoadSynWeightsFromFile(const std::string& filename);
    void LoadSynWeightsFromFile(const char * syn_type, char * filename);


    template<class T>
    void LSMAddSynapse(Neuron * pre, Neuron * post, T weight, bool fixed, T weight_limit,bool liquid, NeuronGroup * group = NULL){
        Synapse * synapse = new Synapse(pre, post, weight, fixed, weight_limit, pre->IsExcitatory(),liquid);
        _synapses.push_back(synapse);
        std::string pre_name = std::string(pre->Name());
        std::string post_name = std::string(post->Name());
        if(_synapses_map.find(pre_name) == _synapses_map.end()){
            _synapses_map[pre_name] = std::unordered_map<std::string, Synapse*>();
        }
        if(_synapses_map[pre_name].find(post_name) != _synapses_map[pre_name].end()){
            std::cout<<"Warning:: duplicate synapse "<<pre_name<<" to "<<post_name<<" detected"<<std::endl;
        }
        _synapses_map[pre_name][post_name] = synapse; 
    
        // push back the reservoir, readout and input synapses into the vector:
        if(!synapse->IsReadoutSyn() && !synapse->IsInputSyn())
            _rsynapses.push_back(synapse);
        else if(synapse->IsReadoutSyn())
            _rosynapses.push_back(synapse);
        else
            _isynapses.push_back(synapse);

        if(group)  group->AddSynapse(synapse);
        pre->AddPostSyn(synapse);
        post->AddPreSyn(synapse);	
    } 
};

#endif
