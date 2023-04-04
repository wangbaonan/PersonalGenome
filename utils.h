//
//  utils.h
//  PersonalGenome
//
//  Created by wangbaonan on 03/07/2022.
//

#ifndef utils_h
#define utils_h

#include <vector>
#include <algorithm>
#include <sstream>
#include <execinfo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>  // for opendir
#include <iterator>
#include <cmath> // for ceil in file size recognise
#include <filesystem> // for file size , exist check
#include <fstream>  // for file size check
#include <cstdint> // check file exist
#include <string> // to_string
#include <curl/curl.h> // curl related function
#include <stdexcept> // throw runtime error in endmapping , read delta file 
#include <limits.h>
#include <string.h>
#include "logger.h"

//Module Switch 按位与操作即可
//Wangbaonan
#define NGS_SWITCH 1 // 00000001
#define PCA_SWITCH 2 // 00000010
#define AS2_SWITCH 4 // 00000100
#define HLA_SWITCH 8 // ...
#define MTY_SWITCH 16 
#define ADMIXTURE_SWITCH 32
#define PRS_SWITCH 64
#define SNPEDIA_SWITCH 128 // 可以放入 Config
#define SV_SWITCH 256
#define PROVINCE_SWITCH 512
// 9 modules all 510 no NGS 
#define ALL_MODULE 1022 // Include Province:1022 No Province: 510 

#define CMD_RESULT_BUF_SIZE 1024

namespace fs = std::filesystem;

inline uint64_t r_pos(std::string T, std::string P, int n){ //反向查找字符串第n个字符
	if(n==0) return std::string::npos; //相当于没找
	int count = 0;
	unsigned begined = T.length() - 1;
    //Logger::get().info() << "T :" << T;
    //Logger::get().info() << "P :" << P;
    //Logger::get().info() << "begined :" << begined;
	while(begined > 0){
        if (T.substr(begined,1).compare(P) == 0 ){
            count++;
        }
        if(n == count){
			return begined;
		}
        begined--;
	}
	return std::string::npos; //意味着没有那么多次
}

inline int ExecuteCMD(const char *cmd, char *result)
{
    int iRet = -1;
    char buf_ps[CMD_RESULT_BUF_SIZE];
    char ps[CMD_RESULT_BUF_SIZE] = {0};
    FILE *ptr;

    strcpy(ps, cmd);

    if((ptr = popen(ps, "r")) != NULL)
    {
        while(fgets(buf_ps, sizeof(buf_ps), ptr) != NULL)
        {
           strcat(result, buf_ps);
           if(strlen(result) > CMD_RESULT_BUF_SIZE)
           {
               break;
           }
        }
        pclose(ptr);
        ptr = NULL;
        iRet = 0;  // 处理成功
    }
    else
    {
        printf("popen %s error\n", ps);
        iRet = -1; // 处理失败
    }

    return iRet;
}

inline std::string SystemWithResult(const char *cmd)
{
    char cBuf[CMD_RESULT_BUF_SIZE] = {0};
    std::string sCmdResult;

    ExecuteCMD(cmd, cBuf);
    sCmdResult = std::string(cBuf); // char * 转换为 string 类型
    sCmdResult.erase(std::remove(sCmdResult.begin(), sCmdResult.end(), '\n'), sCmdResult.end());
    //printf("CMD Result: \n%s\n", sCmdResult.c_str());

    return sCmdResult;
}

template<class T>
void vecRemove(std::vector<T>& v, T val)
{
    v.erase(std::remove(v.begin(), v.end(), val), v.end());
}

template<typename T>
T quantile(const std::vector<T>& vec, int percent)
{
    if (vec.empty()) return 0;
    //NOTE: there's a bug in libstdc++ nth_element,
    //that sometimes leads to a segfault. This is why
    //we have this inefficient impleemntation here
    //std::nth_element(vec.begin(), vec.begin() + vec.size() / 2,
    //                 vec.end());
    auto sortedVec = vec;
    std::sort(sortedVec.begin(), sortedVec.end());
    size_t targetId = std::min(vec.size() * (size_t)percent / 100,
                               vec.size() - 1);
    return sortedVec[targetId];
}

template<typename T>
T median(const std::vector<T>& vec)
{
    return quantile(vec, 50);
}

inline std::vector<std::string>
splitString(const std::string &s, char delim)
{
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) elems.push_back(item);
    return elems;
}

