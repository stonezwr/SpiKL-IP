#include "neuron.h"
#include "synapse.h"
#include "parser.h"
#include "network.h"
#include "simulator.h"
#include "readout.h"
#include "util.h"
#include <sys/time.h>
#include <iostream>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <algorithm>
#include <iterator>

using namespace std;
int file[4142];

struct NTParg{
    long tid;
    Network * network;
};

void * ParallelSim(void * NTPargptr){

    NTParg * arg = (NTParg *)NTPargptr;
    Network * network = arg->network;
    long tid = arg->tid;
    delete arg;

    Simulator simulator(network);

    (network)->AnalogToSpike();   // application for speech
    simulator.LSMRun(tid);
    cout<<"Thread "<<tid<<" done!"<<endl;
    pthread_exit(NULL);
}

void PrintResultsToFile(const vector<int>& r_correct, const vector<int>& r_wrong, const vector<int>& r_even, int n_speeches){
    assert(!r_correct.empty() && !r_wrong.empty() && !r_even.empty());

    ofstream f_c("outputs/rates.txt");
    ofstream f_w("outputs/errors.txt");
    ofstream f_e("outputs/evens.txt");
    if(!f_c.is_open()){
        cout<<"Cannot open: outputs/rates.txt"<<endl;
        return;
    }
    for(size_t i = 0; i < r_correct.size(); ++i){
        f_c<<r_correct[i]<<endl;
        f_w<<r_wrong[i]<<endl;
        f_e<<r_even[i]<<endl;			
        assert(r_correct[i] + r_wrong[i] + r_even[i] == n_speeches);
    }
    f_c<<endl, f_w<<endl, f_e<<endl;
    f_c.close(), f_w.close(), f_e.close();
}


void CollectResultsFromNetwork(Network array_network[]){
    int num_speeches = array_network[0].NumSpeech();
    int num_iters = array_network[0].NumIteration();
    vector<int> num_each_speech = array_network[0].NumEachSpeech();

    vector<int> r_correct(num_iters, 0);
    vector<int> r_wrong(num_iters, 0);
    vector<int> r_even(num_iters, 0);
    vector<vector<int> > r_correct_bd(num_iters, vector<int>(CLS, 0)); 

    for(int i = 0; i < NUM_THREADS; ++i)
        array_network[i].MergeReadoutResults(r_correct, r_wrong, r_even, r_correct_bd);

    auto max_rate_iter = max_element(r_correct.begin(),r_correct.end());
    int max_rate = *max_rate_iter;
    int max_ind = distance(r_correct.begin(), max_rate_iter);
    cout<<"Best performance achieved @"<<max_ind<<" = "<<max_rate<<"/"<<num_speeches<<" = "<<double(max_rate)*100/double(num_speeches)<<'%'<<endl;
    if(r_correct.size() >= 50)
        cout<<"Performance at 50th iteration = "<<double(r_correct[49])*100/double(num_speeches)<<'%'<<endl;
    if(r_correct.size() >= 100)
        cout<<"Performance at 100th iteration = "<<double(r_correct[99])*100/double(num_speeches)<<'%'<<endl;
    if(r_correct.size() >= 200)
        cout<<"Performance at 200th iteration = "<<double(r_correct[199])*100/double(num_speeches)<<'%'<<endl;
    if(r_correct.size() >= 300)
        cout<<"Performance at 300th iteration = "<<double(r_correct[299])*100/double(num_speeches)<<'%'<<endl;

    assert(num_each_speech.size() == r_correct_bd[max_ind].size());
    for(int i = 0; i < num_each_speech.size(); ++i){
        if(num_each_speech[i] != 0)
            cout<<"Performance for "<<i<<"th class = "<<r_correct_bd[max_ind][i]*100/double(num_each_speech[i])<<'%'<<endl;
    }
    PrintResultsToFile(r_correct, r_wrong, r_even, num_speeches);
}


void PrintTestErrorToFile(const vector<double>& test_errors, int num_speeches){
    assert(!test_errors.empty());
    ofstream f_out("outputs/test_errors.txt");
    for(double e : test_errors) f_out<<e/num_speeches<<endl;
    f_out.close();
}


void CollectTestErrorFromNetwork(Network array_network[]){
    int num_speeches = array_network[0].NumSpeech();
    int num_iters = array_network[0].NumIteration();
    vector<double> test_errors(num_iters, 0);
    for(int i = 0; i < NUM_THREADS; ++i)
        array_network[i].MergeTestErrors(test_errors);
    PrintTestErrorToFile(test_errors, num_speeches);        
}


int main(int argc, char * argv[]){
    struct timeval val1,val2;
    struct timezone zone;
    struct NTParg* NTPargptr;
    int rc;
    int i;
    void * status;
    long t;

    gettimeofday(&val1,&zone);
    //  srand(time(NULL));
    for(i = 0; i < 4142; ++i) file[i] = -1;

    Network array_network[NUM_THREADS];

    for(i = 0; i < NUM_THREADS; i++){
        array_network[i].SetTid(i);
        Parser parser(&array_network[i]);
        parser.Parse("netlist.txt"); // For English spoken letters
        array_network[i].CrossValidation(NFOLD);
    }

    pthread_t threads[NUM_THREADS];
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);

    for(t = 0; t < NUM_THREADS; t++){
        NTPargptr = new(struct NTParg);
        NTPargptr->tid = t;
        NTPargptr->network = &array_network[t];

        printf("In main: creating thread %ld \n",t);
        rc = pthread_create(&threads[t],&attr,ParallelSim,(void *) NTPargptr);
        if(rc){
            printf("ERROR; return code form pthread_create() is %d \n",rc);
            exit(-1);
        }
    }

    pthread_attr_destroy(&attr);
    for(t = 0; t < NUM_THREADS; t++){
        rc = pthread_join(threads[t],&status);
        if(rc){
            printf("ERROR; return code form pthread_join() is %d \n",rc);
            exit(-1);
        }
        printf("Main: completed joining with thread %ld ! \n",t);
    }
    printf("Main: program completed. Exiting. \n");

    gettimeofday(&val2,&zone);
    cout<<"Wall clock time: "<<((val2.tv_sec-val1.tv_sec)+double(val2.tv_usec-val1.tv_usec)*1e-6)<<" seconds"<<endl;

    cout<<"Readout the results ..."<<endl; 
    cout<<"Number of samples: "<<array_network[0].NumSpeech()<<endl;
    // collect the result directly from network:
    CollectResultsFromNetwork(array_network);
    // collect the test error at each iteration
    CollectTestErrorFromNetwork(array_network);

    pthread_exit(NULL);

    return 0;
}






