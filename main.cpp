//
//  main.cpp
//  PersonalGenome
//
//  Created by wangbaonan on 07/26/2022.
//

#include <iostream>
#include <getopt.h>
#include <vector>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "logger.h"
#include "parallel.h"
#include "build_files.h"
#include "analysis.h"

bool parseArgs(int argc, char **argv, std::string &inputdir, std::string &outputdir, std::string &configfile)
{
    auto printUsage = []()
    {
        std::cerr << "Usage : Personal_Genome Pipeline \n"
                  << "--indir: input directory that contain vcf file \n"
                  << "--outdir output directory \n"
                  << "--config configuration file \n";
    };
    int optionIndex = 0;
    static option longOptions[] =
        {
            {"indir", required_argument, 0, 0},
            {"outdir", required_argument, 0, 0},
            {"config", required_argument, 0, 0},
            {0, 0, 0, 0}};
    int opt = 0;
    while ((opt = getopt_long(argc, argv, "h", longOptions, &optionIndex)) != -1)
    {
        switch (opt)
        {
        case 0:
            if (!strcmp(longOptions[optionIndex].name, "indir"))
                inputdir = optarg;
            else if (!strcmp(longOptions[optionIndex].name, "outdir"))
                outputdir = optarg;
            else if (!strcmp(longOptions[optionIndex].name, "config"))
                configfile = optarg;
            break;

        case 'h':
            printUsage();
            exit(0);
        }
    }
    if (inputdir.empty() || outputdir.empty())
    {
        printUsage();
        return false;
    }
    return true;
}

// 返回已完成模块CODE
std::vector<int> generateTestSuccessfulModuleCode()
{
    return {ADMIXTURE_SWITCH, AS2_SWITCH, HLA_SWITCH, MTY_SWITCH, PCA_SWITCH, PRS_SWITCH, SV_SWITCH, PROVINCE_SWITCH};
}

void ProductionMode(cuckoohash_map<std::string, SampleFiles> sampleFilesHashMap, std::vector<std::string> vcfFileNames, std::string processBarFile)
{
    // 为0时即不适用任何计算模块 1022为使用所有计算模块(除NGS gVCF的RegionalCalling以外) ALL_MODULE , PCA_SWITCH + ADMIXTURE_SWITCH + AS2_SWITCH +  MTY_SWITCH + HLA_SWITCH + SNPEDIA_SWITCH + PRS_SWITCH
    Analysis analysisInstance(sampleFilesHashMap, vcfFileNames, (ALL_MODULE));
    // 提取出相应VCF的常染色体文件并将文本名保存在私有变量当中
    analysisInstance.preExtractAutosomesVCF();
    // 加载并异步执行所有需要的分析模块
    std::vector<int> successfulModuleCode = analysisInstance.launchLoadedModule(processBarFile);
    // 将结果转换为JSON格式用于前端展示
    analysisInstance.convertRes2Json(successfulModuleCode);
    // 整合结果
    analysisInstance.aggregateRes();
}

void DevelopmentProvinceMode(cuckoohash_map<std::string, SampleFiles> sampleFilesHashMap, std::vector<std::string> vcfFileNames, std::string processBarFile)
{
    // 为0时即不适用任何计算模块 510为使用所有计算模块(除NGS gVCF的RegionalCalling以外) ALL_MODULE , PCA_SWITCH + ADMIXTURE_SWITCH + AS2_SWITCH +  MTY_SWITCH + HLA_SWITCH + SNPEDIA_SWITCH + PRS_SWITCH
    Analysis analysisInstance(sampleFilesHashMap, vcfFileNames, (PROVINCE_SWITCH));
    // 提取出相应VCF的常染色体文件并将文本名保存在私有变量当中
    // analysisInstance.preExtractAutosomesVCF();
    // 加载并异步执行所有需要的分析模块
    std::vector<int> successfulModuleCode = analysisInstance.launchLoadedModule(processBarFile);
    // 将结果转换为JSON格式用于前端展示
    // analysisInstance.convertRes2Json(generateTestSuccessfulModuleCode());
}

