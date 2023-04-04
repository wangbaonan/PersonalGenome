//
//  analysis.cpp
//  PersonalGenome
//
//  Created by wangbaonan on 07/26/2022.
//
#include <future>
#include <thread>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <map>
#include <regex>
#include <unordered_map>
#include <zlib.h>

#include "analysis.h"
#include "parallel.h"
#include "processer.h"
#include "utils.h"
#include "/home/pogadmin/PersonalGenome/rapidjson-master/include/rapidjson/document.h"
#include "/home/pogadmin/PersonalGenome/rapidjson-master/include/rapidjson/writer.h"
#include "/home/pogadmin/PersonalGenome/rapidjson-master/include/rapidjson/stringbuffer.h"
#include "/home/pogadmin/PersonalGenome/rapidjson-master/include/rapidjson/prettywriter.h"

using namespace rapidjson;

void Analysis::preExtractAutosomesVCF()
{
    Logger::get().info() << "Extracting autosomes vcf ...";
    std::function<void(const std::string &filepath)> extractAutosomesVCFFunc =
        [this](const std::string &filePath)
    {
        std::string extractScript = Config::get("extractsh");
        Logger::get().debug() << "extractScript Path :" << extractScript;
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        // 从用户提交的全染色体VCF中提取出常染色体VCF
        uint32_t stat = system(("sh " + extractScript + " " + filePath + " " + fileStruct.chrAPath + " && tabix -f " + fileStruct.chrAPath).c_str());
        if (stat != 0)
        {
            _sampleFilesHashMap.erase(filePath);
            _buildFailsMap.insert(filePath, stat);
        }
    };
    // size_t threadnum = (size_t)std::atoi(Config::get("Thread").c_str());
    processInParallel(_vcfFileNames, extractAutosomesVCFFunc, _threadNum, false); // 注意这里传入的VCF名称应是全染色体的需要被提取出chrA的文件对应的私有变量
}

std::vector<int> Analysis::launchLoadedModule(std::string processBarFile)
{
    Logger::get().info() << "Loading modules ...";
    // 容器装载 已加载的模块 遍历依次wait()
    std::vector<std::future<int>> LoadModule;
    if (_switchCode & ADMIXTURE_SWITCH)
    {
        Logger::get().info() << "Admixture Module load successfully! ADMIXTURE RUNNING ... ";
        std::future<int> futureAdmixture = std::async(std::launch::async, [this]()
                                                      {
                                                          this->admixtureModule();
                                                          // Logger::get().info()<< "async ADMIXTURE thread done.";
                                                          return ADMIXTURE_SWITCH; // 收集switch Code 最终通过判断加和与起始要求的模块是否一致判断是否结束
                                                      });
        LoadModule.push_back(std::move(futureAdmixture)); // 查看move的操作
    }

    if (_switchCode & PCA_SWITCH)
    {
        Logger::get().info() << "PCA Module load successfully! PCA RUNNING ... ";
        std::future<int> futurePca = std::async(std::launch::async, [this]()
                                                { 
            this->pcaModule();
            //Logger::get().info()<< "async PCA thread done.";
            return PCA_SWITCH; });
        LoadModule.push_back(std::move(futurePca));
    }

    if (_switchCode & MTY_SWITCH)
    {
        Logger::get().info() << "MTY_CLASS Module load successfully! MTY_CLASS RUNNING ... ";
        std::future<int> futureMty = std::async(std::launch::async, [this]()
                                                { 
            this->mtYClassificationModule();
            //Logger::get().info()<< "async PCA thread done.";
            return MTY_SWITCH; });
        LoadModule.push_back(std::move(futureMty));
    }

    if (_switchCode & AS2_SWITCH)
    {
        Logger::get().info() << "AS2_CLASS Module load successfully! AS2_CLASS RUNNING ... ";
        std::future<int> futureAS2 = std::async(std::launch::async, [this]()
                                                { 
            this->as2Module();
            //Logger::get().info()<< "async PCA thread done.";
            return AS2_SWITCH; });
        LoadModule.push_back(std::move(futureAS2));
    }

    if (_switchCode & HLA_SWITCH)
    {
        Logger::get().info() << "HLA_CLASS Module load successfully! HLA_CLASS RUNNING ... ";
        std::future<int> futureHLA = std::async(std::launch::async, [this]()
                                                {
            this->hlaModule();
            return HLA_SWITCH; });
        LoadModule.push_back(std::move(futureHLA));
    }

    if (_switchCode & SNPEDIA_SWITCH)
    {
        Logger::get().info() << "SNPEDIA Module load successfully! SNPEIDA_CLASS RUNNING ... ";
        std::future<int> futureSNPedia = std::async(std::launch::async, [this]()
                                                    {
            this->snpediaModule();
            return SNPEDIA_SWITCH; });
        LoadModule.push_back(std::move(futureSNPedia));
    }

    if (_switchCode & PRS_SWITCH)
    {
        Logger::get().info() << "PRS Module load successfully! PRS_CLASS RUNNING ... ";
        std::future<int> futurePRS = std::async(std::launch::async, [this]()
                                                {
            this->pgsModule();
            return PRS_SWITCH; });
        LoadModule.push_back(std::move(futurePRS));
    }

    if (_switchCode & SV_SWITCH)
    {
        Logger::get().info() << "SV Annotation Module load successfully! SV_CLASS RUNNING ... ";
        std::future<int> futureSV = std::async(std::launch::async, [this]()
                                               {
            this->svModule();
            return SV_SWITCH; });
        LoadModule.push_back(std::move(futureSV));
    }

    if (_switchCode & PROVINCE_SWITCH)
    {
        Logger::get().info() << "Province Annotation Module load successfully! PROVINCE_CLASS RUNNING ... ";
        std::future<int> futureProvince = std::async(std::launch::async, [this]()
                                                     {
            this->provinceModule();
            return PROVINCE_SWITCH; });
        LoadModule.push_back(std::move(futureProvince));
    }
    // TODO <添加futureMonitor>
    this->futureMonitor(LoadModule, processBarFile);

    // 要注意这种方式会被阻塞在进入循环加载的第一个Moudle里，因此这种方式中无法嵌入对最后结果的监控，也就无法实现进度条的功能
    // for (uint64_t i = 0; i < LoadModule.size(); ++i)
    // {
    //     LoadModule[i].wait();
    //     int moduleSuccessCode = LoadModule[i].get();
    //     switch (moduleSuccessCode)
    //     {
    //     case PCA_SWITCH:
    //         Logger::get().info() << _sampleFilesHashMap.size() << " samples pca analysis done.";
    //         break;
    //     case ADMIXTURE_SWITCH:
    //         Logger::get().info() << _sampleFilesHashMap.size() << " samples admixture analysis done.";
    //         break;
    //     case MTY_SWITCH:
    //         Logger::get().info() << _sampleFilesHashMap.size() << " samples mty_classification analysis done.";
    //         break;
    //     case AS2_SWITCH:
    //         Logger::get().info() << _sampleFilesHashMap.size() << " samples as2 analysis done.";
    //         break;
    //     case HLA_SWITCH:
    //         Logger::get().info() << _sampleFilesHashMap.size() << " samples hla analysis done.";
    //         break;
    //     case SNPEDIA_SWITCH:
    //         Logger::get().info() << _sampleFilesHashMap.size() << " samples snpeida annotation analysis done. ";
    //         break;
    //     case PRS_SWITCH:
    //         Logger::get().info() << _sampleFilesHashMap.size() << " samples prs analysis done. ";
    //         break;
    //     case SV_SWITCH:
    //         Logger::get().info() << _sampleFilesHashMap.size() << " samples sv analysis done. ";
    //     default:
    //         break;
    //     }
    // }
    Logger::get().info() << "Congratulations! " << _sampleFilesHashMap.size() << " samples all analysis done!";

    return _successfulModuleCode;
}

