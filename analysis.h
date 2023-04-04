//
//  analysis.h
//  PersonalGenome
//
//  Created by wangbaonan on 07/26/2022.
//

#ifndef analysis_h
#define analysis_h

#include <future>
#include <thread>

#include "config.h"
#include "build_files.h"
#include "cuckoohash_map.hh"

class Analysis
{
    friend class admixtureModule;
    friend class pcaModule;

public:
    Analysis(cuckoohash_map<std::string, SampleFiles> sampleFilesHashMap, std::vector<std::string> &vcfFileNames, const size_t switchCode) : _sampleFilesHashMap(sampleFilesHashMap), _vcfFileNames(vcfFileNames), _switchCode(switchCode)
    {
    }
    //~Analysis();// 在析构的模块中join？或者wait每个模块结束
    void preExtractAutosomesVCF();
    std::vector<int> launchLoadedModule(std::string processBarFile);
    void admixtureModule();
    void pcaModule();
    void mtYClassificationModule();
    void as2Module();
    void hlaModule();
    void snpediaModule();
    void pgsModule();
    void svModule();
    void provinceModule();
    void futureMonitor(std::vector<std::future<int>> &LoadModule, std::string processBarFile); // future监视器监控分析模块运行进度，并生成相应的Session内容
    void convertRes2Json(const std::vector<int> &successfulModuleCode);
    // Result to JSON converters
    void convertAdmixtureRes2Json();                                                                                                  // Admixture JSON
    void convertArchaicSeekerRes2Json();                                                                                              // AS2 JSON
    void convertHlaRes2Json();                                                                                                        // HLA JSON
    void convertMTYRes2Json();                                                                                                        // MTY JSON
    void convertPCARes2Json();                                                                                                        // PCA JSON
    void convertPRSRes2Json();                                                                                                        // PRS JSON
    void convertSVRes2Json();                                                                                                         // SV JSON
    void convertClusterTermRes2Json();                                                                                                // AS2 David Cluster Result convert to JSON
    void convertClusterGeneRes2Json();                                                                                                // AS2 David Cluster Result convert to JSON
    void convertProvinceRes2Json();                                                                                                   // Province JSON
    void convertStandardRes2Json(std::string originalResultPath, std::string convertedJSON, std::string MODULE_NAME);                 // 第一行为属性名，其他行对应属性的
    void convertStandardResEmptyColIncluded2Json(std::string originalResultPath, std::string convertedJSON, std::string MODULE_NAME); // 包含有空列的结果转换为JSON
    void convertStandardClusteredRes2Json(std::string originalResultPath, std::string convertedJSON, std::string MODULE_NAME);        // 个别行包含有Cluster信息的结果转换为JSON
    void aggregateJsonRes();
    void aggregateRes();

private:
    cuckoohash_map<std::string, SampleFiles> _sampleFilesHashMap;
    cuckoohash_map<std::string, uint32_t> _buildFailsMap;
    std::vector<std::string> _vcfFileNames;     // 传入的是包含所有染色体的VCF 需要处理成仅包含chrA染色体的文件路径  MT_Y模块传入这个变量
    std::vector<std::string> _vcfChrAFileNames; // 其他模块传入这个变量 首先需要对这个变量进行修改 这个函数应该位于所有其他模块之前执行 在修改变量的同时完成chrA染色体的提取
    size_t _switchCode;                         // 510代表除NGS以外，所有模块都启用 按位与操作
    size_t _threadNum = (size_t)std::atoi(Config::get("Thread").c_str());
    std::vector<int> _successfulModuleCode;
    std::mutex _loggerMutex;        // Logger锁
    std::mutex _jsonConverterMutex; // JSON 转换线程锁 在使用线程锁的过程中需要注意将锁放到临界区以外的地方，对于单例类内的是共享的，但是对于非单例类的可能会出现其他的问题。
    std::mutex _rapidJSONMutex;
    std::mutex _aggregateMutex;
};

#endif