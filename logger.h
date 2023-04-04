//
//  logger.h
//  PersonalGenome
//
//  Created by wangbaonan on 07/26/2022.
//

#ifndef logger_h
#define logger_h
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <ctime>
#include <stdexcept>
#include <mutex>

class Logger
{
public:
    friend class StreamWriter;
    static Logger& get()
    {
        static Logger instance;
        return instance;
    }

    void setOutputFile(const std::string& filename)
    {
        struct stat buffer;
        if(stat(filename.c_str(), &buffer) == 0) //判断log是否存在 
        {
            const char* constc = nullptr;   //初始化const char*类型，并赋值为空
            constc= filename.c_str(); // string 转 char * 先建一个空指针 然后把转好的赋值
            remove(constc); //如果已有log删除原有log
        }
        std::lock_guard<std::mutex> lock(L_mutex);
        _logFile.open(filename, std::ofstream::out | std::ofstream::app);
        _logFileSet = true;
        _logFile << "--------- PersonalGenome Pipeline beta V1.0 --------- \n";
        if (!_logFile.is_open())
        {
            throw std::runtime_error("Can't open log file");
        }
    }

    void setDebugging(bool debug) {_debug = debug;}

    class StreamWriter
    {
    public:
        StreamWriter(const std::string& level,
                     std::ostream* consoleStream = nullptr,
                     std::ostream* fileStream = nullptr):
            _fileStream(fileStream), _consoleStream(consoleStream)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_fileStream) *_fileStream << timestamp() << " " << level << " ";
            if (_consoleStream) *_consoleStream << timestamp()
                                                << " " << level << " ";
        }
        ~StreamWriter()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_fileStream) *_fileStream << std::endl;
            if (_consoleStream) *_consoleStream << std::endl;
        }
        
        template <class T>
        Logger::StreamWriter& operator<< (const T& val)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_fileStream) *_fileStream << val;
            if (_consoleStream) *_consoleStream << val;
            return *this;
        }

    private:
        std::ostream* _fileStream;
        std::ostream* _consoleStream;
        std::mutex _mutex;
    };

    StreamWriter info()
    {   
        std::lock_guard<std::mutex> lock(L_mutex);
        std::ostream* logPtr = _logFileSet ? &_logFile : nullptr;
        return StreamWriter("[INFO]:", &std::cerr, logPtr);
    }

    StreamWriter warning()
    {
        std::ostream* logPtr = _logFileSet ? &_logFile : nullptr;
        return StreamWriter("[WARNING]:", &std::cerr, logPtr);
    }

    StreamWriter error()
    {
        std::ostream* logPtr = _logFileSet ? &_logFile : nullptr;
        return StreamWriter("[ERROR]:", &std::cerr, logPtr);
    }

    StreamWriter debug()
    {
        std::ostream* logPtr = _logFileSet ? &_logFile : nullptr;
        std::ostream* consolePtr = _debug ? &std::cerr : nullptr;
        return StreamWriter("[DEBUG]:", consolePtr, logPtr);
    }

private:
    static std::string timestamp(const char* format = "[%Y-%m-%d %H:%M:%S]")
    {
        std::time_t t = std::time(0);
        char cstr[256];
        std::strftime(cstr, sizeof(cstr), format, std::localtime(&t));
        return cstr;
    }

    Logger():
        _debug(false), _logFileSet(false)
    {std::lock_guard<std::mutex> lock(L_mutex);}
    ~Logger()
    {
        std::lock_guard<std::mutex> lock(L_mutex);
        if (_logFileSet)
        {
            _logFile << "----------- PersonalGenome Pipeline work done! ------------\n";
        }
    }

    bool _debug;
    bool _logFileSet;
    std::ofstream _logFile;
    std::mutex L_mutex;
};


#endif /* logger_h */
