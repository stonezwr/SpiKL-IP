#include "def.h"
#include "synapse.h"
#include "neuron.h"
#include "network.h"
#include "speech.h"
#include "channel.h"
#include "util.h"
#include <stdlib.h>
#include <iostream>
#include <utility>
#include <string>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <utility>
#include <algorithm>
#include <climits>
#include <cmath>


// NOTE: The time constants have been changed to 2*original settings 
//       optimized performance for letter recognition.
using namespace std;

extern int Current;
extern int Threshold;

/** unit defined for digital system but not used in continuous model **/
extern const int one = 1;


// ONLY FOR RESERVOIR
Neuron::Neuron(char * name, bool excitatory, Network * network):
    _mode(NORMAL),
    _excitatory(excitatory),
    _EP_max(INT_MIN), _EP_min(INT_MAX), _EN_max(INT_MIN), _EN_min(INT_MAX), 
    _IP_max(INT_MIN), _IP_min(INT_MAX), _IN_max(INT_MIN), _IN_min(INT_MAX),
    _prev_delta_ep(0), _prev_delta_en(0), _prev_delta_ip(0), _prev_delta_in(0),
    _curr_delta_ep(0), _curr_delta_en(0), _curr_delta_ip(0), _curr_delta_in(0),
    _pre_fire_max(0),
    _lsm_v_mem(0),
    _lsm_v_mem_pre(0),
    _lsm_calcium(0),
    _lsm_calcium_pre(0),
    _lsm_state_EP(0),
    _lsm_state_EN(0),
    _lsm_state_IP(0),
    _lsm_state_IN(0),
    _lsm_tau_EP(4.0*2),
    _lsm_tau_EN(LSM_T_SYNE*2),
    _lsm_tau_IP(4.0*2),
    _lsm_tau_IN(LSM_T_SYNI*2),
    _lsm_tau_FO(LSM_T_FO),
    _lsm_v_thresh(LSM_V_THRESH),
    _lsm_ref(0),
	_lsm_R(LSM_T_M_C),
    _lsm_channel(NULL),
	_lsm_input(0),
	_lsm_t_m_c(LSM_T_M_C),
    _inputsyn_sq_sum(-1),
    _t_next_spike(-1),
    _network(network),
    _teacherSignal(0),
    _ind(-1)
{
    _name = new char[strlen(name)+2];
    strcpy(_name,name);

    _indexInGroup = -1;
    _del = false;
    _f_count = 0;
    _fired = false;
    _error = 0;
    _SpiKL_ip = false;
	_fire_start=false;
}

//FOR INPUT AND OUTPUT
Neuron::Neuron(char * name, bool excitatory, Network * network, double v_mem):
    _mode(NORMAL),
    _excitatory(excitatory),
    _EP_max(INT_MIN), _EP_min(INT_MAX), _EN_max(INT_MIN), _EN_min(INT_MAX), 
    _IP_max(INT_MIN), _IP_min(INT_MAX), _IN_max(INT_MIN), _IN_min(INT_MAX),
    _prev_delta_ep(0), _prev_delta_en(0), _prev_delta_ip(0), _prev_delta_in(0),
    _curr_delta_ep(0), _curr_delta_en(0), _curr_delta_ip(0), _curr_delta_in(0),
    _pre_fire_max(0),
    _lsm_v_mem(0),
    _lsm_v_mem_pre(0),
    _lsm_calcium(0),
    _lsm_calcium_pre(0),
    _lsm_state_EP(0),
    _lsm_state_EN(0),
    _lsm_state_IP(0),
    _lsm_state_IN(0),
    _lsm_tau_EP(4.0*2),
    _lsm_tau_EN(LSM_T_SYNE*2),
    _lsm_tau_IP(4.0*2),
    _lsm_tau_IN(LSM_T_SYNI*2),
    _lsm_tau_FO(LSM_T_FO),
    _lsm_v_thresh(v_mem),
	_lsm_input(0),
    _lsm_ref(0),
	_lsm_R(LSM_T_M_C),
	_lsm_t_m_c(LSM_T_M_C),
    _lsm_channel(NULL),
    _inputsyn_sq_sum(-1),
    _t_next_spike(-1),
    _network(network),
    _teacherSignal(-1),
    _ind(-1)
{
    _name = new char[strlen(name)+2];
    strcpy(_name,name);

    _indexInGroup = -1;
    _del = false;
    _f_count = 0;
    _fired = false;
    _error = 0;
    _SpiKL_ip = false;
	_fire_start=false;
}


Neuron::~Neuron(){
    if(_name != NULL) delete [] _name;

}

char * Neuron::Name(){
    return _name;
}

void Neuron::AddPostSyn(Synapse * postSyn){
    _outputSyns.push_back(postSyn);
}

void Neuron::AddPreSyn(Synapse * preSyn){
    _inputSyns.push_back(preSyn);
}

void Neuron::LSMPrintInputSyns(ofstream& f_out){
    for(Synapse * synapse : _inputSyns){
        f_out<<synapse->PreNeuron()->Name()<<"\t"<<synapse->PostNeuron()->Name()<<"\t"<<synapse->Weight()<<endl;
    }
}

inline void Neuron::SetVth(double vth){
    _lsm_v_thresh = vth;
}

