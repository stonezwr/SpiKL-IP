/**************************************************************
 * This is a simple function to count the weight distribution.
 * This function will first read 'weight.txt' and count
 *************************************************************/

#include <iostream>
#include <unordered_map>
#include <fstream>
#include <utility>
#include <assert.h>

using namespace std;
int main(int argc, char ** argv){
  ifstream f_in("weights.txt");
  if(!f_in.is_open()){
    cout<<"Cannot open file: weights.txt\n"
	<<"This file might not exist!"<<endl;
    assert(f_in.is_open());
  }

  unordered_map<int, int> my_hash;
  int w;
  while(f_in>>w){
    if(my_hash.find(w) == my_hash.end()){
      my_hash.insert(make_pair(w, 1));
    }
    else{
      ++my_hash[w];
    }
  }
  
  // print out the results:
  for(auto p : my_hash){
    cout<<"Weight "<<p.first<<" has "<<p.second<<endl;
  }
  return 0;
}
