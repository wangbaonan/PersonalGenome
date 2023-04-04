//
//  build_files.cpp
//  PersonalGenome
//
//  Created by wangbaonan on 07/26/2022.
//

#include "build_files.h"
#include "utils.h"
#include <string.h>

cuckoohash_map<std::string, SampleFiles> BuildFiles::getSamplesFiles(){
    Logger::get().info()<<"Build paths for "<< _vcfFileNames.size() << " samples." <<"\n";
    for(auto fullPath : _vcfFileNames){
        std::string dirPath;
        std::string filePrefix; // 文件名前缀
        std::string sampleId;   // 样本ID
        std::string allChrFilePath;
        std::string chrADir;
        std::string svDir;
        std::string svPath;
        size_t slash = fullPath.find_last_of("/");
        //size_t dotPos = fullPath.find(".");
        size_t dotPos_r2 = r_pos(fullPath,".",2);
        dirPath = fullPath.substr(0,slash);
        filePrefix = std::string(&fullPath[slash+1],&fullPath[dotPos_r2]);
        //sampleId = std::string(&fullPath[slash+1],&fullPath[dotPos]);
        sampleId = SystemWithResult(("bcftools query -l " + fullPath).c_str());

        allChrFilePath = fullPath;//包含MT Y X染色体的VCF
        chrADir = dirPath + "/chrAVCF";
        fs::create_directories(chrADir);
        fullPath = chrADir + "/" + filePrefix + ".chrA.vcf.gz";//仅包含chrA的数据完整路径  如果要添加新的file 输入输出也应在这里添加后通过结构体取出
        svDir = dirPath + "/SV";
        fs::create_directories(svDir);
        svPath = svDir + "/" + filePrefix + ".SV.vcf";//仅包含SV的数据完整路径

        Logger::get().debug() << "SampleID: " << sampleId;
        Logger::get().debug() << "fullPath   :" << fullPath;
        Logger::get().debug() << "dirPath    :" << dirPath;
        Logger::get().debug() << "filePrefix :" << filePrefix;
        Logger::get().debug() << "sampleId   :" << sampleId;
        Logger::get().debug() << "allChrFilePath   :" << allChrFilePath;
        Logger::get().debug() << "svPath :" << svPath;
        Logger::get().debug() << "\n";
        SampleFiles SampleFiles_instance(sampleId, fullPath, allChrFilePath, svPath);//构建SampleFiles 实例时需要传入ID以及两种类型的输入文件
        SampleFiles_instance.getPaths(_outputPath);
        _samplesFilesMap.insert(allChrFilePath, SampleFiles_instance); //这个map里是以allChr也就是原始文件作为索引的 所以后续的传入也都需要传入最开始的vcffileName
        //再通过这个接口取得对应的chrAfilePath即常染色体名称路径
    }
    return _samplesFilesMap;
}