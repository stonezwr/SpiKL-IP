#include <iostream>
#include <vector>
#include <assert.h>
#include <sys/stat.h>
#include "util_test.h"

using namespace std;


void UtilTest::testUnionFind(){
    vector<pair<int, int>> pairs({{3,4}, {4,9}, {8,0}, {2,3}, {5,6}, {5,9}, {7,3}, {4,8}, {6,1}});
    UnionFind uf(10);
    for(int i = 0; i < pairs.size(); ++i){
        int p = pairs[i].first, q = pairs[i].second;
        uf.add(p), uf.add(q);
        uf.unite(p, q);
    }
    if(uf.count() != 1){
        cout<<"In UtilTest::testUnionFind(), something wrong with the size counting!"
            <<"The size you have: "<<uf.count()<<endl;
        exit(EXIT_FAILURE);
    }
    vector<int> ids = uf.getGroupVec();
    vector<int> szs = uf.getSizeVec();
    bool pass = true;
    for(int i = 0; i < ids.size(); ++i){
        if(i == 0){
            pass = pass && (ids[i] == 8);
        }
        else
            pass = pass && (ids[i] == 3);
    }
    if(pass && szs[3] == 10)
        cout<<"UnionFind successfully passes the test!"<<endl;
    else
        cout<<"In UtilTest::testUnionFind(), wrong group result!"<<endl;
}


void UtilTest::testComputeAccSRM(){
    vector<int> pre_times = {161,177,193,212,237,254,436,447,456,466,477,490,500,512,526};
    vector<int> post_times = {15,22,29,34,41,47,54,61,67,73,79,85,92,98,105,112,118,125,131,137,143,149,156,162,168,174,181,189,198,208,220,235,344,375,390,400,413,430,450,476,491,501,511,521,530,541,551,558,569,577,585,594,602,612,620,628,642};
    vector<vector<double> >  res;
    res.push_back(ComputeAccSRM(pre_times, post_times, 4*32, 8, 32));
    res.push_back(ComputeAccSRM(pre_times, post_times, 4*64, 8, 64));
    res.push_back(ComputeAccSRM(pre_times, post_times, 4*64, 8, 64, 2));
    vector<vector<double> > true_res = {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.11565,0.41967,0.19824,0.46947,0.37729,0.57208,0.36648,0.65739,0.2566,0.12713,6.2227e-06,0,0,0,0,0.92679,1.6627,0.99802,0.77911,0.71383,0.7781,0.6108,0.45261,0.11164,0.027785,0.013667,0.0031121,0.0011447,0.00044087,0.00013666,5.4359e-05,1.4247e-05,5.2413e-06,1.826e-06
}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.11657,0.4419,0.20874,0.48835,0.405,0.60279,0.40144,0.71064,0.29689,0.50779,9.9582e-06,2.2139e-07,3.0285e-08,9.4458e-09,1.9442e-09,1.0248,2.0098,1.1292,0.84333,0.78193,0.84593,0.64374,0.50102,0.12229,0.029536,0.015129,0.0033408,0.001229,0.00047809,0.00014679,5.9689e-05,1.5471e-05,5.6915e-06,2.5622e-06}, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.33573,0.15859,0.30531,0.34481,0.44388,0.36318,0.61575,0.2883,0.52391,1.0187e-05,2.1499e-07,2.7398e-08,8.9879e-09,1.9154e-09,0.80903,1.9577,0.97695,0.65748,0.7074,0.75071,0.44967,0.46268,0.11063,0.023996,0.013972,0.0028444,0.0010464,0.00042131,0.00012497,5.3999e-05,1.3172e-05,4.8457e-06,2.4652e-06
}};
    for(int i = 0; i < true_res.size(); ++i){
        assert(true_res[i].size() == res[i].size());
        for(int j = 0; j < true_res[i].size(); ++j){
            assert(fabs(true_res[i][j] - res[i][j]) < 1e-4);
        }
    }
    cout<<"Function::ComputeAccSRM passes the simple test!"<<endl;
}

