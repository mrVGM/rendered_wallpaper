#include "tasks.h"
#include <threads.h>

namespace
{
    int run_func(void* arg)
    {
        using namespace tasks;

        ThreadPool* pool = static_cast<ThreadPool*>(arg);
        Channel<std::function<int(void)> >& channel = pool->getTaskChannel();

        while (true)
        {
            std::function<int(void)> task = channel.pop();
            int res = task();

            if (res)
            {
                break;
            }
        }

        return 0;
    }
}

tasks::ThreadPool::ThreadPool(int numThreads)
{
    for (int i = 0; i < numThreads; ++i)
    {
        m_threads.emplace_back();
        thrd_t& cur = m_threads.back();

        thrd_create(&cur, run_func, this);
    }
}

tasks::ThreadPool::~ThreadPool()
{
    for (auto it = m_threads.begin(); it != m_threads.end(); ++it)
    {
        m_tasksChannel.push([]() -> int {
            return 1;
        });
    }

#if false
    for (auto it = m_threads.begin(); it != m_threads.end(); ++it)
    {
        thrd_t& cur = *it;
        int res;
        thrd_join(cur, &res);
    }
#endif
}

tasks::Channel<std::function<int(void)> >& tasks::ThreadPool::getTaskChannel()
{
    return m_tasksChannel;
}

void tasks::ThreadPool::run(const std::function<void(void)>& task)
{
    m_tasksChannel.push([=]() -> int {
        task();
        return 0;
    });
}

