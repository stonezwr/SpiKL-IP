#include "def.h"
#include "parser.h"
#include "synapse.h"
#include "neuron.h"
#include "network.h"
#include "speech.h"
#include "channel.h"
#include "util.h"
#include <cstdlib>
#include <iostream>
#include <vector>
#include <set>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>
#include <random>
#include <math.h>

using namespace std;


Parser::Parser(Network * network){
    _network = network;
}

void Parser::Parse(const char * filename){
    int mnist_duration = 500; // the default mnist duration (duration of the possion spike train)
    int i;
    char linestring[8192];
    char ** token = new char*[64];
    FILE * fp = fopen(filename,"r");
    assert(fp != NULL);

    assert(fp != 0);
    while(fgets(linestring, 8191, fp) != NULL){
        if(strlen(linestring) <= 1) continue;
        if(linestring[0] == '#') continue;

        token[0] = strtok(linestring," \t\n");
        assert(token[0] != NULL);

       if(strcmp(token[0],"neurongroup")==0){
            // neurongroup name number
            token[1] = strtok(NULL," \t\n");
            assert(token[1] != NULL);
            token[2] = strtok(NULL," \t\n");
            assert(token[2] != NULL);
            token[3] = strtok(NULL," \t\n");
            assert(token[3] != NULL);
            // read the v_mem from the netlist if applicable
            token[4] = strtok(NULL," \t\n");
            double v_mem = token[4] == NULL ? LSM_V_THRESH : atof(token[4]);
            ParseNeuronGroup(token[1], atoi(token[2]), token[3], v_mem);
        }else if(strcmp(token[0],"column")==0){
            token[1] = strtok(NULL," \t\n");
            assert(token[1] != NULL);
            token[2] = strtok(NULL," \t\n");
            assert(token[2] != NULL);
            token[3] = strtok(NULL," \t\n");
            assert(token[3] != NULL);
            token[4] = strtok(NULL," \t\n");
            assert(token[4] != NULL);

            assert(_network->CheckExistence(token[1]) == false);
            int dim1 = atoi(token[2]);
            int dim2 = atoi(token[3]);
            int dim3 = atoi(token[4]);
            assert((dim1>0)&&(dim2>0)&&(dim3>0));
            _network->LSMAddNeuronGroup(token[1],dim1,dim2,dim3);
        }else if(strcmp(token[0],"lsmsynapse")==0){
            token[1] = strtok(NULL," \t\n");
            assert(token[1] != NULL);
            token[2] = strtok(NULL," \t\n");
            assert(token[2] != NULL);
            token[3] = strtok(NULL," \t\n");
            assert(token[3] != NULL);
            token[4] = strtok(NULL," \t\n");
            assert(token[4] != NULL);
            token[5] = strtok(NULL," \t\n");
            assert(token[5] != NULL);
            token[6] = strtok(NULL," \t\n");
            assert(token[6] != NULL);
            token[7] = strtok(NULL," \t\n");
            assert(token[7] != NULL);

            assert(_network->CheckExistence(token[1]) == true);
            assert(_network->CheckExistence(token[2]) == true);

            ParseLSMSynapse(token[1],token[2],atoi(token[3]),atoi(token[4]),atoi(token[5]),atoi(token[6]),token[7]);
        }else if(strcmp(token[0],"speech")==0){
            token[1] = strtok(NULL," \t\n");
            assert(token[1] != NULL);
            token[2] = strtok(NULL," \t\n");
            assert(token[2] != NULL);

            ParseSpeech(atoi(token[1]),token[2]);
        }else if(strcmp(token[0],"end")==0){
            break;
        }else{
            cout<<token[0]<<endl;
            assert(0);
        }

    }
    fclose(fp);
}

void Parser::ParseNeuronGroup(char * name, int num, char * e_i, double v_mem){
	cout<<std::string(name)<<endl;
    assert(_network->CheckExistence(name) == false);
    assert(num > 1);
    assert((strcmp(e_i,"excitatory")==0)||(strcmp(e_i,"inhibitory")==0));
    bool excitatory;
    if(strcmp(e_i,"excitatory") == 0) excitatory = true;
    else excitatory = false;
    _network->AddNeuronGroup(name, num, excitatory, v_mem);
}


void Parser::ParseLSMSynapse(char * from, char * to, int fromN, int toN, int value, int random, char * fixed_learning){
    bool fixed;
    if(strcmp(fixed_learning,"fixed") == 0) fixed = true;
    else if(strcmp(fixed_learning,"learning") == 0) fixed = false;
    else{
        cout<<fixed_learning<<endl;
        assert(0);
    }
    _network->LSMAddSynapse(from, to, fromN, toN, value, random, fixed);
}

void Parser::ParseSpeech(int cls, char* path){
	if(path==NULL){
		cout<<"directory path is NULL"<<endl;
		assert(0);
	}
	//check if path is a valid dir
	struct stat s;
	lstat(path,&s);
	if(!S_ISDIR(s.st_mode)){
		cout<<"path is not a valid directory"<<endl;
		assert(0);
	}
	struct dirent* filename;
	DIR *dir;
	dir=opendir(path);
	if(dir==NULL){
		cout<<"Cannot open dir"<<endl;
		assert(0);
	}
	int file_read=0;
	while((filename=readdir(dir))!=NULL&&file_read<SAMPLES_PER_CLASS){
		const size_t len = strlen(path)+strlen(filename->d_name);
		char *file_path=new char[len+1];
		strcpy(file_path,path);
		if(filename->d_name[0]<'a'||filename->d_name[0]>'z'){
			continue;
		}
		strcat(file_path,filename->d_name);
		Speech * speech = new Speech(cls);
		Channel * channel;
		char linestring[8192];
		char * token;
		FILE * fp = fopen(file_path,"r");
		if(fp==NULL){
			assert(0);	
		}
		_network->AddSpeech(speech);
		int index = 0;
		while(fgets(linestring,8191,fp)!=NULL){
			if(strlen(linestring) <= 1) continue;
			channel = speech->AddChannel(10, 1, index++);
			token = strtok(linestring," \t\n");
			while(token != NULL){
				channel->AddAnalog(atof(token));
				token = strtok(NULL," \t\n");
			}
		}
		fclose(fp);
		delete token;
		file_read++;
	}
	closedir(dir);
}

