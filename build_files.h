//
//  build_files.h
//  PersonalGenome
//
//  Created by wangbaonan on 07/26/2022.
//

#ifndef build_files_h
#define build_files_h

#include "cuckoohash_map.hh"
#include "config.h"
#include "utils.h"

struct SampleFiles
{
    SampleFiles(std::string& sampleId, std::string& chrAPath, std::string& allChrPath, std::string& svPath):
        sampleId(sampleId), chrAPath(chrAPath), allChrPath(allChrPath), svPath(svPath)
    {}
    void getPaths(std::string& outpath){
        samplePath = outpath + "/" + sampleId;
        std::string mkdirOpt = "mkdir -p ";
        fs::create_directories(samplePath);//创建对应ID的目录
        pcaPath = samplePath + "/PCA";
        as2Path = samplePath + "/AS2";
        prsPath = samplePath + "/PRS";
        ngsPath = samplePath + "/NGS";
        mtyPath = samplePath + "/MT_Y";
        hlaPath = samplePath + "/HLA";
        snpEdiaPath = samplePath + "/SNPEDIA";
        admixturePath = samplePath + "/ADMIXTURE";
        svAnnoPath = samplePath + "/SV_Anno";
        provincePath = samplePath + "/Province";
        jsonPath = samplePath + "/Res_JSON";
        //processPath = samplePath + "/ProcessBar";
        //processFile = processPath + "/ProcessBar.txt";
        aggregateAdmixturePath = jsonPath + "/Admixture";
        aggregateAs2Path = jsonPath + "/AS2";
        aggregateHlaPath = jsonPath + "/HLA";
        aggregateMtyPath = jsonPath + "/MT_Y";
        aggregatePcaPath = jsonPath + "/PCA";
        aggregateProvincePath = jsonPath + "/Province";
        aggregatePrsPath = jsonPath + "/PRS";
        aggregateSnpediaPath = jsonPath + "/Snpedia";
        aggregateSvPath = jsonPath + "/SV";

        fs::create_directories(pcaPath);
        fs::create_directories(as2Path);
        fs::create_directories(prsPath);
        fs::create_directories(ngsPath);
        fs::create_directories(mtyPath);
        fs::create_directories(hlaPath);
        fs::create_directories(snpEdiaPath);
        fs::create_directories(admixturePath);
        fs::create_directories(svAnnoPath);
        fs::create_directories(provincePath);
        fs::create_directories(jsonPath);

        fs::create_directories(aggregateAdmixturePath);
        fs::create_directories(aggregateAs2Path);
        fs::create_directories(aggregateHlaPath);
        fs::create_directories(aggregateMtyPath);
        fs::create_directories(aggregatePcaPath);
        fs::create_directories(aggregateProvincePath);
        fs::create_directories(aggregatePrsPath);
        fs::create_directories(aggregateSnpediaPath);
        fs::create_directories(aggregateSvPath);
        //fs::create_directories(processPath);
    }
    //Initial 
    std::string sampleId;
    std::string samplePath;
    //Analysis Path
    std::string pcaPath;
    std::string as2Path;
    std::string prsPath;
    std::string ngsPath;
    std::string mtyPath;
    std::string hlaPath;
    std::string snpEdiaPath;
    std::string admixturePath;
    std::string svAnnoPath;
    std::string chrAPath;//常染色体的VCF路径
    std::string allChrPath;//包含MTXY染色体的VCF路径
    std::string svPath;//仅包含SV的VCF 用于annotation
    std::string provincePath;
    std::string jsonPath;
    //Aggregate Path
    std::string aggregateAdmixturePath;
    std::string aggregateAs2Path;
    std::string aggregateHlaPath;
    std::string aggregateMtyPath;
    std::string aggregatePcaPath;
    std::string aggregateProvincePath;
    std::string aggregatePrsPath;
    std::string aggregateSnpediaPath;
    std::string aggregateSvPath;
    //Session ProcessBar Data Path
    //std::string processPath;
    //std::string processFile;
};

class BuildFiles
{
public:
    BuildFiles(std::vector<std::string>& vcfFileNames, std::string& outputPath):
        _vcfFileNames(vcfFileNames), _outputPath(outputPath)
    {}
    cuckoohash_map<std::string, SampleFiles> getSamplesFiles();// 创建sampleFIle哈希
    void getSampleIds();
private:
/*     std::string _fullPath;
    std::string _dirPath;
    std::string _fileName; */

    cuckoohash_map<std::string, SampleFiles> _samplesFilesMap;
    std::vector<std::string> _vcfFileNames;
    std::vector<std::string> _sampleIds;
    std::string _outputPath;
};

#endif 