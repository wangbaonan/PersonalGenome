//
//  parallel.h
//  human_pangenome
//
//  Created by wangbaonan on 03/07/2022.
//

#ifndef parallel_h
#define parallel_h

#include <vector>
#include <functional>
#include <atomic>
#include <thread>

#include "progress_bar.h"

//simple thread pool implementation
//updateFun should be thread-safe!
template <class T>
void processInParallel(const std::vector<T>& scheduledTasks,  //这里的意思就是我们有几个共有的参数： jobId
                       std::function<void(const T&)> updateFun,
                       size_t maxThreads, bool progressBar)
{
    if (scheduledTasks.empty()) return;

    std::atomic<size_t> jobId(0);
    ProgressPercent progress(scheduledTasks.size());
    if (progressBar) progress.advance(0);

    auto threadWorker = [&jobId, &scheduledTasks, &updateFun,
                         &progress, progressBar]()
    {
        while (true)
        {
            size_t expected = 0;
            while(true)
            {
                expected = jobId;
                if (jobId == scheduledTasks.size())
                {
                    return;
                }
                if (jobId.compare_exchange_weak(expected, expected + 1))
                {
                    break;
                }
            }
            updateFun(scheduledTasks[expected]);
            if (progressBar) progress.advance();
        }
    };

    std::vector<std::thread> threads(std::min(maxThreads,
                                              scheduledTasks.size()));
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i] = std::thread(threadWorker);
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
}

#endif /* parallel_h */