inline bool fileExists(const std::string& path)
{
    std::ifstream fin(path);
    return fin.good();
}

inline void segfaultHandler(int signal __attribute__((unused)))
{
    void *stackArray[20];
    size_t size = backtrace(stackArray, 10);
    Logger::get().error() << "Segmentation fault! Backtrace:";
    char** backtrace = backtrace_symbols(stackArray, size);
    for (size_t i = 0; i < size; ++i)
    {
        Logger::get().error() << "\t" << backtrace[i];
    }
    abort();
}

inline void exceptionHandler()
{
    static bool triedThrow = false;
    try
    {
        if (!triedThrow)
        {
            triedThrow = true;
            throw;
        }
    }
    catch (const std::exception &e)
    {
        Logger::get().error() << "Caught unhandled exception: " << e.what();
    }
    catch (...) {}

    void *stackArray[20];
    size_t size = backtrace(stackArray, 10);
    char** backtrace = backtrace_symbols(stackArray, size);
    for (size_t i = 0; i < size; ++i)
    {
        Logger::get().error() << "\t" << backtrace[i];
    }
    abort();
}

/*
struct pathhandle{
    pathhandle(std::string& fullpath,std::string& dirpath, std::string& filename):
    fullpath(fullpath), dirpath(dirpath), filename(filename)
    {}
    std::string fullpath;
    std::string dirpath;
    std::string filename;
};

inline pathhandle processpath(const std::string& strings){
    size_t dotPos = strings.rfind(".");
    size_t slash = strings.find_last_of("/");
    if(dotPos == std::string::npos || slash == std::string::npos) return strings;
    std::string dirpath = strings.substr(0, slash);
    std::string filename = std::string(&strings[slash+1], &strings[dotPos]);
    pathhandle paths(strings, dirpath, filename);
    return paths;
}
*/
struct FilesInfo{
    FilesInfo(std::vector<std::string>& filenames, uint16_t& filenumbers,std::string& totalsize):
    filenames(filenames),filenumbers(filenumbers),totalsize(totalsize)
    {}
    std::vector<std::string> filenames;
    uint16_t filenumbers;
    std::string totalsize;
};

//*******************NEED*********************
class FileContainer
{
public:
    FileContainer(std::string& dirname, std::string& suffix):
    _dirname(dirname), _suffix(suffix), _filecount(0)
    {}
    void getfilenames(){      // 下面的代码用来从给定的input文件夹中读取所有的文件名，并根据后缀进行提取。
        std::string dirpath;
        DIR *dpdf = opendir(_dirname.c_str());
        struct dirent *epdf;
        size_t slash = _dirname.find_last_of("/");
        if (slash != std::string::npos) dirpath = _dirname.substr(0, slash + 1); //we had removed the last character if it is  '/'
        //Logger::get().info() << "_dirname :" << _dirname;
        //Logger::get().info() << "dirpath  :" << dirpath;
        unsigned long long int totFilesize{};
        int i{};
        if(dpdf != NULL){
            while ((epdf = readdir(dpdf)) != NULL ){
                std::string filename = epdf->d_name;
                size_t dotPos = r_pos(filename,".",2); //找到后缀中倒数第二个.
                //Logger::get().info() << "dotPos :" << dotPos;
                //size_t dotPos = filename.rfind(".",std::string::npos,2);
                if(dotPos == std::string::npos) continue;
                std::string suffix = filename.substr(dotPos + 1);
                //Logger::get().info() << "suffix :" << suffix;
                if(suffix == _suffix) {
                    //struct stat stat_buff;
                    std::string filepath;
                    //long filesize;
                    filepath = dirpath + "/" + filename;
                    //int rc = stat(filepath.c_str(), &stat_buff);
                    //rc == 0 ? filesize=stat_buff.st_size : -1;
                    //_totalsize += filesize;
                    totFilesize += fs::file_size(filepath);
                    _filecount++;
                    _filenames.push_back(filepath);
                }
            }
        }
        for (; totFilesize >= 1024.; totFilesize /= 1024., ++i) { };
        totFilesize = std::ceil(totFilesize * 10.) / 10.;
        _totalsize = std::to_string(totFilesize) + "BKMGTPE"[i] ;
    }
private:
    std::string _dirname;
    std::string _suffix;
    std::string _totalsize;
    uint16_t _filecount;
    std::vector<std::string> _filenames;
    friend  FilesInfo getinfo(FileContainer);
};
// 这种方式其实并没有很好，在过去看的Flye的代码里面其实对private的部分的数据，我们可能更多的是通过class内的函数来提取的。
inline FilesInfo getinfo(FileContainer t){
    FilesInfo filsinformation(t._filenames, t._filecount, t._totalsize);
        return filsinformation;
}

