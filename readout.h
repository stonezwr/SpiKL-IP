#ifndef READOUT_H
#define READOUT_H

#include <vector>


class Readout{
private:
    int num_of_speeches;
    // std::vector<int> rates;  //Vector storing recognition rates
    // std::vector<int> errors; 
    // std::vector<int> ties;
    std::vector<int> refer;  //Vector storing the speech class
    int num_iteration;
    std::vector<int> multidata1; //Vector storing the raw results for a single speech result file (column 1)
    std::vector<int> multidata2; //Vector storing the raw results for a single speech result file(column 2)
    // std::vector<int> data1; //Vector storing the raw results (column 1)
    // std::vector<int> data2; //Vector storing the raw results (column 2)
    std::vector<int> indices;  //Vector storing the indices that corresponds to -1 (separation marks)
public:
    Readout(char *);
    Readout(int );
    void Multireadout();
    void SetRefer(int flag);
    void LoadData(char *); //Load the data from the file of speech results into multidata 
    std::vector<int> FindVal(const std::vector<int> &, int);  //Find a specific value in the vector
};

#endif
