//
//  progress_bar.hpp
//  human_pangenome
//
//  Created by wangbaonan on 03/07/2022.
//

#ifndef progress_bar_h
#define progress_bar_h

#include <atomic>
#include <iostream>

class ProgressPercent
{
public:
    ProgressPercent(size_t finalCount = 0):
        _finalCount(finalCount), _curCount(0), _prevPercent(-1),
        _stopped(false)
    {}

    void setFinalCount(size_t finalCount) {_finalCount = finalCount;}
    void setValue(size_t value)
    {
        this->advance(value - _curCount);
    }
    void setDone()
    {
        this->setValue(_finalCount);
    }
    void advance(size_t step = 1)
    {
        if (_stopped) return;

        _curCount += step;
        int percent = 10UL * _curCount / _finalCount;

        if (percent > _prevPercent)
        {
            int expected = _prevPercent;
            if (_prevPercent.compare_exchange_weak(expected, percent))
            {
                std::cerr << percent * 10 << "% ";
                if (percent >= 10)
                {
                    std::cerr << std::endl;
                    _stopped = true;
                }
            }
        }
    }

private:
    size_t                 _finalCount;
    std::atomic<size_t> _curCount;
    std::atomic<int>      _prevPercent;
    bool                 _stopped;
};
#endif /* progress_bar_h */
