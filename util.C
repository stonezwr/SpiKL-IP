/*
    Implementation of supporting functions
    Author: Yingyezhe Jin
    Date: Sept 8, 2016
*/
#include "util.h"
#include <assert.h>
#include <utility>
#include <cmath>
#include <sys/stat.h>
#include <dirent.h>
#include <sstream>
#include <fstream>

Timer::Timer(){
    gettimeofday(&m_start, &m_zone);
    is_started = false;
}

void Timer::Start(){
    gettimeofday(&m_start, &m_zone);
    m_cur = m_start;
    is_started = true;
}

void Timer::Report(std::string str){
    timeval tmp;
    gettimeofday(&tmp, &m_zone);
    if(is_started){
        std::cout<<"Total time spent in "<<str<<" : "<<(tmp.tv_sec - m_cur.tv_sec) + double((tmp.tv_usec - m_cur.tv_usec)*1e-6)<<" seconds"<<std::endl;
    }
    else{
        std::cerr<<"Warning: the util Timer is not STARTED! Start the timer...."<<std::endl;
        Start();
        return;
    }
    m_cur = tmp;
}

void Timer::End(std::string str){
    timeval tmp;
    gettimeofday(&tmp, &m_zone);
    if(is_started && !str.empty()){
        std::cout<<"Total time spent in "<<str<<" : "<<(tmp.tv_sec - m_start.tv_sec) + double((tmp.tv_usec - m_start.tv_usec)*1e-6)<<" seconds"<<std::endl;
    }
    else if(!is_started){
        std::cerr<<"Warning: the util Timer is not STARTED! Start the timer...."<<std::endl;
        Start();
        return;
    }  
    m_cur = tmp;
    m_end = tmp;
}


UnionFind::UnionFind(int n):_n(n), _count(0)
{
    _sz = std::vector<int>(_n, -1);
    _id = std::vector<int>(_n, -1);
}

int UnionFind::root(int ind){
    assert(exist(ind));
    while(ind != _id[ind]){
        _id[ind] = _id[_id[ind]]; // path compression
        ind = _id[ind];
    }
    return ind;
}

// check if the group has one element
bool UnionFind::individual(int ind){
    if(ind >= _n){
        std::cout<<"Warning: UnionFind:: index "<<ind<<" out of bound "<<_n<<std::endl;
        return false;
    }
    return _sz[ind] == 1;
}

bool UnionFind::exist(int ind){
    if(ind >= _n){
        std::cout<<"Warning: UnionFind::exist() index "<<ind<<" out of bound "<<_n<<std::endl;
        return -1;
    }
    return (_sz[ind] != -1);
}

void UnionFind::add(int ind){
    if(ind >= _n){
        std::cout<<"Warning: UnionFind::add, index: "<<ind<<" out of bound "<<_n<<std::endl;
        return;
    }
    if(exist(ind))  return;
    _id[ind] = ind;
    _sz[ind] = 1;
    _count++;
}

bool UnionFind::find(int p, int q){
    return root(p) == root(q);
}

void UnionFind::unite(int p, int q){
    int i = root(p);
    int j = root(q);
    if(_sz[i] >= _sz[j]){
        _sz[i] += _sz[j]; _id[j] = i; // always merge the smaller to the larger one
    }
    else{
        _sz[j] += _sz[i]; _id[i] = j;
    } 
    _count--;
}


//* Compute the accumulative synapse effect based on the timings 
//* See the equations in Book: Spiking Neuron Model Page 109
//* t_ref: the refactory period
std::vector<double> ComputeAccSRM(const std::vector<int>& pre_times, const std::vector<int>& post_times, const int window, const double ts, const double tm, const double t_ref){
    std::vector<double> effects;
    for(int i = 0; i < post_times.size(); ++i){
        double effect = 0;
        // notice that the initial time starts from 1
        // add t_ref to consider the effect of refractory period
        int last_post = i == 0 ? 1 : post_times[i-1]+t_ref;        
        int cur_post = post_times[i];
        auto lb = std::lower_bound(pre_times.begin(), pre_times.end(), std::max(1, cur_post - window));
        auto ub = std::upper_bound(pre_times.begin(), pre_times.end(), cur_post);
        for(auto it = lb; it != ub; ++it){
            int pre_time = (*it) + t_ref;
            if(pre_time > cur_post) continue;
            int s = cur_post - last_post;
            int t = cur_post - pre_time;
            double factor = exp(-1*std::max(t - s, 0)/ts)/(1 - ts/tm);
            effect += factor*(exp(-1*std::min(s, t)/tm) - exp(-1*std::min(s, t)/ts));
        }
        effects.push_back(effect);
    }
    return effects;
}


//* Build the dummy firing times for the targeted neuron with zero spike cnt
//* for the target neuron, the fire count after changing should be max_count
//* for the neuron that is not the target, the fire count after chaning should be 4
std::vector<int> BuildDummyTimes(int max_count, int end_time, bool is_target){
    std::vector<int> v;
    int interval = max_count == 0 || !is_target ? end_time - 1 : end_time/max_count;
    for(int i = interval; i < end_time; i += interval){
        v.push_back(i);
    }
    return v;
}


//* Make the directories using system function
void MakeDirs(std::string dir){
    struct stat sb;
    std::string base = ".";
    std::stringstream iss(dir);
    std::string sub_dir;
    // decode the string:
    while(std::getline(iss, sub_dir, '/')){
        std::string cmd;
        std::string path = base + "/" + sub_dir;
        if(stat(path.c_str(), &sb) != 0){
            cmd = "mkdir " + path;
            std::cout<<"Creating path: "<<path<<std::endl;
            int i = system(cmd.c_str());
        }
        base = base + "/" + sub_dir;
    }
}