inline void Neuron::SetTeacherSignal(int signal){
    assert((signal >= -1) && (signal <= 1));
    _teacherSignal = signal;
}

inline void Neuron::PrintTeacherSignal(){
    cout<<"Teacher signal of "<<_name<<": "<<_teacherSignal<<endl;
}

inline void Neuron::PrintMembranePotential(){
    cout<<"Membrane potential of "<<_name<<": "<<_lsm_v_mem<<endl;
}

void Neuron::LSMClear(){
    _f_count = 0;
    _fired = false;
    _error = 0;
    _vmems.clear();
    _calcium_stamp.clear();
    _fire_timings.clear();

    _prev_delta_ep = 0, _prev_delta_en = 0, _prev_delta_ip = 0, _prev_delta_in = 0;
    _curr_delta_ep = 0, _curr_delta_en = 0, _curr_delta_ip = 0, _curr_delta_in = 0;

    _lsm_ref = 0;

    _lsm_v_mem = 0;
    _lsm_v_mem_pre = 0;
    _lsm_calcium = 0;        // both of calicum might be LSM_CAL_MID-3;
    _lsm_calcium_pre = 0; 
    _lsm_state_EP = 0;
    _lsm_state_EN = 0;
    _lsm_state_IP = 0;
    _lsm_state_IN = 0;

    _t_next_spike = -1;
    _teacherSignal = 0;

    _inputsyn_sq_sum = -1;

	_fire_start=false;
	_lsm_R=LSM_T_M_C;
	_lsm_input=0;
	_lsm_t_m_c=LSM_T_M_C;
}
void Neuron::LSMClearIP(){
    _f_count = 0;
    _fired = false;
    _error = 0;
    _vmems.clear();
    _calcium_stamp.clear();
    _fire_timings.clear();

    _prev_delta_ep = 0, _prev_delta_en = 0, _prev_delta_ip = 0, _prev_delta_in = 0;
    _curr_delta_ep = 0, _curr_delta_en = 0, _curr_delta_ip = 0, _curr_delta_in = 0;

    _lsm_ref = 0;

    _lsm_v_mem = 0;
    _lsm_v_mem_pre = 0;
    _lsm_calcium = 0;        // both of calicum might be LSM_CAL_MID-3;
    _lsm_calcium_pre = 0; 
    _lsm_state_EP = 0;
    _lsm_state_EN = 0;
    _lsm_state_IP = 0;
    _lsm_state_IN = 0;

    _t_next_spike = -1;
    _teacherSignal = 0;

    _inputsyn_sq_sum = -1;

	_fire_start=false;
	_lsm_input=0;
}

//* function wrapper for ExpDecay under continuous case
//****** IMPORTANT: According to my experiment, the leaking terms seem to 
//******            too large for v_mem. And you need to reduce the leaking
//******            so that good performance is obtained under continuous case.
//******            But without leaking term, the continuous case will not work!
inline void Neuron::ExpDecay(double & var, const int time_c){
    var -= var/time_c;
#ifdef DIGITAL
    assert(0);
#endif
}
inline void Neuron::ExpDecay(double & var, double time_c){
    var -= var/time_c;
#ifdef DIGITAL
    assert(0);
#endif
}

/** collect the synaptic response and accumulate them **/
inline void Neuron::AccumulateSynapticResponse(const int pos, double value){
    if(pos > 0){
        _lsm_state_EP += value;
        _lsm_state_EN += value;
    }
    else{
        _lsm_state_IP += value;
        _lsm_state_IN += value;
    }

}

/** Calculate the whole response together **/
inline double Neuron::NOrderSynapticResponse(){
    return (_lsm_state_EP-_lsm_state_EN)/(_lsm_tau_EP-_lsm_tau_EN)+(_lsm_state_IP-_lsm_state_IN)/(_lsm_tau_IP-_lsm_tau_IN);
}


/**********************************************************************************
 *  This is a function to handle the firing actvities with regard to postsynaptic-neurons
 *  @para1: is used as a neuron that only output spikes to drive the network
 *          both input and reservoir neurons can be in this case; 
 *  @para2: simulation time; @para3: in supervised training or not
 **********************************************************************************/      
inline void Neuron::HandleFiringActivity(bool isInput, int time, bool train){

    for(Synapse * synapse : _outputSyns){
        // need to get rid of the deactivated neuron !
        if(synapse->PostNeuron()->LSMCheckNeuronMode(DEACTIVATED) == true)  continue;

        // need to get rid of the deactivated synapse !
        if(synapse->DisableStatus())  continue;
    
        synapse->LSMDeliverSpike();  
		if(isInput){
			if(_name[0] != 'i'&&_mode == READCHANNEL){
				if(synapse->IsReadoutSyn())
					synapse->LSMActivate(_network, true, train);
			}
		}
    }

}

//* update the previous delta effect with current delta effect
void Neuron::UpdateDeltaEffect(){
    if(_mode == DEACTIVATED  || _mode == READCHANNEL)
        return;
    
    _prev_delta_ep = _curr_delta_ep;
    _prev_delta_en = _curr_delta_en;
    _prev_delta_ip = _curr_delta_ip;
    _prev_delta_in = _curr_delta_in;
   
    _curr_delta_ep = 0; 
    _curr_delta_en = 0; 
    _curr_delta_ip = 0; 
    _curr_delta_in = 0; 
}