template <typename Out>
inline void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        *result++ = item;
    }
}

inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}


inline bool demo_exists(const fs::path& p, fs::file_status s = fs::file_status{}) // 判断文件大小
{
    //std::cout << p;
    if(fs::status_known(s) ? fs::exists(s) : fs::exists(p))
        return true;
    else
        return false;
}

inline size_t curl_to_string(char* ptr, size_t size, size_t nmemb, void* data) // 将从网上摘录的字符进行转化。
{
        std::string* str = (std::string*)data;
        size_t x;
        for (x = 0; x < size * nmemb; ++x)
        {
                (*str) += ptr[x];
        }

        return size*nmemb;
}

inline std::string curlGetHtmlSource(std::string& link) //接受网址并返回字符。
{
    CURL* curl;
    CURLcode res;
    std::string html_txt;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, &link[0]);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_to_string);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html_txt);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);

        /* Check for errors */
        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                curl_easy_cleanup(curl);
                throw std::runtime_error("Can't get html source");
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    return html_txt;
}

template <class T>
std::pair<std::string, bool> getLargestChr(T& Inmap){
    float sum = 0;
    float max = 0 ;
    std::string maxChr;
    for(const auto& i : Inmap.lock_table()){
        sum = sum + i.second;
        if (i.second > max){
            max = i.second;
            maxChr = i.first;
        }
    }
    if(max/sum >= 0.95){
        return {maxChr, true};
    }
    else{
        return {maxChr, false};
    }
}

// use strtok to split string with delimiter. 多分隔符
inline std::vector<std::string> splitMulti(std::string str, const char* delimiter){
    std::vector<std::string> v;
    char *token = strtok(const_cast<char*>(str.c_str()), delimiter);
    while (token != nullptr)
    {
        v.push_back(std::string(token));
        token = strtok(nullptr, delimiter);
    }
    return v;
}
// use strtok to split string with delimiter. 单分隔符
inline void splitStringEmptyColIncluded(const std::string& input, std::vector<std::string>& tokens, char delim = '\t', const std::string& emptyColReplacement = "-") {
    std::stringstream ss(input);
    std::string token;
    while (std::getline(ss, token, delim)) {
        if (token.empty()) {
            tokens.push_back(emptyColReplacement);
        }
        else {
            tokens.push_back(token);
        }
    }
    // if the last column is empty, add emptyColReplacement to the end
    if (tokens.size() != 15) {
        tokens.push_back(emptyColReplacement);
    }
}

inline void copyFile(std::string srcPath, std::string dstPath, std::string srcFile, std::string targetFile = ""){
    if(targetFile == ""){
        std::ifstream  src(srcPath + "/" + srcFile, std::ios::binary);
        std::ofstream  dst(dstPath + "/" + srcFile, std::ios::binary);
        dst << src.rdbuf();
    }else{
        std::ifstream  src(srcPath + "/" + srcFile, std::ios::binary);
        std::ofstream  dst(dstPath + "/" + targetFile, std::ios::binary);
        dst << src.rdbuf();
    }
}

template<class T>
int getExtreamDeltaMatch(T& v1, const bool& largest, const bool& first){
    int index = 0;
    int reIndex = 0;
    int SmallestValue = INT_MIN ;
    int LargestValue = INT_MAX ;
    for(const auto& i : v1){
        if(largest){
            if(first){
                if(i.first >= SmallestValue){
                    SmallestValue = i.first;
                    reIndex = index;
                }
            }
            else{
                if(i.second >= SmallestValue){
                    SmallestValue = i.second;
                    reIndex = index;
                }
            }
        }
        else{
            if(first){
                if(i.first <= LargestValue){
                    LargestValue = i.first;
                    reIndex = index;
                }
            }
            else{
                if(i.second <= LargestValue){
                    LargestValue = i.second;
                    reIndex = index;
                }
            }
            
        }
        index++;
        
    }
    return reIndex;
}


#endif /* utils_h */