//* get the files recursively in a specific directory that is end is certain suffix
std::vector<std::string> GetFilesEndWith(std::string path, const std::string& suffix){
    struct stat sb;
    if(stat(path.c_str(), &sb) != 0){
        std::cout<<"The given path: "<<path<<" does not exist!"<<std::endl;
        exit(EXIT_FAILURE);
    }
    if(path.back() == '/')  path.pop_back();

    std::vector<std::string> filenames;
    file_finder(path, filenames, suffix);
    return filenames;
}

//* recursively find the files
void file_finder(const std::string& path, std::vector<std::string>& filenames, const std::string& suffix){
    DIR *dir;
    struct dirent *ent; 
    struct stat st;

    dir = opendir(path.c_str());
    while((ent = readdir(dir)) != NULL){
        std::string file_name = ent->d_name;
        std::string full_file_name = path + "/" + file_name;
        if(file_name[0] == '.') continue;
    
        if(stat(full_file_name.c_str(), &st) == -1) continue;

        bool is_directory = (st.st_mode & S_IFDIR) != 0;

        if(is_directory)    file_finder(full_file_name, filenames, suffix);
    
        if(file_name.length() >= suffix.length() && file_name.substr(file_name.length() - suffix.length()) == suffix)
            filenames.push_back(file_name);
    }
}

//* given a dumped speech name, get the class and index information
//* the speech name is like: reservoir_spikes_[index]_[cls].dat
void GetSpeechIndexClass(std::string filename, int& cls, int& index){
    size_t pos = filename.find_last_of("_");
    if(pos == std::string::npos){
        std::cout<<"Invalid spike response filename: "<<filename<<std::endl;
        exit(EXIT_FAILURE);
    }
    cls = std::stoi(filename.substr(pos + 1));
    
    filename = filename.substr(0, pos);
    pos = filename.find_last_of("_");
    if(pos == std::string::npos){
        std::cout<<"Invalid spike response filename: "<<filename<<std::endl;
        std::cout<<"Cannot find the index before the class id!"<<std::endl;
        exit(EXIT_FAILURE);
    }
    index = std::stoi(filename.substr(pos + 1));
}

//* reverse the int
int ReverseInt (int i){
	unsigned char ch1, ch2, ch3, ch4;
	ch1 = i & 255;
	ch2 = (i >> 8) & 255;
	ch3 = (i >> 16) & 255;
	ch4 = (i >> 24) & 255;
	return((int) ch1 << 24) | ((int)ch2 << 16) | ((int)ch3 << 8) | ch4;
}

//* read the MNIST sample
int ReadMnistSample(std::string filename, Matrices& vec, int num)
{
    std::ifstream file(filename.c_str(), std::ios::binary);
    if(!file.is_open()){
        std::cout<<"Cannot open the file: "<<filename<<std::endl;
        exit(EXIT_FAILURE);
    }
    int magic_number = 0;
    int number_of_images = 0;
    int n_rows = 0;
    int n_cols = 0;
    file.read((char*) &magic_number, sizeof(magic_number));
    magic_number = ReverseInt(magic_number);
    file.read((char*) &number_of_images,sizeof(number_of_images));
    number_of_images = ReverseInt(number_of_images);
    if(number_of_images >= num){
        number_of_images = num;
    }
    else{
        std::cout<<"ReadMnistSample::number of images "<<num<<" is overflow!"<<std::endl;
        exit(EXIT_FAILURE);
    }
    file.read((char*) &n_rows, sizeof(n_rows));
    n_rows = ReverseInt(n_rows);
    file.read((char*) &n_cols, sizeof(n_cols));
    n_cols = ReverseInt(n_cols);
    for(int i = 0; i < number_of_images; ++i){
        Matrix tpmat = Matrix(n_rows, std::vector<float>(n_cols, 1));
                    
        for(int r = 0; r < n_rows; ++r){
            for(int c = 0; c < n_cols; ++c){
                unsigned char temp = 0;
                file.read((char*) &temp, sizeof(temp));        
                tpmat[r][c] = (float)temp / (5.5*255.0f);
                //if(i == 0)  std::cout<<tpmat[r][c]<<"\t";
            }
            //if(i == 0)  std::cout<<std::endl;
        }
        vec.push_back(tpmat);
    }
    return vec.size();
}

//* read the lable
int ReadMnistLabel(std::string filename, std::vector<int> &mat){
    std::ifstream file(filename.c_str(), std::ios::binary);
    if(!file.is_open()){
        std::cout<<"Cannot open the file: "<<filename<<std::endl;
        exit(EXIT_FAILURE);
    }
    int magic_number = 0;
    int number_of_images = 0;
    file.read((char*) &magic_number, sizeof(magic_number));
    magic_number = ReverseInt(magic_number);
    file.read((char*) &number_of_images,sizeof(number_of_images));
    number_of_images = ReverseInt(number_of_images);

    for(int i = 0; i < number_of_images; ++i){
        unsigned char temp = 0;
        file.read((char*) &temp, sizeof(temp));
        mat.push_back(temp);
    }
    return mat.size();
}

//* read trainning data and lables of the mnist 
int  ReadMnistData(Matrices& x, std::vector<int>& y, std::string xpath, std::string ypath, int nums)
{
    //* read MNIST image
    int len = ReadMnistSample(xpath, x, nums);
    //* read MNIST label
    int t = ReadMnistLabel(ypath, y);
    return t;
}