void Neuron::LSMNextTimeStep(int t, FILE * Foutp, bool train, int end_time){

    if(_mode == DEACTIVATED) return;
    if(_mode == READCHANNEL ){
        if(_mode != READCHANNEL){
            // if training the input-reservoir/reservoir-readout synapses, need to keep track the cal!
            _lsm_v_mem_pre = _lsm_v_mem;
            _lsm_calcium_pre = _lsm_calcium;
            _lsm_calcium -= _lsm_calcium/TAU_C;
        }
        if(_t_next_spike == -1) return;
        if(t < _t_next_spike) return;
        if(_mode != READCHANNEL){
            _lsm_calcium += one;
        }

        /** Hand the firing behavior here for both input neurons and reservoir neurons */
        /** @param1: is the neuron only used to output spike? **/
        HandleFiringActivity(true, t, train);
        _fired = true;

        _t_next_spike = _lsm_channel->NextSpikeT();
        if(_t_next_spike == -1) {
            _network->LSMChannelDecrement(_lsm_channel->Mode()); 
        }
        return;
    }
    

    // the following code is to simulate the mode of NORMAL, WRITECHANNEL, and STDP
    _lsm_v_mem_pre = _lsm_v_mem;
    _lsm_calcium_pre = _lsm_calcium;
    _lsm_calcium -= _lsm_calcium/TAU_C;

    if((_teacherSignal==1)&&(_lsm_calcium < LSM_CAL_MID+1)){
        _lsm_v_mem += 20;
    }else if((_teacherSignal==-1)&&(_lsm_calcium > LSM_CAL_MID-1)){
        _lsm_v_mem -= 15*0.75;
    }

    list<Synapse*>::iterator iter;

    int pos;
    double value;
	if(_name[0]=='r')
		ExpDecay(_lsm_v_mem, _lsm_t_m_c);
	else
		ExpDecay(_lsm_v_mem, LSM_T_M_C);
    ExpDecay(_lsm_state_EP, _lsm_tau_EP);
    ExpDecay(_lsm_state_EN, _lsm_tau_EN);
    ExpDecay(_lsm_state_IP, _lsm_tau_IP);
    ExpDecay(_lsm_state_IN, _lsm_tau_IN);

    // sum up the effect
    _lsm_state_EP += _prev_delta_ep;
    _lsm_state_EN += _prev_delta_en;
    _lsm_state_IP += _prev_delta_ip;
    _lsm_state_IN += _prev_delta_in;

    double temp = NOrderSynapticResponse();
    _lsm_v_mem += _lsm_R*temp/_lsm_t_m_c;
	_lsm_input=temp;
	if(temp>0){
		_fire_start=true;
	}

    if(_lsm_ref > 0){
        _lsm_ref--;
        _lsm_v_mem = LSM_V_REST;
        _fired = false;
        return;
    }

    _fired = false;

    if(_lsm_v_mem > _lsm_v_thresh){
        _lsm_calcium += one;

        // 1. handle the _outputSyns after the neuron fires and activate the _outputSyns
        // 2. keep track of the t_spike_pre for the corresponding syns
        // 3. @para1: whether or not the current neuron is only used as a dummy input
        HandleFiringActivity(false, t, train);
        _fired = true;

        _lsm_v_mem = LSM_V_RESET;

		_lsm_ref = LSM_T_REFRAC;

        if(_name[0] == 'o' || _name[0] == 'h'){
            _f_count ++;
            assert(t > (_fire_timings.empty() ? -1 : _fire_timings.back()));
            _fire_timings.push_back(t);
        }
		else
			_f_count++;

        if(_mode == WRITECHANNEL&&_generate_transient){
            if(_lsm_channel == NULL){
                cout<<"Failure to assign a channel ptr to the neuron: "<<_name<<endl;
                assert(_lsm_channel);
            }
            _lsm_channel->AddSpike(t);
        }

    }
#ifdef SPIKL_IP
    if(_SpiKL_ip && _network->LSMGetNetworkMode()==TRANSIENTSTATE)  
        SpiKL_IP(t);
#endif
}


void Neuron::LSMSetChannel(Channel * channel, channelmode_t channelmode){
    _lsm_channel = channel;
    _t_next_spike = _lsm_channel->FirstSpikeT();
    if(_t_next_spike == -1 || _mode == DEACTIVATED) _network->LSMChannelDecrement(channelmode);

}

void Neuron::LSMRemoveChannel(){
    _lsm_channel = NULL;
}


//* Get the timing of the spikes
void Neuron::GetSpikeTimes(vector<int>& times){
    if((_name[0] == 'o' || _name[0] == 'h') || !_fire_timings.empty()){
        times = _fire_timings;
    }
    else{
        if(_lsm_channel == NULL){
            times = vector<int>();
        }
        else
            _lsm_channel->GetAllSpikes(times);
    }
}

//* Set the timing of the spikes, only valid for the readout or hidden neurons
void Neuron::SetSpikeTimes(const vector<int>& times){
    assert(_name[0] == 'o' || _name[0] == 'h');
    _fire_timings = times;
}

