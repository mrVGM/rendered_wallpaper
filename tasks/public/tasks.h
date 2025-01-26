#pragma once

#include <threads.h>
#include <vector>
#include <queue>
#include <functional>

namespace tasks
{
    template <typename T>
    class Channel
    {
    private:
        mtx_t m_mtx;
        cnd_t m_cnd;
        std::queue<T> m_queue;

    public:
        Channel()
        {
            mtx_init(&m_mtx, mtx_plain);
            cnd_init(&m_cnd);
        }
        ~Channel()
        {
            mtx_destroy(&m_mtx);
            cnd_destroy(&m_cnd);
        }
        
        void push(const T& item)
        {
            mtx_lock(&m_mtx);
            m_queue.push(item);
            mtx_unlock(&m_mtx);

            cnd_signal(&m_cnd);
        }

        T pop()
        {
            mtx_lock(&m_mtx);
            while (m_queue.empty())
            {
                cnd_wait(&m_cnd, &m_mtx);
            }
            T res = m_queue.front();
            m_queue.pop();
            mtx_unlock(&m_mtx);

            cnd_signal(&m_cnd);

            return res;
        }
    };


    class ThreadPool
    {
    private:
        std::vector<thrd_t> m_threads;
        Channel<std::function<int(void)> > m_tasksChannel;

    public:
        ThreadPool(int numThreads);
        ~ThreadPool();
        
        Channel<std::function<int(void)> >& getTaskChannel();
        void run(const std::function<void(void)>& task);
    };

}