// 轮询监控future容器中的future对象
void Analysis::futureMonitor(std::vector<std::future<int>> &LoadModule, std::string processBarFile)
{
    Processer::get().setOutputFile(processBarFile); // 配置ProcessLog路径
    do
    {
        for (uint64_t i = 0; i < LoadModule.size(); ++i)
        {
            std::future_status status = LoadModule[i].wait_for(std::chrono::seconds(1));
            if (status == std::future_status::ready)
            {
                int moduleSuccessCode = LoadModule[i].get();
                _successfulModuleCode.push_back(moduleSuccessCode);
                std::string successfulModuleName;
                switch (moduleSuccessCode)
                {
                case PCA_SWITCH:
                    Logger::get().info() << _sampleFilesHashMap.size() << " samples pca analysis done.";
                    successfulModuleName = "PCA";
                    break;
                case ADMIXTURE_SWITCH:
                    Logger::get().info() << _sampleFilesHashMap.size() << " samples admixture analysis done.";
                    successfulModuleName = "ADMIXTURE";
                    break;
                case MTY_SWITCH:
                    Logger::get().info() << _sampleFilesHashMap.size() << " samples mty_classification analysis done.";
                    successfulModuleName = "MT_Y";
                    break;
                case AS2_SWITCH:
                    Logger::get().info() << _sampleFilesHashMap.size() << " samples as2 analysis done.";
                    successfulModuleName = "AS2";
                    break;
                case HLA_SWITCH:
                    Logger::get().info() << _sampleFilesHashMap.size() << " samples hla analysis done.";
                    successfulModuleName = "HLA";
                    break;
                case SNPEDIA_SWITCH:
                    Logger::get().info() << _sampleFilesHashMap.size() << " samples snpeida annotation analysis done. ";
                    successfulModuleName = "SNPEDIA";
                    break;
                case PRS_SWITCH:
                    Logger::get().info() << _sampleFilesHashMap.size() << " samples prs analysis done. ";
                    successfulModuleName = "PRS";
                    break;
                case SV_SWITCH:
                    Logger::get().info() << _sampleFilesHashMap.size() << " samples sv analysis done. ";
                    successfulModuleName = "SV";
                    break;
                case PROVINCE_SWITCH:
                    Logger::get().info() << _sampleFilesHashMap.size() << " samples province analysis done. ";
                    successfulModuleName = "PROVINCE";
                    break;
                default:
                    break;
                }
                // 从future容器中移出已完成的Module
                LoadModule.erase(LoadModule.begin() + i);
                // 与此同时向Process文件中输入模块已完成的信息，SpringBoot后端通过监控该文件修改对应用户的session即可
                // SampleFiles fileStruct = _sampleFilesHashMap.find(filePath); // 当前的架构中，这一层是包含了所有样本的，因此没有fileStruct，需要额外传入一个输出ProcessBar信息的路径参数。
                Processer::get().process() << successfulModuleName << " ";
                // TODO 给Processer增加一个update的函数，每次更新都会覆盖之前的内容，而不是每次都append，其中使用rapidJson的Writer来实现
            }
            else
            {
                continue;
            }
        }
    } while (!LoadModule.empty());
}

void Analysis::admixtureModule()
{
    std::function<void(const std::string &filepath)> admixtureFunc =
        [this](const std::string &filePath)
    {
        std::string admixtureScript = Config::get("admixturesh");
        Logger::get().debug() << "admixtureScript Path :" << admixtureScript;
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath); // 构建成的文件结构体中取出chrA的FilePath再传入下方fileStruct.fullPath
        // uint32_t stat = system(("sh " + admixtureScript + " " + filePath + " " + fileStruct.admixturePath + "/" + fileStruct.sampleId + ".admix.out").c_str());
        uint32_t stat = system(("module load hap.py/0.3.15 && "
                                "module load bcftools/1.14 && "
                                "module load python/3.8.13 && "
                                "sh " +
                                admixtureScript + " " + fileStruct.chrAPath + " " + fileStruct.sampleId + " " + fileStruct.admixturePath)
                                   .c_str());
        if (stat != 0)
        {
            _sampleFilesHashMap.erase(filePath);
            _buildFailsMap.insert(filePath, stat);
        }
    };
    // size_t threadnum = (size_t)std::atoi(Config::get("Thread").c_str());
    processInParallel(_vcfFileNames, admixtureFunc, _threadNum, false); // 注意这里传入的VCF名称 是包含MT和Y的最初始的
}

void Analysis::pcaModule()
{
    std::function<void(const std::string &filepath)> pcaFunc =
        [this](const std::string &filePath)
    {
        std::string pcaScript = Config::get("pcash");
        Logger::get().debug() << "pcaScript Path :" << pcaScript;
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath); // 构建成的文件结构体中取出chrA的FilePath再传入下方
        // uint32_t stat = system(("sh " + pcaScript + " " + filePath + " " + fileStruct.pcaPath + "/" + fileStruct.sampleId + ".pca.out").c_str());
        uint32_t stat = system(("module load hap.py/0.3.15 && "
                                "module load plink/1.90 && "
                                "module load python/3.8.13 && "
                                "module load bcftools/1.14 && "
                                "sh " +
                                pcaScript + " " + fileStruct.chrAPath + " " + fileStruct.sampleId + " " + fileStruct.pcaPath)
                                   .c_str());
        if (stat != 0)
        {
            _sampleFilesHashMap.erase(filePath);
            _buildFailsMap.insert(filePath, stat);
        }
    };
    // size_t threadnum = (size_t)std::atoi(Config::get("Thread").c_str());
    processInParallel(_vcfFileNames, pcaFunc, _threadNum, false); // 注意这里传入的VCF名称 是包含MT和Y的
}

void Analysis::mtYClassificationModule()
{
    std::function<void(const std::string &filepath)> mtyFunc =
        [this](const std::string &filePath)
    {
        std::string mtyScript = Config::get("mtysh");
        // std::string mtyScrdir = Config::get("mtyscrdir");//b38inference
        Logger::get().debug() << "MT_Y_Classification Script Path :" << mtyScript;
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        uint32_t stat = system(("module load hap.py/0.3.15 && "
                                "module load python/3.8.13 && "
                                "module load plink/2.0 && "
                                "module load AdmixTools/7.0.2 && "
                                "module load admixture/1.3.0 && "
                                "sh " +
                                mtyScript + " " + fileStruct.allChrPath + " " + fileStruct.mtyPath)
                                   .c_str());
        if (stat != 0)
        {
            _sampleFilesHashMap.erase(filePath);
            _buildFailsMap.insert(filePath, stat);
        }
    };
    // size_t threadNum = (size_t)std::atoi(Config::get("Thread").c_str());
    processInParallel(_vcfFileNames, mtyFunc, _threadNum, false);
}

// TODO 测试该模块
void Analysis::provinceModule()
{
    std::function<void(const std::string &filepath)> provinceFunc =
        [this](const std::string &filePath)
    {
        std::string provinceScript = Config::get("provincesh");

        Logger::get().debug() << "Province script path: " << provinceScript;
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        uint32_t stat = system(("module load hap.py/0.3.15 && "
                                "module load bcftools/1.14 && "
                                "module load python/3.7.13 && "
                                "sh " +
                                provinceScript + " " + fileStruct.chrAPath + " " + fileStruct.sampleId + " " + fileStruct.provincePath)
                                   .c_str());
        if (stat != 0)
        {
            _sampleFilesHashMap.erase(filePath);
            _buildFailsMap.insert(filePath, stat);
        }
    };
    // size_t threadNum = (size_t)std::atoi(Config::get("Thread").c_str());
    processInParallel(_vcfFileNames, provinceFunc, _threadNum, false);
}