//* Collect the presynaptic neuron firing activity:
void Neuron::CollectPreSynAct(double& p_n, double& avg_i_n, int& max_i_n){
    if(_presyn_act.empty()){
        cout<<"Do you forget to record the pre-synaptic firing activities??\n"
            <<"Or do you mistakenly clear the vector<bool> _presyn_act ?"<<endl;
        assert(!_presyn_act.empty());
    }

    int sum_intvl = 0, cnt_f = 0, max_i = 0;
    int start = -1;
    for(int i = 0; i < _presyn_act.size(); ++i){
        if(_presyn_act[i]){
            ++cnt_f;
            max_i = max(max_i, i - start - 1);
            sum_intvl += i - start - 1;
            start = i;
        }
    }
    max_i = max(max_i, (int)_presyn_act.size() - start - 1);
    sum_intvl += _presyn_act.size() - start - 1;

    p_n = ((double)cnt_f)/((double)_presyn_act.size());
    avg_i_n = ((double)sum_intvl)/(cnt_f+1);
    max_i_n = max_i;
    // clear the presynaptic neuron activity vector after visiting it!
    _presyn_act.clear();
}


// The function to apply dynamic threshold
void Neuron::SpiKL_IP(int t){
	double Tau_Lower_Bound=32;
	double Tau_Upper_Bound=512;
	double R_Lower_Bound=32;
	double R_Upper_Bound=512;
	double y=_lsm_calcium/(TAU_C-1);
	double LR1=5;
	double LR2=5;
	double beta=5;  //reciprocal of miu
    if(!(_name[0] == 'r' && _name[9] == '_'))   return;	
	double X;
	if(y>0.01){
		X=_lsm_v_thresh/(exp((1/_lsm_t_m_c)*(1/y-LSM_T_REFRAC))-1);
		_lsm_R+=LR1*(2*y*_lsm_t_m_c*_lsm_v_thresh-X-_lsm_v_thresh-beta*_lsm_t_m_c*_lsm_v_thresh*y*y)/(_lsm_R*X);
		_lsm_t_m_c+=LR2*(2*LSM_T_REFRAC*y-1-beta*(LSM_T_REFRAC*y*y-y))/_lsm_t_m_c;		
	}
	else{
		_lsm_R+=LR1*0.1;
		_lsm_t_m_c-=LR2*0.1;
	}
	if(_lsm_R<R_Lower_Bound)
		_lsm_R=R_Lower_Bound;
	if(_lsm_R>R_Upper_Bound)
		_lsm_R=R_Upper_Bound;

	if(_lsm_t_m_c<Tau_Lower_Bound)
		_lsm_t_m_c=Tau_Lower_Bound;
	if(_lsm_t_m_c>Tau_Upper_Bound)
		_lsm_t_m_c=Tau_Upper_Bound;
	

}


//* write the output synaptic weigths to the file
void Neuron::WriteOutputWeights(ofstream& f_out, int& index, const string& post_g){
    for(Synapse* synapse : _outputSyns){
        assert(synapse);
        string post_name(synapse->PostNeuron()->Name());

        if(post_name.substr(0, post_g.length()) != post_g)   continue;
        f_out<<index++<<"\t"<<_name<<"\t"<<synapse->PostNeuron()->Name()<<"\t"
             <<synapse->Weight()<<endl;
    }
}


//* this function is to disable the output synapses whose type is syn_t
void Neuron::DisableOutputSyn(synapsetype_t syn_t){
    for(Synapse* synapse : _outputSyns){
        bool flag = syn_t == INPUT_SYN ? synapse->IsInputSyn() :
            syn_t == READOUT_SYN ? synapse->IsReadoutSyn() : 
            syn_t == RESERVOIR_SYN ? synapse->IsLiquidSyn() : false;
        if(flag){
            synapse->DisableStatus(true);
        }
    }

}

// erase the ptr of the input syns given the name of the presynaptic neuron
void Neuron::LSMDeleteInputSynapse(char * pre_name){
    Neuron * pre;
    for(auto iter = _inputSyns.begin(); iter != _inputSyns.end(); ){
        pre = (*iter)->PreNeuron();
        assert(pre != NULL);
        //cout<<pre->Name()<<endl;
        if(strcmp(pre->Name(),pre_name) == 0){
            iter = _inputSyns.erase(iter);
            break;
        }
        else
            iter++;	       
    }
}

// erase the ptr of the synapses whose weight is zero with the given type
// @param1: synapsetype_t , @param2: "in" or "out" synapses
int Neuron::RMZeroSyns(synapsetype_t syn_t, const char * t){
    bool f;
    int cnt = 0;
    if(strcmp(t, "in") == 0)  f = false;
    else if(strcmp(t, "out") == 0)  f = true;
    else assert(0);
    vector<Synapse*>& syns = !f ? _inputSyns : _outputSyns;

    for(auto iter = syns.begin(); iter != syns.end(); ){
        assert(*iter);
        bool flag = syn_t == INPUT_SYN ? (*iter)->IsInputSyn() :
            syn_t == READOUT_SYN ? (*iter)->IsReadoutSyn() : 
            syn_t == RESERVOIR_SYN ? (*iter)->IsLiquidSyn() : false;
        flag &= (*iter)->IsWeightZero();
        if(flag){
            (*iter)->DisableStatus(true); 
            iter = syns.erase(iter);
            cnt++;
        }
        else
            iter++;	       
    }
    return cnt;
}


