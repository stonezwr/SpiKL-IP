/*
   Utility function for supporting
Author: Yingyezhe Jin
Date: Sept 8, 2016
*/
#ifndef UTIL_H
#define UTIL_H

#include <sys/time.h>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <iterator>
#include <assert.h>

typedef std::vector<std::vector<std::vector<float> > > Matrices;
typedef std::vector<std::vector<float> > Matrix;
class Timer{
private:
    struct timeval m_start, m_cur, m_end;
    struct timezone m_zone;
    bool is_started;
public:
    Timer();
    void Start();
    void Report(std::string str);
    void End(std::string str="");
};

class UnionFind{
private:
    int _n;
    int _count;
    std::vector<int> _sz;
    std::vector<int> _id;

public:
    UnionFind(int n);
    int root(int ind);
    bool individual(int ind);
    bool exist(int ind);
    void add(int ind);
    bool find(int p, int q);
    void unite(int p, int q);
    int count(){return _count;}
    int size(){return _n;}
    std::vector<int> getGroupVec(){return _id;}
    std::vector<int> getSizeVec(){return _sz;}
};

std::vector<double> ComputeAccSRM(const std::vector<int>& pre_times, const std::vector<int>& post_times, const int window, const double ts, const double tm, const double t_ref = 0.0);
std::vector<int> BuildDummyTimes(int max_count, int end_time, bool is_target);
void MakeDirs(std::string dir);

std::vector<std::string> GetFilesEndWith(std::string path, const std::string& suffix);
void file_finder(const std::string& path, std::vector<std::string>& filename, const std::string& suffix);
void GetSpeechIndexClass(std::string filename, int& cls, int& index);

int ReadMnistData(Matrices & x, std::vector<int>& y, std::string xpath, std::string ypath, int nums);

//* reload the + for the vectors:
template<class T> std::vector<T> operator+(const std::vector<T>& a, const std::vector<T>& b)
{
    if(a.empty())   return b;
    if(b.empty())   return a;
    assert(a.size() == b.size());
    std::vector<T> res;
    res.reserve(a.size());
    std::transform(a.begin(), a.end(), b.begin(), std::back_inserter(res), std::plus<T>());
    return res;
}


//* reload the * for the vectors:
template<class T> std::vector<T> operator*(const T c, const std::vector<T> & a)
{
    std::vector<T> res;
    res.reserve(a.size());
    std::transform(a.begin(), a.end(), std::back_inserter(res), std::bind1st(std::multiplies<T>(), c));
    return res;
}


//* reload the << for print the vector
template<class T> std::ostream& operator<<(std::ostream& out, const std::vector<T>& v)
{
    if(!v.empty()){
        out << '[';
        std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, ", "));
        out << "\b\b]";
    }
    return out;
}


#endif