void UtilTest::testBuildDummyTimes(){
    vector<vector<int> > params = {{0, 10, true}, {53, 658, true}, {0, 10, false}, {53, 658, false}};
    vector<vector<int> > true_res = {{9}, {12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 168, 180, 192, 204, 216, 228, 240, 252, 264, 276, 288, 300, 312, 324, 336, 348, 360, 372, 384, 396, 408, 420, 432, 444, 456, 468, 480, 492, 504, 516, 528, 540, 552, 564, 576, 588, 600, 612, 624, 636, 648},{9},{657}};
    for(int i = 0; i < true_res.size(); ++i){
        vector<int> res = BuildDummyTimes(params[i][0], params[i][1], params[i][2]);
        assert(res == true_res[i]);
    }

    cout<<"Function::BuildDummyTimes passes the simple test!"<<endl;
}


void UtilTest::testMakeDirs(){
    string path = "tmp/tmp/tmp/train/";
    struct stat sb;
    if(stat(path.c_str(), &sb) == 0){
        string cmd = "rm -r " + path;
        int i = system(cmd.c_str()); 
    }
    MakeDirs(path); 
    assert(stat(path.c_str(), &sb) == 0);
    path = "./tmp";
    if(stat(path.c_str(), &sb) == 0){
        string cmd = "rm -r " + path;
        int i = system(cmd.c_str()); 
    }
    cout<<"Function::MakeDirs passes the simple test!"<<endl;
    
}

// inorder to run the test, please enable the DUMP_RESPONSE in the simulator to generate the 
// Waveform/ directory. Remember, only load one speech!
void UtilTest::testGetFilesEndWith(){
    string path = "test/";
    vector<string> res = GetFilesEndWith(path, ".sh");
    vector<string> true_res = {"run_test.sh"};
    for(size_t i = 0; i < res.size(); ++i)  assert(res[i] == true_res[i]);
    cout<<"Function::GetFilesEndWith passes the simple test!!"<<endl;
}

void UtilTest::testGetSpeechIndexClass(){
    vector<string> filenames = {"reservoir_0_0.dat", "hidden_0_100_9.dat", "output_291_10.dat"};
    vector<pair<int, int> > true_res = {{0, 0}, {100, 9}, {291, 10}};
    for(int i = 0; i < filenames.size(); ++i){
        int cls = -1, index = -1;
        GetSpeechIndexClass(filenames[i], cls, index);
        assert(cls == true_res[i].second && index == true_res[i].first);
    }
    cout<<"Function::GetSpeechIndexClass passes the simple test!!"<<endl;
}

void UtilTest::testReadMnistData(){
    Matrices x;
    vector<int> y;
    int ret = ReadMnistData(x, y, "mnist/t10k-images-idx3-ubyte", "mnist/t10k-labels-idx1-ubyte",  10000);
    assert(ret == 10000);
    assert(x.size() == 10000);
    assert(y.size() == 10000);
    assert(y[0] == 7);
    cout<<"Function::ReadMnistData passes the simple test!!"<<endl;

}

int main(int argc, char** argv){
    UtilTest ut;
    cout<<"======================= Begin testing ====================== "<<endl;
    cout<<"Testing the ComputeAccSRM: "<<endl;
    ut.testComputeAccSRM();

    cout<<"*********************************"<<endl;
    cout<<"Testing the BuildDummyTimes: "<<endl;
    ut.testBuildDummyTimes();

    cout<<"*********************************"<<endl;
    cout<<"Test the union find: "<<endl;
    ut.testUnionFind();
    
    cout<<"*********************************"<<endl;
    cout<<"Test the MakeDirs: "<<endl;
    ut.testMakeDirs();
    
    cout<<"*********************************"<<endl;
    cout<<"Test the GetFilesEndWith: "<<endl;
    ut.testGetFilesEndWith();

    cout<<"*********************************"<<endl;
    cout<<"Test the GetSpeechIndexClass: "<<endl;
    ut.testGetSpeechIndexClass();
 
    cout<<"*********************************"<<endl;
    cout<<"Test the ReadMnistData: "<<endl;
    ut.testReadMnistData();

    
    return 0;
}
