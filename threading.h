#pragma once

#include <list>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>

#include "cpputilities_global.h"
#include "signals_slots.h"

namespace CppUtilities {

class AbstractExecutor;
template<class C, class ... Args> class GenericFunctor;
template<class C, class ... Args> class GenericExecutor;

class ThreadLooping;
class SingleLooping;
class AbstractThread;

#ifdef THREAD_TRACKING

class ThreadTracker;
class AbstractThreadTracking;

class ThreadTracker {
public:
    inline static ThreadTracker *get() {
        static ThreadTracker *inst = new ThreadTracker;
        return inst;
    }

    std::list<int> get_running();
    std::list<int> get_stopped();
    int next_id();
    AbstractThread *get_thread(int id);

private:
    std::list<int> running_ones;
    std::list<int> stopped_ones;
    std::map<int, AbstractThread *> mapped;

    void add_thread(int id, AbstractThread *t);
    void ran(int);
    void stopped(int);
    void remove_thread(int id);

    std::mutex mtx;

    friend class AbstractThread;
};

class AbstractThreadTracking
{
public:
    inline static int last_id = 0;
    int allocated_id = -2;
    friend class ThreadTracker;
};

class AbstractThread : private AbstractThreadTracking
#else
class AbstractThread
#endif
{
public:
    explicit AbstractThread(std::string sn = "Undefined");
    virtual ~AbstractThread();
    virtual bool is_running();

    //Be aware that when a callback is done, it is deleted! And the execution depends on the implementation!
    virtual void add_callback(AbstractExecutor *to_execute);
    template<class C = void> inline void add_callback(GenericFunctor<C> *to_execute);

    virtual void start();
    virtual void stop();
    virtual void wait_for_ends();
    virtual void pause_s(int secs);
    virtual void pause_ms(int msecs);
    virtual void process();

    int get_id();

    std::string name() {
#ifdef THREAD_NAME_USE
        return _name;
#else
        return "";
#endif
    };
    void set_name(std::string n) {
#ifdef THREAD_NAME_USE
        _name = n;
#endif
    };

protected:
    virtual void looping();
    std::thread *loop = nullptr;
    std::atomic<bool> loop_enable = {false};
    std::list<AbstractExecutor *> cb_schd_list;
    std::list<AbstractExecutor *> waits_list;
    mutable std::mutex mtx;
    bool is_waiting = false;
    bool stopped_its = false; //In case the thread itself wanted to stop (a func running in thread called stop()), so enable delete() and new() recycle later by using this.

private:
#ifdef THREAD_TRACKING
    friend class ThreadTracker;
#endif
#ifdef THREAD_NAME_USE
    std::string _name;
#endif
};

class ThreadLooping : public AbstractThread
{
public:
    explicit ThreadLooping(std::string sn = "Undefined");
    ~ThreadLooping() override;

    void add_routine(AbstractExecutor *routine);
    template<class C> inline void add_routine(GenericFunctor<C> *to_execute);

protected:
    void looping() override;

private:
    std::list<AbstractExecutor *> rout_list;
};

class SingleLooping : public AbstractThread
{
public:
    explicit SingleLooping(std::string sn, AbstractExecutor *func, bool auto_delete = true);
    SingleLooping(AbstractExecutor *func, bool auto_delete = true);
    template<class C> inline SingleLooping(std::string sn, GenericFunctor<C> *func, bool auto_delete = true);
    template<class C> inline SingleLooping(GenericFunctor<C> *func, bool auto_delete = true);
    ~SingleLooping();

protected:
    void looping() override;

private:
    AbstractExecutor *func;
};

//Here are the template functions defs
template <class C> inline
void AbstractThread::add_callback(GenericFunctor<C> *f)
{
    add_callback(new GenericExecutor<C>(f));
}

template <class C> inline
void ThreadLooping::add_routine(GenericFunctor<C> *f)
{
    add_routine(new GenericExecutor<C>(f));
}
template<class C> inline
SingleLooping::SingleLooping(std::string sn, GenericFunctor<C> *executor, bool del) : AbstractThread(sn)
{
    func = new GenericFunctor<C>(executor);
    start();
    stop();

    if (del) {
        delete func;
        delete executor;
    }
}

template<class C> inline
SingleLooping::SingleLooping(GenericFunctor<C> *executor, bool del) : SingleLooping("Undefined", executor, del)
{
}

};