//* This function is to delete all the in/out synapses starts with char 's'
// @param1: the type : "in" or "out"; @param2: the start character
void Neuron::DeleteSyn(const char * t, const char s){
    bool f;
    if(strcmp(t, "in") == 0)  f = false;
    else if(strcmp(t, "out") == 0)  f = true;
    else assert(0);
    vector<Synapse*>& syns = !f ? _inputSyns : _outputSyns;

    // delete all the input synapses starts with 's':
    for(auto it=syns.begin();it != syns.end(); ){
        Neuron * pre = (*it)->PreNeuron();  assert(pre);
        char * pre_name = pre->Name();  assert(pre_name);
        if(pre_name[0] == s)   it = syns.erase(it);
        else ++it;
    }
}

//* This function is to print all the in/out synapses starts with char 's'
//* It is mainly used for debugging purpose.
//* @param1: target file name @param2: "in" or "out"; @param3: the start character 
void Neuron::PrintSyn(ofstream& f_out, const char * t, const char s){
    bool f;
    if(strcmp(t, "in") == 0)  f = false;
    else if(strcmp(t, "out") == 0)  f = true;
    else assert(0);
    vector<Synapse*>& syns = !f ? _inputSyns : _outputSyns;

    // print all the input synapses starts with 's':
    for(auto it=syns.begin();it != syns.end(); ++it){
        Neuron * neu = !f ? (*it)->PreNeuron() : (*it)->PostNeuron();  assert(neu);
        char * name = neu->Name();  assert(name);
        if(name[0] == s){
            if(!f)
                f_out<<name<<"\t"<<_name;
            else
                f_out<<_name<<"\t"<<name;
        }   
    }
    f_out<<endl;
}

void Neuron::DeleteAllSyns(){
    _inputSyns.clear();
    _outputSyns.clear();
    _del = true;
}


inline bool Neuron::GetStatus(){
    if(_del == true) return true;
    else return false;
}