void Analysis::as2Module()
{
    std::function<void(const std::string &filepath)> as2Func =
        [this](const std::string &filePath)
    {
        std::string as2Script = Config::get("as2sh");

        Logger::get().debug() << "AS2 script path: " << as2Script;
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        uint32_t stat = system(("module load hap.py/0.3.15 && "
                                "module load bcftools/1.14 && "
                                "module load vcftools/0.1.16 && "
                                "module load ArchaicSeeker/2.0 && "
                                "module load shapeit4/4.2.2 && "
                                "sh " +
                                as2Script + " " + fileStruct.as2Path + " " + fileStruct.chrAPath + " " + fileStruct.as2Path)
                                   .c_str());
        if (stat != 0)
        {
            _sampleFilesHashMap.erase(filePath);
            _buildFailsMap.insert(filePath, stat);
        }
    };
    // size_t threadNum = (size_t)std::atoi(Config::get("Thread").c_str());
    processInParallel(_vcfFileNames, as2Func, _threadNum, false);
}

void Analysis::hlaModule()
{
    std::function<void(const std::string &filepath)> hlaFunc =
        [this](const std::string &filePath)
    {
        std::string hlaScript = Config::get("hlash");
        Logger::get().debug() << "HLA script Path: " << hlaScript;
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        uint32_t stat = system(("module load hap.py/0.3.15 && "
                                "module load beagle/4.1 && "
                                "module load AncestryPainter/5.0 && "
                                "module load picard/2.26.10 && "
                                "module load fgbio/1.5.0 && "
                                "module load bcftools/1.14 && "
                                "module load plink/1.90  && "
                                "module load htslib/1.14 && "
                                "module load python/3.7.13 && "
                                "sh " +
                                hlaScript + " " + fileStruct.chrAPath + " " + fileStruct.hlaPath)
                                   .c_str());
        if (stat != 0)
        {
            _sampleFilesHashMap.erase(filePath);
            _buildFailsMap.insert(filePath, stat);
        }
    };
    processInParallel(_vcfFileNames, hlaFunc, _threadNum, false);
}

void Analysis::snpediaModule()
{
    std::function<void(const std::string &filepath)> snpediaFunc =
        [this](const std::string &filePath)
    {
        std::string snpeidaScript = Config::get("snpediash");
        Logger::get().debug() << "SNPeida script Path: " << snpeidaScript;
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        uint32_t stat = system(("module load hap.py/0.3.15 && "
                                "module load bcftools/1.14 && "
                                "module load python/3.7.13 && "
                                "sh " +
                                snpeidaScript + " " + fileStruct.allChrPath + " " + fileStruct.snpEdiaPath)
                                   .c_str());
        if (stat != 0)
        {
            _sampleFilesHashMap.erase(filePath);
            _buildFailsMap.insert(filePath, stat);
        }
    };
    processInParallel(_vcfFileNames, snpediaFunc, _threadNum, false);
}

void Analysis::pgsModule()
{
    std::function<void(const std::string &filepath)> pgsFunc =
        [this](const std::string &filePath)
    {
        std::string pgsScript = Config::get("pgssh");
        std::string pgsRetryScript = Config::get("pgsRetrysh");
        std::string HGDPrefVCF = Config::get("pgsrefPanel");
        std::string pgsTxtPath = Config::get("pgstxtPath");
        std::string pgsRef = Config::get("pgsRef");
        Logger::get().debug() << "PGS script Path: " << pgsScript;
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        int retryTimes = 1;
        /*
        // old version 1.0
        uint32_t stat = system(("module load python/3.8.13 && "
                                "sh " + pgsScript + " " + fileStruct.allChrPath + " " + HGDPrefVCF + " " +
                                fileStruct.prsPath + " " + fileStruct.sampleId + " " + pgsRef + " " + pgsTxtPath).c_str());
        */
        const char *pgsCommand = ("module load python/3.8.13 && "
                                  "sh " +
                                  pgsScript + " " + fileStruct.allChrPath + " " + fileStruct.prsPath + " " + fileStruct.sampleId)
                                     .c_str();
        uint32_t stat = system(pgsCommand);
        const char *pgsRetryCommand = ("module load python/3.8.13 && "
                                       "sh " +
                                       pgsRetryScript + " " + fileStruct.allChrPath + " " + fileStruct.prsPath + " " + fileStruct.sampleId)
                                          .c_str();

        // Retry prs only pgsc_calc
        Logger::get().info() << "PGS retryTimes num: " << retryTimes;
        while (stat != 0 && retryTimes < 10)
        {
            Logger::get().info() << "Accidental [ERROR] in pgsc_calc occured. [Retry times]: " << retryTimes;
            stat = system(pgsRetryCommand);
            retryTimes++;
            if(retryTimes==10){
                throw std::runtime_error("PGS retry 10 times, but still failed.");
            }
        }
        if (stat != 0)
        {
            _sampleFilesHashMap.erase(filePath);
            _buildFailsMap.insert(filePath, stat);
        }
    };
    processInParallel(_vcfFileNames, pgsFunc, _threadNum, false);
}

void Analysis::svModule()
{
    std::function<void(const std::string &filepath)> svFunc =
        [this](const std::string &filePath)
    {
        std::string svScript = Config::get("svAnnotationPy");
        std::string svRefDir = Config::get("svAnnotationRef");
        std::string svAnnoDB = Config::get("svAnnoDB");
        std::string svOverlap = Config::get("svOverlapPer");
        std::string svGeneAnnoFlag = Config::get("svGeneAnnoFlag");
        std::string assemblyVersion = Config::get("assemblyVersion");
        std::string svAnnoExtractSh = Config::get("svAnnotationExtractSh");
        bool svGene;
        Logger::get().debug() << "SVAnnotaion script Path: " << svScript;
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        std::string svAnnoFile = fileStruct.svAnnoPath + "/" + fileStruct.sampleId + ".sv.anno.txt";
        std::istringstream(svGeneAnnoFlag) >> svGene;
        if (svGene)
        {
            uint32_t stat = system(("module load python/3.8.13 && "
                                    "python3 " +
                                    svScript + " -i " + fileStruct.svPath + " --Gene " + " --SVDB " + svAnnoDB + " -r " + svRefDir + " -o " + svAnnoFile + " -a " + assemblyVersion + " --svSVOverlapPer " +
                                    svOverlap + " && " + svAnnoExtractSh + " " + svAnnoFile + " " + fileStruct.svAnnoPath + " ")
                                       .c_str());
            if (stat != 0)
            {
                _sampleFilesHashMap.erase(filePath);
                _buildFailsMap.insert(filePath, stat);
            }
        }
        else
        {
            uint32_t stat = system(("module load python/3.8.13 && "
                                    "python3 " +
                                    svScript + " -i " + fileStruct.svPath + " --SVDB " + svAnnoDB + " -r " + svRefDir + " -o " + svAnnoFile + " -a " + assemblyVersion + " --svSVOverlapPer " +
                                    svOverlap + " && " + svAnnoExtractSh + " " + svAnnoFile + " " + fileStruct.svAnnoPath + " ")
                                       .c_str());
            if (stat != 0)
            {
                _sampleFilesHashMap.erase(filePath);
                _buildFailsMap.insert(filePath, stat);
            }
        }
    };
    processInParallel(_vcfFileNames, svFunc, _threadNum, false);
}