void DevelopmentMode(cuckoohash_map<std::string, SampleFiles> sampleFilesHashMap, std::vector<std::string> vcfFileNames)
{
    // 为0时即不适用任何计算模块 510为使用所有计算模块(除NGS gVCF的RegionalCalling以外) ALL_MODULE , PCA_SWITCH + ADMIXTURE_SWITCH + AS2_SWITCH +  MTY_SWITCH + HLA_SWITCH + SNPEDIA_SWITCH + PRS_SWITCH
    Analysis analysisInstance(sampleFilesHashMap, vcfFileNames, (ALL_MODULE));
    // 提取出相应VCF的常染色体文件并将文本名保存在私有变量当中
    // analysisInstance.preExtractAutosomesVCF();
    // 加载并异步执行所有需要的分析模块
    // std::vector<int> successfulModuleCode = analysisInstance.launchLoadedModule(processBarFile);
    //  将结果转换为JSON格式用于前端展示
    analysisInstance.convertRes2Json(generateTestSuccessfulModuleCode());
    analysisInstance.aggregateRes();
}

int main(int argc, char **argv)
{
    std::string inputdir;
    std::string outputdir;
    std::string configfile;
    // bool debugging = true;
    bool debugging = true;
    if (!parseArgs(argc, argv, inputdir, outputdir, configfile))
        return 1; // 传引用修改值
    Config::load(configfile);
    std::string logFile = outputdir + "/PersonalGenome.log";
    std::string processBarFile = outputdir + "/PGP.process"; // 传入launchLoadedModule()
    Logger::get().setDebugging(debugging);
    if (!logFile.empty())
        Logger::get().setOutputFile(logFile);
    if (inputdir.back() != '/')
        inputdir.push_back('/');
    Logger::get().info() << "@inputdir: \n"
                         << inputdir;
    Logger::get().info() << "@outputdir: \n"
                         << outputdir;
    Logger::get().info() << "@configfile: \n"
                         << configfile;

    std::string suffix = Config::get("suffix"); // 获取Config中的输入文件后缀
    FileContainer files(inputdir, suffix);      // 获取文件夹中符合后缀的所有文件
    files.getfilenames();
    auto filesInfo = getinfo(files);
    Logger::get().info() << "[ Loading files ] " << filesInfo.filenumbers << " files have been found.";
    Logger::get().info() << "[ Loading files ] " << filesInfo.totalsize << "  totol size of files.";
    std::vector<std::string> vcfFileNames = filesInfo.filenames; // 所有样本的文件名

    // Build analysis paths
    BuildFiles bfInstance(vcfFileNames, outputdir);
    auto sampleFilesHashMap = bfInstance.getSamplesFiles(); // <fullpath, SampleFile_instance> pair

    // Launch loaded modules // 整个流程是解耦模块化的 如果后续需要添加NGS流程则可以再添加一个模块然后由参数来决定是从什么输入数据分析起步
    // Analysis analysisInstance(sampleFilesHashMap, vcfFileNames, (ALL_MODULE));  // 为0时即不适用任何计算模块 510为使用所有计算模块(除NGS gVCF的RegionalCalling以外) ALL_MODULE , PCA_SWITCH + ADMIXTURE_SWITCH + AS2_SWITCH +  MTY_SWITCH + HLA_SWITCH + SNPEDIA_SWITCH + PRS_SWITCH
    // analysisInstance.preExtractAutosomesVCF();    // 提取出相应VCF的常染色体文件并将文本名保存在私有变量当中
    // std::vector<int> successfulModuleCode = analysisInstance.launchLoadedModule(processBarFile);        // 加载并异步执行所有需要的分析模块
    //  将结果转换为JSON格式用于前端展示
    // analysisInstance.convertRes2Json(generateTestSuccessfulModuleCode());
    // analysisInstance.convertRes2Json(successfulModuleCode);

    // 开发者模式，测试JSON转换
    // DevelopmentMode(sampleFilesHashMap, vcfFileNames);

    // 开发者模式，测试省份模式
    // DevelopmentProvinceMode(sampleFilesHashMap, vcfFileNames, processBarFile);

    // 生产模式，包括所有模块
    ProductionMode(sampleFilesHashMap, vcfFileNames, processBarFile);

    return 0;
}