//* Get the square sum of all its input synapses
double Neuron::GetInputSynSqSum(double weight_limit){
    if(_inputsyn_sq_sum == -1){
        double sum = 0;
        for(auto it = _inputSyns.begin(); it != _inputSyns.end(); ++it){
            double weight = (*it)->Weight();
            sum += (weight/weight_limit)*(weight/weight_limit);
        }
        _inputsyn_sq_sum = _inputSyns.empty() ? 0 : sum/_inputSyns.size();
    }
    return _inputsyn_sq_sum;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

BiasNeuron::BiasNeuron(char * name, bool excitatory, Network * network, double vmem, int dummy_f):
Neuron(name, excitatory, network, vmem),
_dummy_freq(dummy_f)
{
}

void BiasNeuron::LSMNextTimeStep(int t, FILE * Foutp, bool train, int end_time){
    assert(_mode == NORMAL);
    _fired = false;
    if(t % _dummy_freq == 0){
        HandleFiringActivity(true, t, train);
        _fired = true;
        ++_f_count;
        assert(t > (_fire_timings.empty() ? -1 : _fire_timings.back()));
        _fire_timings.push_back(t);
    }  
}


///////////////////////////////////////////////////////////////////////////

NeuronGroup::NeuronGroup(char * name, int num, Network * network, bool excitatory, double v_mem):
    _firstCalled(false),
    _s_firstCalled(false),
    _lsm_coordinates(0),
    _b_neuron(NULL),
    _has_lateral(false),
    _lateral_w(0.0),
    _network(network)
{
    _name = new char[strlen(name)+2];
    strcpy(_name,name);
    _neurons.resize(num);

    char * neuronName = new char[1024];
    for(int i = 0; i < num; i++){
        sprintf(neuronName,"%s_%d",name,i);
        Neuron * neuron = new Neuron(neuronName, excitatory, network, v_mem);
        neuron->SetIndexInGroup(i);
        _neurons[i] = neuron;
    }

    delete [] neuronName;
}

NeuronGroup::NeuronGroup(char * name, char * path_neuron, char * path_synapse, Network * network):
    _firstCalled(false),
    _s_firstCalled(false),
    _b_neuron(NULL),
    _has_lateral(false),
    _lateral_w(0.0),
    _network(network)
{
    bool excitatory;
    char ** token = new char*[64];
    char linestring[8192];
    int i,weight_value,count;
    size_t pre_i,post_j;
    FILE * fp_neuron_path = fopen(path_neuron , "r");
    assert(fp_neuron_path != NULL);
    FILE * fp_synapse_path = fopen(path_synapse , "r");
    assert(fp_synapse_path != NULL);

    _name = new char[strlen(name)+2];
    strcpy(_name,name);

    _neurons.resize(0);

    while(fgets(linestring, 8191 , fp_neuron_path) != NULL){ //Parse the information of reservoir neurons
        if(strlen(linestring) <= 1)  continue;
        if(linestring[0] == '#')  continue;

        token[0] = strtok(linestring,"\t\n");
        assert(token[0] != NULL);
        token[1] = strtok(NULL,"\t\n");
        assert(token[1] != NULL);
        assert((strcmp(token[1],"excitatory") == 0)||(strcmp(token[1],"inhibitory") == 0));

        if(strcmp(token[1],"excitatory") == 0) excitatory = true;
        else excitatory = false;
        Neuron * neuron = new Neuron(token[0],excitatory,network);
        _neurons.push_back(neuron);        
    }
    //  for(i = 0; i < _neurons.size(); ++i)
    //    cout<<"i:\t"<<i<<"\tname:"<<(*_neurons[i]).Name()<<endl;
    fclose(fp_neuron_path);

    count = 0;
    while(fgets(linestring, 8191, fp_synapse_path) != NULL){ //Parse the information of connectivity within the reservoir
        if(strlen(linestring) <= 1) continue;
        if(linestring[0] == '#')  continue;

        token[0] = strtok(linestring,"\t\n");
        assert(token[0] != NULL);
        token[1] = strtok(NULL,"\t\n");
        assert(token[1] != NULL);
        token[2] = strtok(NULL,"\t\n");
        assert(token[1] != NULL);
        token[3] = strtok(NULL,"\t\n");
        assert(token[1] != NULL);

        pre_i = atoi(token[0]+10);
        post_j = atoi(token[1]+10);
        assert((pre_i <= _neurons.size())&&(post_j <= _neurons.size()));

        weight_value = atoi(token[3]);
        _network->LSMAddSynapse(_neurons[pre_i], _neurons[post_j], (double)weight_value, true, 8.0, true, this);
        ++count;
    }
    fclose(fp_synapse_path);

    cout<<"# of Reservoir Synapses = "<<count<<endl;
    delete [] token;

}    




NeuronGroup::NeuronGroup(char * name, int dim1, int dim2, int dim3, Network * network):
    _firstCalled(false),
    _s_firstCalled(false),
    _b_neuron(NULL),
    _has_lateral(false),
    _lateral_w(0.0),
    _network(network)
{
    int num = dim1*dim2*dim3;
    bool excitatory;
    int i, j, k, index;
    char * neuronName = new char[1024];
    srand(5);
    _name = new char[strlen(name)+2];
    strcpy(_name,name);

    _firstCalled = false;
    _neurons.resize(num);
    _lsm_coordinates = new int*[num];

    // initialization of neurons
    for(i = 0; i < num; i++){
        if(rand()%100 < 20) excitatory = false;
        else excitatory = true;
        sprintf(neuronName,"%s_%d",name,i);
        Neuron * neuron = new Neuron(neuronName,excitatory,network);
        neuron->SetIndexInGroup(i);
        _neurons[i] = neuron;
        _lsm_coordinates[i] = new int[3];
    }

    for(i = 0; i < dim1; i++)
        for(j = 0; j < dim2; j++)
            for(k = 0; k < dim3; k++){
                index = ((i*dim2+j)*dim3)+k;
                _lsm_coordinates[index][0] = i;
                _lsm_coordinates[index][1] = j;
                _lsm_coordinates[index][2] = k;
            }

    // initialization of synapses
    double c, a;
    double distsq, dist;
    const double factor = 10;
    const double factor2 = 1;
    int counter = 0;
    for(i = 0; i < num; i++)
        for(j = 0; j < num; j++){
            //      if(i==j) continue;
            if(_neurons[i]->IsExcitatory()){
                if(_neurons[j]->IsExcitatory()){
                    c = 0.3*factor2;
                    a = 1;
                }else{
                    c = 0.2*factor2;
                    a = 1;
                }
            }else{
                if(_neurons[j]->IsExcitatory()){
                    c = 0.4*factor2;
                    a = -1;
                }else{
                    c = 0.1*factor2;
                    a = -1;
                }
            }

            distsq = 0;
            dist= _lsm_coordinates[i][0]-_lsm_coordinates[j][0];
            distsq += dist*dist;
            dist= _lsm_coordinates[i][1]-_lsm_coordinates[j][1];
            distsq += dist*dist;
            dist= _lsm_coordinates[i][2]-_lsm_coordinates[j][2];
            distsq += dist*dist;
            if(rand()%100000 < 100000*c*exp(-distsq/3)){
                counter++;
                _network->LSMAddSynapse(_neurons[i], _neurons[j],a, true, 8.0, true, this);
            }
    }

    cout<<"# of reservoir synapses = "<<counter<<endl;

    delete [] neuronName;
    neuronName = 0;
}

NeuronGroup::~NeuronGroup(){
    if(_name != NULL) delete [] _name;
}

// add the synapse (reservoir synapses) into the neurongroup:
void NeuronGroup::AddSynapse(Synapse * synapse){
    assert(synapse && !synapse->IsReadoutSyn()&& !synapse->IsInputSyn());
    _synapses.push_back(synapse);
}

Neuron * NeuronGroup::First(){
    assert(_firstCalled == false);
    _firstCalled = true;
    _iter = _neurons.begin();
    if(_iter != _neurons.end()) return (*_iter);
    else return NULL;
}

Neuron * NeuronGroup::Next(){
    assert(_firstCalled == true);
    if(_iter == _neurons.end()) return NULL;
    _iter++;
    if(_iter != _neurons.end()) return (*_iter);
    else{
        _firstCalled = false;
        return NULL;
    }
}

Synapse * NeuronGroup::FirstSynapse(){
    assert(_s_firstCalled == false);
    _s_firstCalled = true;
    _s_iter = _synapses.begin();
    if(_s_iter != _synapses.end()) return (*_s_iter);
    else return NULL;
}

Synapse * NeuronGroup::NextSynapse(){
    assert(_s_firstCalled == true);
    if(_s_iter == _synapses.end()) return NULL;
    _s_iter++;
    if(_s_iter != _synapses.end()) return (*_s_iter);
    else{
        _s_firstCalled = false;
        return NULL;
    }
}


Neuron * NeuronGroup::Order(int index){
    assert((index >= 0)&&(index < _neurons.size()));
    return _neurons[index];
}

void NeuronGroup::UnlockFirst(){
    _firstCalled = false;
}

void NeuronGroup::UnlockFirstSynapse(){
    _s_firstCalled = false;     
}

void NeuronGroup::PrintTeacherSignal(){
    for(int i = 0; i < _neurons.size(); i++) _neurons[i]->PrintTeacherSignal();
}

void NeuronGroup::PrintMembranePotential(double t){
    cout<<t;
    for(int i = 0; i < _neurons.size(); i++) _neurons[i]->PrintMembranePotential();
    cout<<endl;
}

//* this function is to load speech into the neuron group:
//* channel mode can be INPUT/RESERVOIR, which is a way to tell the types of the neuron:
void NeuronGroup::LSMLoadSpeech(Speech * speech, int * n_channel, neuronmode_t neuronmode, channelmode_t channelmode){
    assert(speech);
    if(_neurons.size() != speech->NumChannels(channelmode)){
        if(!(channelmode == RESERVOIRCHANNEL && (speech->NumChannels(channelmode) == 0))){
            cout<<"channelmode: "<<channelmode<<" has "<<speech->NumChannels(channelmode)<<" channels!"<<endl;
            assert(channelmode == RESERVOIRCHANNEL && speech->NumChannels(channelmode) == 0);
        }
        if(channelmode == RESERVOIRCHANNEL)
            speech->SetNumChannel(_neurons.size(), RESERVOIRCHANNEL);
        else
            assert(0); // your code should never go here
    }
    if(neuronmode == DEACTIVATED)
        *n_channel = 0;
    else
        *n_channel = speech->NumChannels(channelmode);

    // assign the channel ptr to each neuron:
    for(int i = 0; i < _neurons.size(); i++){
        _neurons[i]->LSMSetChannel(speech->GetChannel(i,channelmode),channelmode);
    }
    LSMSetNeurons(neuronmode);
}

//* Set the neuron mode for each neuron in the group
void NeuronGroup::LSMSetNeurons(neuronmode_t neuronmode){
    for(int i = 0; i < _neurons.size(); i++){
        _neurons[i]->LSMSetNeuronMode(neuronmode);
    }
}

//* Collect the max/min for E/I/P/N and max # of active pre-spikes for each neuron 
void NeuronGroup::Collect4State(int& ep_max, int& ep_min, int& ip_max, int& ip_min, 
        int& en_max, int& en_min, int& in_max, int& in_min, int& pre_active_max)
{
    for(size_t i = 0; i < _neurons.size(); ++i){
        assert(_neurons[i]);
        ep_max = max(ep_max, _neurons[i]->GetEPMax()), ep_min = min(ep_min, _neurons[i]->GetEPMin());
        en_max = max(en_max, _neurons[i]->GetENMax()), en_min = min(en_min, _neurons[i]->GetENMin());
        ip_max = max(ip_max, _neurons[i]->GetIPMax()), ip_min = min(ip_min, _neurons[i]->GetIPMin());
        in_max = max(in_max, _neurons[i]->GetINMax()), in_min = min(in_min, _neurons[i]->GetINMin());
        pre_active_max = max(pre_active_max, _neurons[i]->GetPreActiveMax());
    }
}


//* Collect the presynaptic firing activity:
void NeuronGroup::CollectPreSynAct(double & p_r, double & avg_i_r, int & max_i_r){
    double p_n = 0.0, avg_i_n = 0.0;
    int max_i_n = 0;
    for(size_t i = 0; i < _neurons.size(); ++i){
        assert(_neurons[i]);
        _neurons[i]->CollectPreSynAct(p_n, avg_i_n, max_i_n);
        p_r += p_n,  avg_i_r += avg_i_n, max_i_r = max(max_i_r, max_i_n);
    }
    p_r = _neurons.empty() ? 0 : p_r/((double)_neurons.size());
    avg_i_r = _neurons.empty() ? 0 : avg_i_r/((double)_neurons.size());
}


//* judge the results of the readout layer after each speech is presented:
int NeuronGroup::Judge(int cls){
    vector<pair<int, int> > f_pairs;
    auto comp = [](const pair<int, int>& x, const pair<int, int>& y){ return x.first > y.first;};
    for(int i = 0; i < _neurons.size(); ++i){  
        assert(_neurons[i]);
        f_pairs.push_back(make_pair(_neurons[i]->FireCount(), i));
    }
    sort(f_pairs.begin(), f_pairs.end(), comp);
    if(f_pairs.size() < 2){
        cerr<<"Warning: only "<<f_pairs.size()<<" output neurons!!"<<endl;
        return 0;
    }

    int res = f_pairs[0].second;
    if(res == cls && f_pairs[0].first > f_pairs[1].first)  return 1; // correct case
    else if(res == cls && f_pairs[0].first == f_pairs[1].first)  return 0; // even case
    else return -1;  // wrong case
}


int NeuronGroup::MaxFireCount(){
    int max_count = -1;
    for(int i = 0; i < _neurons.size(); ++i){
        assert(_neurons[i]);
        max_count = max(_neurons[i]->FireCount(), max_count);
    }
    return max_count;
}

//* write the output synapse weights to the file
void NeuronGroup::WriteSynWeightsToFile(ofstream& f_out, int& index, const string& post_g){
    for(Neuron* neuron : _neurons){
        assert(neuron);
        neuron->WriteOutputWeights(f_out, index, post_g);
    }
    if(_b_neuron)   _b_neuron->WriteOutputWeights(f_out, index, post_g);
}


//* dump the spike times for the neurons in the group:
void NeuronGroup::DumpSpikeTimes(const string& filename){
    ofstream f_out(filename.c_str(),ios::app);
    if(!f_out.is_open()){
        cout<<"Cannot file : "<<filename<<endl;
        assert(f_out.is_open());
    }
    f_out<<"-1\t-1"<<endl;
    for(int i = 0; i < _neurons.size(); ++i){
        vector<int> spike_times;
        _neurons[i]->GetSpikeTimes(spike_times);
        for(int time : spike_times) 
            f_out<<i<<"\t"<<time<<endl;    
    }
    f_out<<"-1\t-1"<<endl;
    f_out.close();
}


//* dump the calcium levels of each neuron in the group:
void NeuronGroup::DumpCalciumLevels(ofstream & f_out){
    for(size_t i = 0; i < _neurons.size(); ++i){
        f_out<<"Neuron Index: "<<i<<"\tCalcium level: "<<_neurons[i]->GetCalcium()<<endl;
    }
}

void NeuronGroup::DumpVMems(ofstream & f_out){
    size_t size = 0;
    for(int i = 0; i < _neurons.size(); ++i){
        vector<double> v;
        _neurons[i]->GetWaveForm(v);
        if(i == 0)  size = v.size();
        else   assert(size == v.size());
        for(auto e : v)  f_out<<e<<"\t";
        f_out<<endl;
    }
}

void NeuronGroup::PrintSpikeCount(int cls){
    cout<<"The current speech class: "<<cls<<endl;
    for(int i = 0; i < _neurons.size(); ++i){
        cout<<"Neuron "<<i<<" fires "<<_neurons[i]->FireCount()<<endl;
    }
}

void NeuronGroup::LSMRemoveSpeech(){
    for(int i = 0; i < _neurons.size(); i++) _neurons[i]->LSMRemoveChannel();
}

// decrease the vth to enable more firings for the undesired neurons under reward based
// which might be helpful for depression
void NeuronGroup::LSMTuneVth(int index){
    assert((index >= 0)&&(index <_neurons.size()));
    for(int i = 0; i < _neurons.size(); i++) _neurons[i]->SetVth(LSM_V_THRESH);
    _neurons[index]->SetVth(LSM_V_THRESH);
}

void NeuronGroup::LSMSetTeacherSignal(int index){
    assert((index >= 0)&&(index < _neurons.size()));
    for(int i = 0; i < _neurons.size(); i++) _neurons[i]->SetTeacherSignal(-1);
    _neurons[index]->SetTeacherSignal(1);
    //  cout<<"Teacher Signal Set!!!"<<endl;
}

void NeuronGroup::LSMRemoveTeacherSignal(int index){
    assert((index >= 0)&&(index < _neurons.size()));
    //  _neurons[index]->SetTeacherSignal(0);
    for(int i = 0; i < _neurons.size(); i++) _neurons[i]->SetTeacherSignal(0);
    //  cout<<"Teacher Sginal Removed..."<<endl;
}


void NeuronGroup::LSMPrintInputSyns(ofstream & f_out){
    for(size_t i = 0; i < _neurons.size(); ++i){
        _neurons[i]->LSMPrintInputSyns(f_out);
    }
}

//* This function is to delete all the reservoir synapses in this neurongroup
void NeuronGroup::DestroyResConn(){
    for(size_t i = 0; i < _neurons.size(); ++i){
        _neurons[i]->DeleteSyn("in", 'r');
        _neurons[i]->DeleteSyn("out", 'r');
    }
}

//* This function is to print out the reservoir syns:
void NeuronGroup::PrintResSyn(ofstream & f_out){
    assert(f_out.is_open());
    for(size_t i = 0; i < _neurons.size(); ++i){
        _neurons[i]->PrintSyn(f_out,"in", 'r');
        _neurons[i]->PrintSyn(f_out,"out", 'r');
    }
}

//* Remove the outcoming synapses with weight zeros for each neuron
void NeuronGroup::RemoveZeroSyns(synapsetype_t syn_type){
    int cnt = 0;
    for(size_t i = 0; i < _neurons.size(); ++i){
        assert(_neurons[i]);
        cnt += _neurons[i]->RMZeroSyns(syn_type, "in");
        cnt += _neurons[i]->RMZeroSyns(syn_type, "out");
    }
    cout<<"The number of zero-out synapses: "<<cnt/2<<endl;
}

void NeuronGroup::SetGenerateTransient(bool t){
	for(int i=0;i<_neurons.size();i++){
		_neurons[i]->SetGenerateTransient(t);
	}
}