void Analysis::convertRes2Json(const std::vector<int> &successfulModuleCode)
{
    // TODO 新建一个目录存放所有的JSON数据(cp) 或 最终交付给前端的数据
    for (uint64_t moduleCode : successfulModuleCode)
    {
        switch (moduleCode)
        {
        case PCA_SWITCH:
            Logger::get().info() << "Convert PCA result to JSON running ...";
            this->convertPCARes2Json();
            break;
        case ADMIXTURE_SWITCH:
            Logger::get().info() << "Convert ADMIXTURE result to JSON running ...";
            this->convertAdmixtureRes2Json();
            break;
        case MTY_SWITCH:
            Logger::get().info() << "Convert MTY result to JSON running ...";
            this->convertMTYRes2Json();
            break;
        case AS2_SWITCH:
            Logger::get().info() << "Convert AS2 result to JSON running ...";
            this->convertArchaicSeekerRes2Json();
            this->convertClusterTermRes2Json();
            this->convertClusterGeneRes2Json();
            break;
        case HLA_SWITCH:
            Logger::get().info() << "Convert HLA result to JSON running ...";
            this->convertHlaRes2Json();
            break;
        case SNPEDIA_SWITCH:
            break;
        case PRS_SWITCH:
            Logger::get().info() << "Convert PRS result to JSON running ...";
            this->convertPRSRes2Json();
            break;
        case SV_SWITCH:
            Logger::get().info() << "Convert SV result to JSON running ...";
            this->convertSVRes2Json();
            break;
        case PROVINCE_SWITCH:
            Logger::get().info() << "Convert Province result to JSON running ...";
            this->convertProvinceRes2Json();
            break;
        default:
            break;
        }
    }
}

void Analysis::convertAdmixtureRes2Json()
{
    std::function<void(const std::string &filepath)> convertAdmixtureRes2JsonFunc =
        [this](const std::string &filePath)
    {
        std::lock_guard<std::mutex> lock(_jsonConverterMutex);
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        std::string AdmixturePercentFile = fileStruct.admixturePath + "/NGS1926." + fileStruct.sampleId +
                                           ".chrA.MAF.miss10.admixture.input.ancestry.pop.languageFamily.txt";
        std::string EuclideanDistance = fileStruct.admixturePath + "/NGS1926." + fileStruct.sampleId +
                                        ".chrA.MAF.miss10.admixture.input.similarity.txt";
        // 打开文件
        std::ifstream file(AdmixturePercentFile);
        std::ifstream fileED(EuclideanDistance);
        if (file.is_open())
        {
            _loggerMutex.lock();
            Logger::get().info() << "file Open!";
            _loggerMutex.unlock();
        }
        if (fileED.is_open())
        {
            _loggerMutex.lock();
            Logger::get().info() << "fileED Open!";
            _loggerMutex.unlock();
        }
        // 创建一个 JSON 文档对象
        std::mutex JsonConvertMutex;
        JsonConvertMutex.lock();
        Document document;
        document.SetArray();
        Document documentED;
        documentED.SetObject();
        // 定义分隔符
        const char *delimiters = "\t\n";

        // P1 similarity
        std::string lineED;
        std::getline(fileED, lineED);
        char *tokenED = strtok(const_cast<char *>(lineED.c_str()), delimiters);
        double ed = std::stod(tokenED);
        Value ed_value(ed);
        documentED.AddMember("Euclidean Distance", ed_value, documentED.GetAllocator());
        tokenED = strtok(nullptr, delimiters);
        std::string population(tokenED);
        Value population_value(population.c_str(), documentED.GetAllocator());
        documentED.AddMember("Population", population_value, documentED.GetAllocator());
        StringBuffer bufferED;
        std::ofstream outJsonED;
        outJsonED.open(fileStruct.admixturePath + "/NGS1926." + fileStruct.sampleId + ".ED.similarity.json");
        PrettyWriter<StringBuffer> prettyWriterED(bufferED);
        documentED.Accept(prettyWriterED);
        outJsonED << bufferED.GetString() << std::endl;

        // P2 ancestry.proportion
        // 逐行读取文件中的数据
        std::string line;
        while (std::getline(file, line))
        {
            // 最外层while遍历每行，下面的逻辑是对每一行的操作
            // 创建并初始化一个空的JSON 对象，后续将数据存储到其中
            Value object(kObjectType);
            // const_cast将其修改为可修改的C字符串
            char *token = strtok(const_cast<char *>(line.c_str()), delimiters);
            // 创建空的字符串向量，存储分割后的子字符串
            std::vector<std::string> fields;
            while (token != nullptr)
            {
                fields.push_back(token);
                token = strtok(nullptr, delimiters);
            }
            // 成员名称 成员值（需要传入内存分配器）
            // 使用c_str()的原因是，RapidJSON 的字符串值需要一个指向以 '\0' 结尾的字符数组的指针
            object.AddMember("Population", Value(fields[0].c_str(), document.GetAllocator()), document.GetAllocator());
            object.AddMember("Region", Value(fields[1].c_str(), document.GetAllocator()), document.GetAllocator());
            object.AddMember("Language Family", Value(fields[2].c_str(), document.GetAllocator()), document.GetAllocator());
            object.AddMember("Percent", Value(fields[3].c_str(), document.GetAllocator()), document.GetAllocator());

            // 将 JSON 对象添加到 JSON 数组中
            document.PushBack(object, document.GetAllocator());
        }

        // 将 JSON 格式的字符串输出到JSON文件中
        StringBuffer buffer;
        std::ofstream outJson;
        outJson.open(fileStruct.admixturePath + "/NGS1926." + fileStruct.sampleId + ".ancestry.json");
        PrettyWriter<StringBuffer> prettyWriter(buffer);
        document.Accept(prettyWriter);
        outJson << buffer.GetString() << std::endl;
        Logger::get().info() << "Convert Admixture Result to JSON format done!";
        JsonConvertMutex.unlock();
    };
    processInParallel(_vcfFileNames, convertAdmixtureRes2JsonFunc, _threadNum, false);
    // NOT Parallel
    // for (const auto &task : _vcfFileNames)
    // {
    //     convertAdmixtureRes2JsonFunc(task);
    // }
}

