#ifndef NEURON_H
#define NEURON_H

#include <list>
#include <vector>
#include <map>
#include <set>
#include "def.h"
#include "util.h"
#include <cstdio>
#include <fstream>

class Synapse;
class Network;
class Speech;
class Channel;


class Neuron{
protected:
    neuronmode_t _mode;
    char * _name;

    std::vector<Synapse*> _outputSyns; 
    std::vector<Synapse*> _inputSyns; 
    std::vector<double> _fire_freq;
    std::vector<bool> _presyn_act;
    std::vector<int> _fire_timings;
    std::vector<double> _vmems;
    std::vector<double> _calcium_stamp;
    double _prev_delta_ep;
    double _prev_delta_en;
    double _prev_delta_ip;
    double _prev_delta_in;
    double _curr_delta_ep;
    double _curr_delta_en;
    double _curr_delta_ip;
    double _curr_delta_in;
    bool _excitatory; 
    int _teacherSignal;
    int _indexInGroup;
    bool _del;
    Network * _network;
    int _ind;
    int _f_count; // counter for firing activity
    bool _fired;
    double _error; // error used in the back-prop

    bool _SpiKL_ip; // enable dynamic threshold

    // collect ep/en/ip/in stat for resolution
    int _EP_max, _EP_min, _EN_max, _EN_min, _IP_max, _IP_min, _IN_max, _IN_min;
    // collect max pre-spike count at each time point
    int _pre_fire_max;

    /* FOR LIQUID STATE MACHINE */
    double _lsm_v_mem;
    double _lsm_v_mem_pre;
    double _lsm_calcium;
    double _lsm_calcium_pre;
    double _lsm_state_EP;
    double _lsm_state_EN;
    double _lsm_state_IP;
    double _lsm_state_IN;
    const double _lsm_tau_EP;
    const double _lsm_tau_EN;
    const double _lsm_tau_IP;
    const double _lsm_tau_IN;
    const double _lsm_tau_FO;
    double _lsm_v_thresh;
	double _lsm_R;
    double _inputsyn_sq_sum; // normalized sq sum of its presynaptic weights, initialized as -1
	bool _fire_start;
	double _lsm_t_m_c;
	double _lsm_input;
	double _generate_transient;

    int _lsm_ref;
    Channel * _lsm_channel;
    int _t_next_spike;

public:
    Neuron(char* name, bool excitatory, Network* network, double v_mem);
    Neuron(char* name, bool excitatory, Network* network); // only for reservoir
    ~Neuron();
    char * Name();
    void AddPostSyn(Synapse*);
    void AddPreSyn(Synapse*);

    void LSMPrintInputSyns(std::ofstream& f_out);

	bool IsReservoir(){
        if(_name[0] == 'r')
			return true;
		else
			return false;
	}

    // return EP/EN/IP/IN max/min
    int GetEPMax(){return _EP_max;}
    int GetENMax(){return _EN_max;}
    int GetIPMax(){return _IP_max;}
    int GetINMax(){return _IN_max;}
    int GetEPMin(){return _EP_min;}
    int GetENMin(){return _EN_min;}
    int GetIPMin(){return _IP_min;}
    int GetINMin(){return _IN_min;}
    int GetPreActiveMax(){return _pre_fire_max;}

    // return the tau for EP/EN/IP/IN
    int GetTauEP(){return _lsm_tau_EP;}
    int GetTauEN(){return _lsm_tau_EN;}
    int GetTauIP(){return _lsm_tau_IP;}
    int GetTauIN(){return _lsm_tau_IN;}

    // record the firing frequency
    void FireFreq(double f){_fire_freq.push_back(f);}
    double FireFreq(){return _fire_freq.empty() ? 0 : _fire_freq.back();}

    // record the firing count:
    int FireCount(){return _f_count;}
    void FireCount(int count){_f_count = count;}

    void EnableSpiKL_IP(bool dt){_SpiKL_ip=dt;}
    bool IsSpiKL_IP(){return _SpiKL_ip;}

    bool Fired(){return _fired;}
	
	void SetGenerateTransient(bool t){
		_generate_transient=t;
	}

    template<typename T> void GetWaveForm(std::vector<T>& v){v = _vmems;}
    // set the neuron index under the separated reservoir cases:
    void Index(int ind){_ind = ind;}

    // return the index of the neuron under the separated reservoir
    int Index(){return _ind;}

    void SetIndexInGroup(int index){_indexInGroup = index;}
    int IndexInGroup(){return _indexInGroup;}
    void SetVth(double vth);
    double GetVth(){return _lsm_v_thresh;}
    void SetTeacherSignal(int signal);
    int GetTeacherSignal(){return _teacherSignal;}
    void SetError(double error){_error = error;}
    double GetError(){return _error;}
    void PrintTeacherSignal();
    void PrintMembranePotential();
    Network * GetNetwork(){return _network;}

    double LSMGetVMemPre(){return _lsm_v_mem_pre;}
    double GetCalciumPre(){return _lsm_calcium_pre;}
    double GetCalcium(){return _lsm_calcium;}
    void SetExcitatory(){_excitatory = true;}
    bool IsExcitatory(){return _excitatory;}

