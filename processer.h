//
//  Processer.h
//  PersonalGenome
//
//  Created by wangbaonan on 07/26/2022.
//

#ifndef processer_h
#define processer_h
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <ctime>
#include <stdexcept>

class Processer
{
public:
    static Processer& get()
    {
        static Processer instance;
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
        _logFile.open(filename, std::ofstream::out | std::ofstream::app);
        _logFileSet = true;
        _logFile << "Process: \n";
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
            if (_fileStream) *_fileStream << level << " ";
            if (_consoleStream) *_consoleStream  << level << " ";
        }
        ~StreamWriter()
        {
            if (_fileStream) *_fileStream << std::endl;
            if (_consoleStream) *_consoleStream << std::endl;
        }
        
        template <class T>
        Processer::StreamWriter& operator<< (const T& val)
        {
            if (_fileStream) *_fileStream << val;
            if (_consoleStream) *_consoleStream << val;
            return *this;
        }

    private:
        std::ostream* _fileStream;
        std::ostream* _consoleStream;
    };

    StreamWriter process()
    {
        std::ostream* logPtr = _logFileSet ? &_logFile : nullptr;
        return StreamWriter("[PROCESS]: ", &std::cerr, logPtr);
    }

    

private:
    static std::string timestamp(const char* format = "[%Y-%m-%d %H:%M:%S]")
    {
        std::time_t t = std::time(0);
        char cstr[256];
        std::strftime(cstr, sizeof(cstr), format, std::localtime(&t));
        return cstr;
    }

    Processer():
        _debug(false), _logFileSet(false)
    {}
    ~Processer()
    {
        if (_logFileSet)
        {
            _logFile << "Done";
        }
    }

    bool _debug;
    bool _logFileSet;
    std::ofstream _logFile;
};


#endif /* processer_h */