void Analysis::convertArchaicSeekerRes2Json()
{
    std::function<void(const std::string &filepath)> convertArchaicSeekerSumRes2JsonFunc =
        [this](const std::string &filePath)
    {
        std::lock_guard<std::mutex> lock(_jsonConverterMutex);
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        std::string as2SumFilePath = fileStruct.as2Path + "/as2out.sum";
        // 打开文件
        std::ifstream as2SumFile(as2SumFilePath);

        // 创建一个 JSON 文档对象s
        Document documentSum;
        documentSum.SetObject();
        // 定义分隔符
        const char *delimiters = "\t\n";

        std::string lineSum;
        // 读取第二行sum
        std::getline(as2SumFile, lineSum);
        std::getline(as2SumFile, lineSum);
        char *tokenSum = strtok(const_cast<char *>(lineSum.c_str()), delimiters);
        std::vector<std::string> fields;
        while (tokenSum != nullptr)
        {
            fields.push_back(tokenSum);
            tokenSum = strtok(nullptr, delimiters);
        }
        Value objectSum(kObjectType);
        documentSum.AddMember("ID", Value(fields[0].c_str(), documentSum.GetAllocator()), documentSum.GetAllocator());
        documentSum.AddMember("YRI_Modern_Denisova_Vindija33.19_Altai(cM)", Value(fields[1].c_str(), documentSum.GetAllocator()), documentSum.GetAllocator());
        documentSum.AddMember("YRI_Modern(cM)", Value(fields[2].c_str(), documentSum.GetAllocator()), documentSum.GetAllocator());
        documentSum.AddMember("YRI(cM)", Value(fields[3].c_str(), documentSum.GetAllocator()), documentSum.GetAllocator());
        documentSum.AddMember("Modern(cM)", Value(fields[4].c_str(), documentSum.GetAllocator()), documentSum.GetAllocator());
        documentSum.AddMember("Denisova_Vindija33.19_Altai(cM)", Value(fields[5].c_str(), documentSum.GetAllocator()), documentSum.GetAllocator());
        documentSum.AddMember("Denisova(cM)", Value(fields[6].c_str(), documentSum.GetAllocator()), documentSum.GetAllocator());
        documentSum.AddMember("Vindija33.19_Altai(cM)", Value(fields[7].c_str(), documentSum.GetAllocator()), documentSum.GetAllocator());
        documentSum.AddMember("Vindija33.19(cM)", Value(fields[8].c_str(), documentSum.GetAllocator()), documentSum.GetAllocator());
        documentSum.AddMember("Altai(cM)", Value(fields[9].c_str(), documentSum.GetAllocator()), documentSum.GetAllocator());

        StringBuffer bufferSum;
        std::ofstream outJsonSum;
        outJsonSum.open(fileStruct.as2Path + "/as2out.sum.json");
        PrettyWriter<StringBuffer> prettyWriterSum(bufferSum);
        documentSum.Accept(prettyWriterSum);
        outJsonSum << bufferSum.GetString() << std::endl;
    };
    processInParallel(_vcfFileNames, convertArchaicSeekerSumRes2JsonFunc, _threadNum, false);

    std::function<void(const std::string &filepath)> convertArchaicSeekerSegRes2JsonFunc =
        [this](const std::string &filePath)
    {
        std::lock_guard<std::mutex> lock(_jsonConverterMutex);
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        std::string as2SegFilePath = fileStruct.as2Path + "/as2out.seg";
        std::string as2SegJsonPath = fileStruct.as2Path + "/as2out.seg.json";
        convertStandardRes2Json(as2SegFilePath, as2SegJsonPath, "AS2_SEG");
    };
    processInParallel(_vcfFileNames, convertArchaicSeekerSegRes2JsonFunc, _threadNum, false);

    // AS2 David ChartReport Result.
    std::function<void(const std::string &filepath)> convertArchaicSeekerDavidChartReportRes2JsonFunc =
        [this](const std::string &filePath)
    {
        std::lock_guard<std::mutex> lock(_jsonConverterMutex);
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        std::string as2SegFilePath = fileStruct.as2Path + "/list1.chartReport.txt";
        std::string as2SegJsonPath = fileStruct.as2Path + "/list1.chartReport.json";
        convertStandardRes2Json(as2SegFilePath, as2SegJsonPath, "AS2_SEG");
    };
    processInParallel(_vcfFileNames, convertArchaicSeekerDavidChartReportRes2JsonFunc, _threadNum, false);

    // AS2 David GeneClusterReport Result.
    // TODO 使用额外的方法，这部分的文件结构非标准，不应使用convertStandardRes2Json
    // std::function<void(const std::string &filepath)> convertArchaicSeekerDavidGeneClusterReportRes2JsonFunc =
    //     [this](const std::string &filePath)
    // {
    //     std::lock_guard<std::mutex> lock(_jsonConverterMutex);
    //     SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
    //     std::string as2SegFilePath = fileStruct.as2Path + "/list1.geneClusterReport.txt";
    //     std::string as2SegJsonPath = fileStruct.as2Path + "/list1.geneClusterReport.json";
    //     convertStandardRes2Json(as2SegFilePath, as2SegJsonPath, "AS2_SEG");
    // };
    // processInParallel(_vcfFileNames, convertArchaicSeekerDavidGeneClusterReportRes2JsonFunc, _threadNum, false);

    // AS2 David TableReport Result.
    // std::function<void(const std::string &filepath)> convertArchaicSeekerDavidTableReportRes2JsonFunc =
    //     [this](const std::string &filePath)
    // {
    //     std::lock_guard<std::mutex> lock(_jsonConverterMutex);
    //     SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
    //     std::string as2SegFilePath = fileStruct.as2Path + "/list1.tableReport.txt";
    //     std::string as2SegJsonPath = fileStruct.as2Path + "/list1.tableReport.json";
    //     convertStandardRes2Json(as2SegFilePath, as2SegJsonPath, "AS2_SEG");
    // };
    // processInParallel(_vcfFileNames, convertArchaicSeekerDavidTableReportRes2JsonFunc, _threadNum, false);

    // AS2 David TermClusteringReport Result.
    // TODO
    // std::function<void(const std::string &filepath)> convertArchaicSeekerDavidTermClusteringReportRes2JsonFunc =
    //     [this](const std::string &filePath)
    // {
    //     std::lock_guard<std::mutex> lock(_jsonConverterMutex);
    //     SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
    //     std::string as2SegFilePath = fileStruct.as2Path + "/list1.termClusteringReport.txt";
    //     std::string as2SegJsonPath = fileStruct.as2Path + "/list1.termClusteringReport.json";
    //     convertStandardRes2Json(as2SegFilePath, as2SegJsonPath, "AS2_SEG");
    // };
    // processInParallel(_vcfFileNames, convertArchaicSeekerDavidTermClusteringReportRes2JsonFunc, _threadNum, false);
}

void Analysis::convertHlaRes2Json()
{
    std::function<void(const std::string &filepath)> convertHlaRes2JsonFunc =
        [this](const std::string &filePath)
    {
        std::lock_guard<std::mutex> lock(_jsonConverterMutex);
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        std::string hlaFilePath = fileStruct.hlaPath + "/" + fileStruct.sampleId + ".VQSR.SNP.chrA_out2_hla_type_results_2_field.txt";
        std::string hlaJsonPath = fileStruct.hlaPath + "/" + fileStruct.sampleId + ".VQSR.SNP.chrA_out2_hla_type_results_2_field.json";
        std::ifstream hlaInput(hlaFilePath);
        Document document;
        document.SetArray();
        const char *delimiters = "\t\n";

        std::string line;
        std::getline(hlaInput, line);
        std::vector<std::string> hlaVector = splitMulti(line, delimiters);
        std::unordered_map<std::string, std::string> hlaHash;

        for (size_t i = 0; i < hlaVector.size(); ++i)
        {
            std::smatch regexMatchResults;
            std::string targetStr = hlaVector[i];
            std::regex regexA("A\\*\\d{2}:\\d{2}");
            std::regex regexB("B\\*\\d{2}:\\d{2}");
            std::regex regexC("C\\*\\d{2}:\\d{2}");
            std::regex regexDPA1("DPA1\\*\\d{2}:\\d{2}");
            std::regex regexDPB1("DPB1\\*\\d{2}:\\d{2}");
            std::regex regexDQA1("DQA1\\*\\d{2}:\\d{2}");
            std::regex regexDQB1("DQB1\\*\\d{2}:\\d{2}");
            std::regex regexDRB1("DRB1\\*\\d{2}:\\d{2}");
            // 哈希表转Json 先把HLA-A
            std::unordered_map<std::string, std::regex> regexHash = {
                {"HLA-A", regexA},
                {"HLA-B", regexB},
                {"HLA-C", regexC},
                {"HLA-DPA1", regexDPA1},
                {"HLA-DPB1", regexDPB1},
                {"HLA-DQA1", regexDQA1},
                {"HLA-DQB1", regexDQB1},
                {"HLA-DRB1", regexDRB1}};

            for (const auto &regexPair : regexHash)
            {
                if (std::regex_match(targetStr, regexMatchResults, regexPair.second))
                {
                    if (hlaHash.find(regexPair.first) != hlaHash.end())
                    {
                        hlaHash[regexPair.first] = hlaHash[regexPair.first] + "," + targetStr;
                    }
                    else
                    {
                        hlaHash[regexPair.first] = targetStr;
                    }
                }
            }
        }

        Value jsonObject(kObjectType);
        for (const auto &hlaPair : hlaHash)
        {
            jsonObject.AddMember(Value(hlaPair.first.c_str(), document.GetAllocator()), Value(hlaPair.second.c_str(), document.GetAllocator()), document.GetAllocator());
        }
        Value jsonObjectHLA(kObjectType);
        jsonObjectHLA.AddMember(Value(fileStruct.sampleId.c_str(), document.GetAllocator()), jsonObject, document.GetAllocator());
        document.PushBack(jsonObjectHLA, document.GetAllocator());

        // 将 JSON 格式的字符串输出到JSON文件中
        StringBuffer buffer;
        std::ofstream outJson;
        outJson.open(hlaJsonPath);
        PrettyWriter<StringBuffer> prettyWriter(buffer);
        document.Accept(prettyWriter);
        outJson << buffer.GetString() << std::endl;
        Logger::get().info() << "Convert HLA Result to JSON format done!";
    };
    processInParallel(_vcfFileNames, convertHlaRes2JsonFunc, _threadNum, false);
}

