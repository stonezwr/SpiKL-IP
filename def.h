#ifndef DEF_H
#define DEF_H

// this is the prob. for the random connections
#define CONNECTION_PROB 0.2

// this control variable enable you to remove the zero reservoir weights
// after reservoir training
#define _RM_ZERO_RES_WEIGHT

// calcium parameters
#define TAU_C		64

#define LSM_T_M_C 64 
#define LSM_T_SYNE 8
#define LSM_T_SYNI 2
#define LSM_T_FO 8
#define LSM_T_REFRAC 2
#define LSM_V_REST 0
#define LSM_V_RESET 0
#define LSM_V_THRESH 20
#define LSM_CAL_MID 6
#define LSM_CAL_BOUND 3
#define LSM_CAL_MARGIN 0
#define LSM_DELTA_POT 0.006
#define LSM_DELTA_DEP 0.006
#define ITER_SEARCH_CONV 25.0
#define CLS 26
#define NUM_THREADS 5
#define NUM_ITERS 500

#define LSM_TBIT_SYNE 1
#define LSM_TBIT_SYNI 3


#define LOST_RATE 0.0
#define LIQUID_SYN_MODIFICATION 1

#define NFOLD 5

#define SPIKL_IP

#define SAMPLES_PER_CLASS 10

enum channelmode_t {INPUTCHANNEL,RESERVOIRCHANNEL, OUTPUTCHANNEL}; // for allocate speech channels
enum neuronmode_t {DEACTIVATED,READCHANNEL,WRITECHANNEL,NORMAL}; // for implement network stat.
enum networkmode_t {TRANSIENTSTATE,READOUT,VOID}; // different network mode

enum synapsetype_t {RESERVOIR_SYN, INPUT_SYN, READOUT_SYN, INVALID}; // different synaptic type


#endif


