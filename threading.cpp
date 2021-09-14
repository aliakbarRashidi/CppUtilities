#include "threading.h"

#include <iostream>
#include <chrono>

namespace CppUtilities {

#ifdef THREAD_TRACKING

std::list<int> ThreadTracker::get_running()
{
    return running_ones;
}

std::list<int> ThreadTracker::get_stopped()
{
    return stopped_ones;
}

int ThreadTracker::next_id()
{
    return AbstractThreadTracking::last_id + 1;
}

AbstractThread *ThreadTracker::get_thread(int id)
{
    return mapped[id];
}

void ThreadTracker::add_thread(int id, AbstractThread *t)
{
    mtx.lock();
    mapped.insert({id, t});
    mtx.unlock();
}

void ThreadTracker::ran(int id)
{
    mtx.lock();
    stopped_ones.remove(id);
    running_ones.push_back(id);
    mtx.unlock();
}

void ThreadTracker::stopped(int id)
{
    mtx.lock();
    running_ones.remove(id);
    stopped_ones.push_back(id);
    mtx.unlock();
}

void ThreadTracker::remove_thread(int id)
{
    mtx.lock();
    mapped.erase(id);
    stopped_ones.remove(id);
    running_ones.remove(id);
    mtx.unlock();
}

AbstractThread::AbstractThread(std::string sn) : AbstractThreadTracking()
{
    _name = sn;
    last_id++;
    allocated_id = last_id;
    ThreadTracker::get()->add_thread(allocated_id, this);
}
#else
AbstractThread::AbstractThread(std::string sn)
{
    _name = sn;
}
#endif

AbstractThread::~AbstractThread()
{
    if (loop) {
        stop();
    }
    for (AbstractExecutor *cb : cb_schd_list) {
        delete cb;
    }
    cb_schd_list.clear();

#ifdef THREAD_TRACKING
    ThreadTracker::get()->remove_thread(allocated_id);
#endif
}

int AbstractThread::get_id()
{
#ifdef THREAD_TRACKING
    return allocated_id;
#else
    return -1;
#endif
}

void AbstractThread::looping()
{
    process();
}

bool AbstractThread::is_running()
{
    return loop_enable;
}

void AbstractThread::pause_s(int secs)
{
    waits_list.push_back(new GenericExecutor<>(std::bind(
                    ((void (*)(const std::chrono::seconds &))&std::this_thread::sleep_for),
                    std::chrono::seconds(secs)
                )
            )
        );
}

void AbstractThread::pause_ms(int msecs)
{
    waits_list.push_back(new GenericExecutor<>(std::bind(
                    ((void (*)(const std::chrono::milliseconds &))&std::this_thread::sleep_for),
                    std::chrono::milliseconds(msecs)
                )
            )
        );
}

void AbstractThread::process()
{
    for (AbstractExecutor *cb : cb_schd_list) {
        cb->execute();
        delete cb;
        for (AbstractExecutor *wait : waits_list) {
            wait->execute();
            delete wait;
        }
        waits_list.clear();
    }
    cb_schd_list.clear();
}

void AbstractThread::stop()
{
    mtx.lock();
    loop_enable = false; // should be modified inside mutex lock
    if (loop) {
        if (std::this_thread::get_id() == loop->get_id()) {
            //Means it came from inside!
            stopped_its = true;
        } else if (loop->joinable()) {
            loop->join();
            loop->~thread();
            delete loop;
            loop = nullptr;
        }
    }

#ifdef THREAD_TRACKING
    ThreadTracker::get()->stopped(allocated_id);
#endif

    mtx.unlock();
}

void AbstractThread::wait_for_ends()
{
    if (loop) {
        //Never use join() here! CPPRef says join() called from different threads cause race conditions.
        //And most of the time it does freeze. Instead use a classical while.
        //The thread itself never waits for its own end, it just pass out.
        if (std::this_thread::get_id() != loop->get_id()) {
            while(loop_enable) {}
        }
    }
}

void AbstractThread::start()
{
#ifdef THREAD_TRACKING
    ThreadTracker::get()->ran(allocated_id);
#endif

    if (loop == nullptr) {
        loop_enable = true;
        loop = new std::thread([this](){this->looping();});
    } else if (stopped_its) {
        loop->~thread();
        delete loop;
        loop_enable = true;
        loop = new std::thread([this](){this->looping();});
    }
}

void AbstractThread::add_callback(AbstractExecutor *cb)
{
    mtx.lock();
    cb_schd_list.push_back(cb);
    mtx.unlock();
}

ThreadLooping::ThreadLooping(std::string sn) : AbstractThread(sn)
{
}

ThreadLooping::~ThreadLooping()
{
    for (AbstractExecutor *r : rout_list) {
        delete r;
    }
    rout_list.clear();
    AbstractThread::~AbstractThread();
}

void ThreadLooping::add_routine(AbstractExecutor *exec)
{
    mtx.lock();
    rout_list.push_back(exec);
    mtx.unlock();
}

void ThreadLooping::looping()
{
    while (loop_enable) {
        for (AbstractExecutor *r : rout_list) {
            r->execute();
            //Process all between a routine all the time, ensures good responding with cbs and waits.
            AbstractThread::looping();
        }
    }
}

SingleLooping::SingleLooping(std::string sn, AbstractExecutor *executor, bool del) : AbstractThread(sn)
{
    func = executor;
    start();
    stop();

    if (del) {
        delete executor;
        func = nullptr;
    }
}

SingleLooping::SingleLooping(AbstractExecutor *executor, bool del) : SingleLooping("Undefined", executor, del)
{
}

SingleLooping::~SingleLooping()
{
    AbstractThread::~AbstractThread();
}

void SingleLooping::looping()
{
    func->execute();
    AbstractThread::looping();
    loop_enable = false;
}

}