void Analysis::convertMTYRes2Json()
{
    std::function<void(const std::string &filepath)> convertMTYRes2JsonFunc =
        [this](const std::string &filePath)
    {
        std::lock_guard<std::mutex> lock(_jsonConverterMutex);
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        std::string mtyFilePath = fileStruct.mtyPath + "/" + fileStruct.sampleId + "/Report.txt";
        std::string mtyJsonPath = fileStruct.mtyPath + "/" + fileStruct.sampleId + "/Report.json";
        std::ifstream mtyInput(mtyFilePath);
        Document document;
        document.SetArray();
        const char *delimiters = "\t\n";

        // Extract MTY Report Infomation : Y hap/ MT hap/ Classification/ Region
        std::string line;
        std::getline(mtyInput, line);
        std::vector<std::string> yVector = splitMulti(line, delimiters);
        std::getline(mtyInput, line);
        std::vector<std::string> mtVector = splitMulti(line, delimiters);
        std::getline(mtyInput, line);
        std::vector<std::string> classificationVector = splitMulti(line, delimiters);
        std::getline(mtyInput, line);
        std::vector<std::string> regionVector = splitMulti(line, delimiters);

        // Convert to json (add member to json object)
        Value jsonObject(kObjectType);
        jsonObject.AddMember(Value(yVector[0].c_str(), document.GetAllocator()), Value(yVector[1].c_str(), document.GetAllocator()), document.GetAllocator());
        jsonObject.AddMember(Value(mtVector[0].c_str(), document.GetAllocator()), Value(mtVector[1].c_str(), document.GetAllocator()), document.GetAllocator());
        jsonObject.AddMember(Value(classificationVector[0].c_str(), document.GetAllocator()), Value(classificationVector[1].c_str(), document.GetAllocator()), document.GetAllocator());
        jsonObject.AddMember(Value(regionVector[0].c_str(), document.GetAllocator()), Value(regionVector[1].c_str(), document.GetAllocator()), document.GetAllocator());

        // Add sample id
        Value jsonObjectMTY(kObjectType);
        jsonObjectMTY.AddMember(Value(fileStruct.sampleId.c_str(), document.GetAllocator()), jsonObject, document.GetAllocator());
        document.PushBack(jsonObjectMTY, document.GetAllocator());

        // 将 JSON 格式的字符串输出到JSON文件中
        StringBuffer buffer;
        std::ofstream outJson;
        outJson.open(mtyJsonPath);
        PrettyWriter<StringBuffer> prettyWriter(buffer);
        document.Accept(prettyWriter);
        outJson << buffer.GetString() << std::endl;
        Logger::get().info() << "Convert MTY Result to JSON format done!";
    };
    processInParallel(_vcfFileNames, convertMTYRes2JsonFunc, _threadNum, false);
}

void Analysis::convertPCARes2Json()
{
    std::function<void(const std::string &filepath)> convertPCARes2JsonFunc =
        [this](const std::string &filePath)
    {
        // mutex lock_guard
        std::lock_guard<std::mutex> lock(_jsonConverterMutex);
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        std::string pcaFilePath = fileStruct.pcaPath + "/" + fileStruct.sampleId + ".VQSR.SNP.chrA.NGS1926.merged.biallelic.pca";
        std::string pcaJsonPath = fileStruct.pcaPath + "/" + fileStruct.sampleId + ".VQSR.SNP.chrA.NGS1926.merged.biallelic.pca.json";

        std::ifstream mtyInput(pcaFilePath);
        Document document;
        document.SetArray();
        const char *delimiters = "\t\n";

        std::string line;
        std::vector<std::string> headerVector{"Population", "Region", "SampleID", "PC1", "PC2", "PC3", "Label"};
        std::unordered_map<std::string, int> headerColHash{
            {"Population", 0},
            {"Region", 1},
            {"SampleID", 2},
            {"PC1", 3},
            {"PC2", 4},
            {"PC3", 5},
            {"Label", 13}};

        while (std::getline(mtyInput, line))
        {
            std::vector<std::string> targetVector = splitMulti(line, delimiters);
            Value jsonObject(kObjectType);
            for (size_t i = 0; i < headerVector.size(); ++i)
            {
                jsonObject.AddMember(Value(headerVector[i].c_str(), document.GetAllocator()), Value(targetVector[headerColHash[headerVector[i]]].c_str(), document.GetAllocator()), document.GetAllocator());
            }
            document.PushBack(jsonObject, document.GetAllocator());
        }

        StringBuffer buffer;
        std::ofstream outJson;
        outJson.open(pcaJsonPath);
        PrettyWriter<StringBuffer> prettyWriter(buffer);
        document.Accept(prettyWriter);
        outJson << buffer.GetString() << std::endl;
        Logger::get().info() << "Convert PCA Result to JSON format done!";
    };
    processInParallel(_vcfFileNames, convertPCARes2JsonFunc, _threadNum, false);
}

// convert PRS scores .gz file to json.
void Analysis::convertPRSRes2Json()
{
    std::function<void(const std::string &filepath)> convertPRSRes2JsonFunc =
        [this](const std::string &filePath)
    {
        std::lock_guard<std::mutex> lock(_jsonConverterMutex);
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        std::string gzPRSResFile = fileStruct.prsPath + "/results/score/aggregated_scores.txt.gz";
        std::string gzPRSResJsonPath = fileStruct.prsPath + "/results/score/aggregated_scores.json";

        // Open gzfile by "rb"
        // complie need to add -lz paramter
        gzFile gzPRSResInput = gzopen(gzPRSResFile.c_str(), "rb");
        if (!gzPRSResInput)
        {
            Logger::get().error() << "PRS results gz open error!";
        }
        char buffer[1024];
        std::string file_contents;
        while (gzgets(gzPRSResInput, buffer, 1024) != NULL)
        {
            file_contents += buffer;
        }
        gzclose(gzPRSResInput);
        std::vector<std::string> lines;
        char *line = strtok(file_contents.data(), "\n");
        while (line != NULL)
        {
            lines.push_back(line);
            line = strtok(NULL, "\n");
        }
        const char *delimiters = "\t";
        std::vector<std::string> prsHeaderVector = splitMulti(lines[0], delimiters);
        Document document;
        document.SetArray();

        // 第一行是Header 列数是动态的与选择的PRS个数有关，PRS的选择也可以交给用户，同时自己也需要准备一个相对合适的PRSlist作为默认。
        for (size_t i = 1; i < lines.size(); ++i)
        {
            Value jsonObject(kObjectType);
            std::vector<std::string> lineVector = splitMulti(lines[i], delimiters);
            for (size_t j = 0; j < prsHeaderVector.size(); ++j)
            {
                jsonObject.AddMember(Value(prsHeaderVector[j].c_str(), document.GetAllocator()), Value(lineVector[j].c_str(), document.GetAllocator()), document.GetAllocator());
            }
            document.PushBack(jsonObject, document.GetAllocator());
        }
        // document.PushBack(jsonObject, document.GetAllocator());
        StringBuffer bufferJson;
        std::ofstream outJson;
        outJson.open(gzPRSResJsonPath);
        PrettyWriter<StringBuffer> prettyWriter(bufferJson);
        document.Accept(prettyWriter);
        outJson << bufferJson.GetString() << std::endl;
        Logger::get().info() << "Convert PRS Result to JSON format done!";
    };
    processInParallel(_vcfFileNames, convertPRSRes2JsonFunc, _threadNum, false);
}