    template<typename T> void IncreaseEP(T effect){_curr_delta_ep += effect;}
    template<typename T> void IncreaseEN(T effect){_curr_delta_en += effect;}
    template<typename T> void IncreaseIP(T effect){_curr_delta_ip += effect;}
    template<typename T> void IncreaseIN(T effect){_curr_delta_in += effect;}

    std::vector<double> GetCalciumStamp(){return _calcium_stamp;}
    void LSMClear();
	void LSMClearIP();

    /** Wrappers for clean code: **/
    void ExpDecay(int& var, const int time_c);
    void ExpDecay(double& var, const int time_c);
    void ExpDecay(double& var, double time_c);
    void AccumulateSynapticResponse(const int pos, double value);
    double NOrderSynapticResponse();
    void HandleFiringActivity(bool isInput, int time, bool train);

    void UpdateDeltaEffect();
    virtual void LSMNextTimeStep(int t , FILE * Foutp, bool train, int end_time);
    double LSMSumAbsInputWeights();
    int  DLSMSumAbsInputWeights();
    void LSMSetChannel(Channel*,channelmode_t);
    void LSMRemoveChannel();
    void GetSpikeTimes(std::vector<int>& times);
    void SetSpikeTimes(const std::vector<int>& times);
    void LSMSetNeuronMode(neuronmode_t neuronmode){_mode = neuronmode;}
    bool LSMCheckNeuronMode(neuronmode_t neuronmode){return _mode == neuronmode;}

    void CollectPreSynAct(double& p_n, double& avg_i_n, int& max_i_n);

    //* Bp the error for each neuron

    void SpiKL_IP(int t);

    void WriteOutputWeights(std::ofstream& f_out, int& index, const std::string& post_g); 
    void DisableOutputSyn(synapsetype_t syn_t);
    void LSMDeleteInputSynapse(char* pre_name);
    int RMZeroSyns(synapsetype_t syn_t, const char * t);
    void DeleteSyn(const char * t, const char s);
    void PrintSyn(std::ofstream& f_out, const char * t, const char s);

    void DeleteAllSyns();
    bool GetStatus();
    double GetInputSynSqSum(double weight_limit);
};

class BiasNeuron : public Neuron{
private:
    int _dummy_freq;
public:
    BiasNeuron(char* name, bool excitatory, Network * network, double v_mem, int dummy_f);
    void LSMNextTimeStep(int t , FILE * Foutp, bool train, int end_time);
};

class NeuronGroup{
private:
    char * _name;
    std::vector<Synapse*> _synapses;
    std::vector<Synapse*>::iterator _s_iter;
    std::vector<Neuron*> _neurons;
    std::vector<Neuron*>::iterator _iter;
    BiasNeuron* _b_neuron; // the bias neuron
    std::set<int> _s_labels;
    bool _firstCalled;
    bool _s_firstCalled;
    bool _has_lateral;
    double _lateral_w;

    int ** _lsm_coordinates;
    Network * _network;
public:
    NeuronGroup(char* name, int num, Network* network, bool excitatory, double v_mem);
    NeuronGroup(char*,int,int,int,Network*); // only for reservoir
    NeuronGroup(char*,char*,char*,Network*); // for brain-like structure
    ~NeuronGroup();

    char * Name(){return _name;};

    void AddSynapse(Synapse * synapse);
    Neuron * First();
    Neuron * Next();
    Synapse * FirstSynapse();
    Synapse * NextSynapse();

    Neuron * Order(int);
    void UnlockFirst();
    void UnlockFirstSynapse();

    int Size(){return _neurons.size();}
    void SubSpeechLabel(std::set<int> labels){_s_labels = labels;}
    std::set<int> SubSpeechLabel(){return _s_labels;}
    bool InSet(int num){return _s_labels.empty()? true: _s_labels.count(num)!=0;}

    void SetBiasNeuron(BiasNeuron * b_neuron){_b_neuron = b_neuron;}
    BiasNeuron* GetBiasNeuron(){return _b_neuron;}

    void PrintTeacherSignal();
    void PrintMembranePotential(double);
    Network * GetNetwork(){return _network;}

    void LSMLoadSpeech(Speech*,int*,neuronmode_t,channelmode_t);
    void LSMSetNeurons(neuronmode_t neuronmode);

    void SetLateral(){_has_lateral = true;}
    void SetLateralWeight(double weight){_lateral_w = weight;}

    void Collect4State(int& ep_max, int& ep_min, int& ip_max, int& ip_min, 
            int& en_max, int& en_min, int& in_max, int& in_min, int& pre_active_max);
    void CollectPreSynAct(double & p_r, double & avg_i_r, int & max_i_r);

    int Judge(int cls);
    int MaxFireCount();

    void UpdateLearningWeights();

    void WriteSynWeightsToFile(std::ofstream & f_out, int& index, const std::string& post_g);
    void DumpSpikeTimes(const std::string& filename);
    void DumpCalciumLevels(std::ofstream & f_out);
    void DumpVMems(std::ofstream & f_out);
    void PrintSpikeCount(int cls);
    void LSMRemoveSpeech();
    void LSMTuneVth(int index);
    void LSMSetTeacherSignal(int);
    void LSMRemoveTeacherSignal(int);
    void LSMPrintInputSyns(std::ofstream& f_out);

    void DestroyResConn();
    void PrintResSyn(std::ofstream& f_out);
    void RemoveZeroSyns(synapsetype_t syn_type);
	void SetGenerateTransient(bool t);
};

#endif