void Analysis::convertSVRes2Json()
{
    std::function<void(const std::string &filepath)> convertSVRes2JsonFunc =
        [this](const std::string &filePath)
    {
        // 标准方法可能不适合多\t分割的文档
        std::lock_guard<std::mutex> lock(_jsonConverterMutex);
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        std::string svFilePath = fileStruct.svAnnoPath + "/" + fileStruct.sampleId + ".sv.anno.txt";
        std::string svJsonPath = fileStruct.svAnnoPath + "/" + fileStruct.sampleId + ".sv.anno.json";

        convertStandardResEmptyColIncluded2Json(svFilePath, svJsonPath, "SV_ANNO");
    };
    processInParallel(_vcfFileNames, convertSVRes2JsonFunc, _threadNum, false);
}

// convert standard result file to json. Specifically build for SV result file.
void Analysis::convertStandardResEmptyColIncluded2Json(std::string originalResultPath, std::string convertedJSON, std::string MODULE_NAME)
{
    std::ifstream originalFile(originalResultPath);
    Document document;
    document.SetArray();

    std::string line;
    std::getline(originalFile, line);
    std::vector<std::string> headerVector;
    splitStringEmptyColIncluded(line, headerVector);
    while (std::getline(originalFile, line))
    {
        std::vector<std::string> attributeVector;
        splitStringEmptyColIncluded(line, attributeVector);
        Value jsonObject(kObjectType);
        // Logger::get().info() << line << "colNum: " << std::to_string(attributeVector.size());
        for (size_t i = 0; i < headerVector.size(); ++i)
        {
            if (headerVector[i] == "pggsv_sv_ID" && attributeVector[i] != "-")
            {
                std::string pggsvLink = "https://www.biosino.org/pggsv/#/search/detailSV?svId=" + attributeVector[i] + "&version=GRCh38";
                jsonObject.AddMember("pggsv_link", Value(pggsvLink.c_str(), document.GetAllocator()), document.GetAllocator());
            }
            jsonObject.AddMember(Value(headerVector[i].c_str(), document.GetAllocator()), Value(attributeVector[i].c_str(), document.GetAllocator()), document.GetAllocator());
        }
        document.PushBack(jsonObject, document.GetAllocator());
    }
    // Logger::get().info() << std::to_string(headerVector.size());
    StringBuffer buffer;
    std::ofstream outJson;
    outJson.open(convertedJSON);
    PrettyWriter<StringBuffer> prettyWriter(buffer);
    document.Accept(prettyWriter);
    outJson << buffer.GetString() << std::endl;
    Logger::get().info() << "Convert " << MODULE_NAME << " Result to JSON format done!";
}

// 标准结果转换为JSON格式的方法，第一行是header的结果数据
void Analysis::convertStandardRes2Json(std::string originalResultPath, std::string convertedJSON, std::string MODULE_NAME)
{
    // std::lock_guard<std::mutex> lock(_rapidJSONMutex); 外部已经有锁了
    std::ifstream originalFile(originalResultPath);
    Document document;
    document.SetArray();
    const char *delimiters = "\t\n";

    std::string line;
    // 读取第一行到line中获取header
    std::getline(originalFile, line);
    // 多分隔符
    std::vector<std::string> headerVector = splitMulti(line, delimiters);

    while (std::getline(originalFile, line))
    {
        std::vector<std::string> attributeVector = splitMulti(line, delimiters);
        Value jsonObject(kObjectType);
        for (size_t i = 0; i < headerVector.size(); ++i)
        {
            jsonObject.AddMember(Value(headerVector[i].c_str(), document.GetAllocator()), Value(attributeVector[i].c_str(), document.GetAllocator()), document.GetAllocator());
            // Logger::get().info() << headerVector[i] << attributeVector[i];
        }
        document.PushBack(jsonObject, document.GetAllocator());
    }

    // 将 JSON 格式的字符串输出到JSON文件中
    StringBuffer buffer;
    std::ofstream outJson;
    outJson.open(convertedJSON);
    PrettyWriter<StringBuffer> prettyWriter(buffer);
    document.Accept(prettyWriter);
    outJson << buffer.GetString() << std::endl;
    Logger::get().info() << "Convert " << MODULE_NAME << " Result to JSON format done!";
}

// ClusterTermRes to JSON
void Analysis::convertClusterTermRes2Json()
{
    std::function<void(const std::string &filepath)> convertClusterTermRes2JsonFunc =
        [this](const std::string &filePath)
    {
        // 标准方法可能不适合多\t分割的文档
        std::lock_guard<std::mutex> lock(_jsonConverterMutex);
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        std::string clusterTermFilePath = fileStruct.as2Path + "/list1.termClusteringReport.txt";
        std::string clusterTermJsonPath = fileStruct.as2Path + "/list1.termClusteringReport.json";

        convertStandardClusteredRes2Json(clusterTermFilePath, clusterTermJsonPath, "AS2_CLUSTER_TERM");
    };
    processInParallel(_vcfFileNames, convertClusterTermRes2JsonFunc, _threadNum, false);
}

void Analysis::convertClusterGeneRes2Json()
{
    std::function<void(const std::string &filepath)> convertClusterGeneRes2JsonFunc =
        [this](const std::string &filePath)
    {
        // 标准方法可能不适合多\t分割的文档
        std::lock_guard<std::mutex> lock(_jsonConverterMutex);
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        std::string clusterGeneFilePath = fileStruct.as2Path + "/list1.geneClusterReport.txt";
        std::string clusterGeneJsonPath = fileStruct.as2Path + "/list1.geneClusterReport.json";

        convertStandardClusteredRes2Json(clusterGeneFilePath, clusterGeneJsonPath, "AS2_CLUSTER_GENE");
    };
    processInParallel(_vcfFileNames, convertClusterGeneRes2JsonFunc, _threadNum, false);
}

// convert standard Clustered result file to json. Specifically build for AS2 david annotation result file.
void Analysis::convertStandardClusteredRes2Json(std::string originalResultPath, std::string convertedJSON, std::string MODULE_NAME)
{
    std::ifstream originalFile(originalResultPath);
    Document document;
    document.SetArray();

    std::string line;
    std::smatch regexMatch;
    std::regex regexClusterRow(".*Cluster.*");
    int count = 1; // count the number of cluster

    // 读取文件，每个cluster是一个json object，每个cluster的每一行是一个json object的attribute
    while (std::getline(originalFile, line))
    {
        if (std::regex_match(line, regexMatch, regexClusterRow))
        {
            Value jsonClusterObject(kArrayType);
            jsonClusterObject.SetArray();
            int countClusterMember = 1;
            std::getline(originalFile, line);
            std::vector<std::string> headerVector = splitMulti(line, "\t\n");
            std::streampos prePos = originalFile.tellg();
            while (!std::regex_match(line, regexMatch, regexClusterRow))
            {
                std::getline(originalFile, line);
                if (line.empty() || std::regex_match(line, regexMatch, regexClusterRow))
                {
                    prePos = originalFile.tellg();
                    break;
                }
                std::vector<std::string> attributeVector = splitMulti(line, "\t\n");
                Value jsonObject(kObjectType);
                jsonObject.SetObject(); // 这一步是防止后面Pushback报错必须是Object才会通过，否则会断言直接跳出。
                for (size_t i = 0; i < headerVector.size(); ++i)
                {
                    jsonObject.AddMember(Value(headerVector[i].c_str(), document.GetAllocator()), Value(attributeVector[i].c_str(), document.GetAllocator()), document.GetAllocator());
                }
                jsonObject.AddMember("Cluster", Value(std::to_string(count).c_str(), document.GetAllocator()), document.GetAllocator());
                std::string ClusterMemberName = "Cluster" + std::to_string(count) + "_Member" + std::to_string(countClusterMember);
                // jsonClusterObject.AddMember(Value(ClusterMemberName.c_str(), document.GetAllocator()), jsonObject, document.GetAllocator());
                jsonClusterObject.PushBack(jsonObject, document.GetAllocator());
                countClusterMember++;
            }
            count++;
            originalFile.seekg(prePos);
            document.PushBack(jsonClusterObject, document.GetAllocator());
        }
    }

    // 将 JSON 格式的字符串输出到JSON文件中
    StringBuffer buffer;
    std::ofstream outJson;
    outJson.open(convertedJSON);
    PrettyWriter<StringBuffer> prettyWriter(buffer);
    document.Accept(prettyWriter);
    outJson << buffer.GetString() << std::endl;
    Logger::get().info() << "Convert " << MODULE_NAME << " Result to JSON format done!";
}

void Analysis::convertProvinceRes2Json()
{
    std::function<void(const std::string &filepath)> convertProvinceRes2JsonFunc =
        [this](const std::string &filePath)
    {
        // 标准方法可能不适合多\t分割的文档
        std::lock_guard<std::mutex> lock(_jsonConverterMutex);
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        std::string provinceFilePath = fileStruct.provincePath + "/PGG_" + fileStruct.sampleId + "_000001/province/" + "PGG_" + fileStruct.sampleId + "_000001_var.txt";
        std::string provinceJsonPath = fileStruct.provincePath + "/PGG_" + fileStruct.sampleId + "_000001/province/" + "PGG_" + fileStruct.sampleId + "_000001_var.json";

        std::ifstream originalFile(provinceFilePath);
        Document document;
        document.SetArray();

        std::string line;
        std::smatch regexMatch;
        std::regex regexResPreRow("result.*");
        Value jsonObjectPercent(kObjectType);
        jsonObjectPercent.SetObject();
        while (getline(originalFile, line))
        {
            if (std::regex_match(line, regexMatch, regexResPreRow))
            {
                break;
            }
            else
            {
                std::vector<std::string> attributeVectorPercent = splitMulti(line, "\t\n");
                jsonObjectPercent.AddMember(Value(attributeVectorPercent[0].c_str(), document.GetAllocator()), Value(attributeVectorPercent[1].c_str(), document.GetAllocator()), document.GetAllocator());
            }
        }
        getline(originalFile, line);
        std::vector<std::string> attributeVector = splitMulti(line, "\t\n");
        Value jsonObject(kObjectType);
        jsonObject.SetObject();
        jsonObject.AddMember("Best_Matched_Province", Value(attributeVector[0].c_str(), document.GetAllocator()), document.GetAllocator());
        document.PushBack(jsonObject, document.GetAllocator());
        document.PushBack(jsonObjectPercent, document.GetAllocator());
        // 将 JSON 格式的字符串输出到JSON文件中
        StringBuffer buffer;
        std::ofstream outJson;
        outJson.open(provinceJsonPath);
        PrettyWriter<StringBuffer> prettyWriter(buffer);
        document.Accept(prettyWriter);
        outJson << buffer.GetString() << std::endl;
        Logger::get().info() << "Convert Province Result to JSON format done!";
    };
    processInParallel(_vcfFileNames, convertProvinceRes2JsonFunc, _threadNum, false);
}

void Analysis::aggregateJsonRes()
{
    std::function<void(const std::string &filepath)> aggregateJsonResFunc =
        [this](const std::string &filePath)
    {
        Logger::get().info() << "Aggregating JSON Results...";
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);
        std::string findPath = fileStruct.samplePath;
        const char *aggregateCommand = ("find " + findPath + " -name \"*.json\" | grep -v \"out.json\" | grep -v \"log\" | xargs -i cp {} " + fileStruct.jsonPath)
                                           .c_str();

        uint32_t stat = system(aggregateCommand);
        if (stat != 0)
        {
            _sampleFilesHashMap.erase(filePath);
            _buildFailsMap.insert(filePath, stat);
        }
        Logger::get().info() << "Aggregating JSON Results done!";
    };
    processInParallel(_vcfFileNames, aggregateJsonResFunc, _threadNum, false);
}

// Aggregate all module results.
void Analysis::aggregateRes()
{
    std::function<void(const std::string &filepath)> aggregateResFunc =
        [this](const std::string &filePath)
    {
        std::lock_guard<std::mutex> lock(_aggregateMutex);
        Logger::get().info() << "Aggregating all module Results...";
        SampleFiles fileStruct = _sampleFilesHashMap.find(filePath);

        // AS2 Module
        copyFile(fileStruct.as2Path, fileStruct.aggregateAs2Path, "as2out.seg.json", "archaic.seg.json");
        copyFile(fileStruct.as2Path, fileStruct.aggregateAs2Path, "as2out.sum.json", "archaic.sum.json");
        copyFile(fileStruct.as2Path, fileStruct.aggregateAs2Path, "as2out.seg", "archaic.seg");
        copyFile(fileStruct.as2Path, fileStruct.aggregateAs2Path, "finalChrPlot.png", "archaicChrPlot.png");
        copyFile(fileStruct.as2Path, fileStruct.aggregateAs2Path, "finalChrPlot.svg", "archaicChrPlot.svg");
        copyFile(fileStruct.as2Path, fileStruct.aggregateAs2Path, "list1.chartReport.json", "david.anno.chartReport.json");
        copyFile(fileStruct.as2Path, fileStruct.aggregateAs2Path, "list1.geneClusterReport.json", "david.anno.geneClusterReport.json");
        copyFile(fileStruct.as2Path, fileStruct.aggregateAs2Path, "list1.termClusteringReport.json", "david.anno.termClusteringReport.json");

        // Admixture Module
        copyFile(fileStruct.admixturePath, fileStruct.aggregateAdmixturePath, "NGS1926." + fileStruct.sampleId + ".ED.similarity.json", "similarity.ED.json");
        copyFile(fileStruct.admixturePath, fileStruct.aggregateAdmixturePath, "NGS1926." + fileStruct.sampleId + ".ancestry.json", "ancestry.json");

        // HLA Module
        copyFile(fileStruct.hlaPath, fileStruct.aggregateHlaPath, fileStruct.sampleId + ".VQSR.SNP.chrA_out2_hla_type_results_2_field.json", "HLA.json");

        // MT_T Module
        copyFile(fileStruct.mtyPath, fileStruct.aggregateMtyPath, fileStruct.sampleId + "/Report.json", "MT_Y.json");

        // PCA Module
        copyFile(fileStruct.pcaPath, fileStruct.aggregatePcaPath, fileStruct.sampleId + ".VQSR.SNP.chrA.NGS1926.merged.biallelic.pca.json", "PCA.json");

        // Province Module
        copyFile(fileStruct.provincePath, fileStruct.aggregateProvincePath, "Province/PGG_" + fileStruct.sampleId + "_000001/province/" + "PGG_" + fileStruct.sampleId + "_000001_var.json", "province.json");

        // PRS Module
        copyFile(fileStruct.prsPath, fileStruct.aggregatePrsPath, "results/score/aggregated_scores.json", "prs.json");
        copyFile(fileStruct.prsPath, fileStruct.aggregatePrsPath, "results/score/report.html", "prsPgsCatalogReport.html");

        // Snpedia Module
        copyFile(fileStruct.snpEdiaPath, fileStruct.aggregateSnpediaPath, "SNPedia_data_medicine_Res/" + fileStruct.sampleId + ".snpedia.json", "snpedia.medicine.json");
        copyFile(fileStruct.snpEdiaPath, fileStruct.aggregateSnpediaPath, "SNPedia_data_medicalCondiction_Res/" + fileStruct.sampleId + ".snpedia.json", "snpedia.medicalCondiction.json");

        // SV Module
        copyFile(fileStruct.svAnnoPath, fileStruct.aggregateSvPath, fileStruct.sampleId + ".sv.anno.json", "sv.anno.json");

        Logger::get().info() << "Aggregating all module results done!";
    };
    processInParallel(_vcfFileNames, aggregateResFunc, _threadNum, false);
}