#pragma once

#include "cpputilities_global.h"

#include "threading.h"
#include "cxxabi.h"

#include <iostream>
#include <utility>
#include <functional>
#include <string>
#include <list>
#include <map>
#include <mutex>

namespace CppUtilities {
class AbstractThread;

/**
 * Due to the fact that templates' definitions cannot be in a source
 * file, put member functions' declarations as inline. And after def
 * ine it. It avoids member inaccessibility when a definition is mad
 * e before a class is declared (as usually it does not happen with
 * a source file and a header, all separated).
 **/

template<class C, class ... Args> class GenericFunctor;

//Signal/Slot Data Set
// Use that for better accessibility
class SSDSet
{
public:
    inline explicit SSDSet(std::string sn = "Undefined", size_t fsl = 0, std::list<std::string> sign = {})
        :
#ifdef SIGSOT_META_USE
          _sign(sign), _fsl(fsl), _name(sn)
#endif
    {}
    inline virtual ~SSDSet() {_sign.clear();};

    inline void set_name(std::string n) {
#ifdef SIGSOT_META_USE
        _name = n;
#endif
    }

    inline std::string name() const {
#ifdef SIGSOT_META_USE
        return _name;
#else
        return "";
#endif
    }

    inline size_t functor_sign_length() {
#ifdef SIGSOT_META_USE
        return _fsl;
#else
        return 0;
#endif
    };
    inline const std::list<std::string> signature() {
#ifdef SIGSOT_META_USE
        return _sign;
#else
        return {};
#endif
    };
    inline virtual std::string get_type() {
        return "Undefined";
    };

    inline operator const SSDSet *() {
        return this;
    }
private:
#ifdef SIGSOT_META_USE
    std::list<std::string> _sign;
    const size_t _fsl;
    std::string _name = "Undefined";
#endif
};

//It lets you call execute() without knowing what it contains, btw, you can use <int>, <void *>, <double, int, char[2]> and a lot more.
class AbstractExecutor : public SSDSet
{
public:
    inline explicit AbstractExecutor(std::string sn = "Undefined", size_t fsl = 0, std::list<std::string> sign = {}) : SSDSet(sn, fsl, sign) {};
    virtual void execute() = 0;
};

template<class C = void, class ... Args>
class GenericExecutor : public AbstractExecutor
{
public:
    using function_t = std::function<C(Args ...)>;
    using binded_t = decltype(std::bind(std::declval<function_t>(), std::declval<Args>() ...));

    inline explicit GenericExecutor(function_t func, Args ... vals);
    inline GenericExecutor(std::string, function_t func, Args ... vals);
    inline GenericExecutor(GenericFunctor<C, Args ...> *src, Args ... vals);

    inline C run();
    inline void execute() override;
    inline std::string get_type() override;

    static const std::tuple<Args ...> functor_model;

    inline explicit operator GenericExecutor<C> *() {
        return new GenericExecutor<C>(_bind);
    }

    inline operator GenericExecutor<C>() {
        return GenericExecutor<C>(_bind);
    }

private:
    binded_t _bind;
};

template<class C>
class GenericExecutor<C> : public AbstractExecutor
{
public:
    using function_t = std::function<C()>;
    using binded_t = decltype(std::bind(std::declval<function_t>()));

    inline explicit GenericExecutor(function_t func);
    inline GenericExecutor(std::string, function_t func);
    inline GenericExecutor(GenericFunctor<C> *src);

    inline C run();
    inline void execute() override;
    inline std::string get_type() override;

    static const std::tuple<> functor_model;

    inline explicit operator GenericExecutor<void> *() {
        return new GenericExecutor<void>(_bind);
    }

    inline operator GenericExecutor<void>() {
        return GenericExecutor<void>(_bind);
    }

private:
    binded_t _bind;
};


//Overlay used for signals, e.g.: you want to pass a GenericFunctor<int>, or a GenericFunctor<int, int> to a SignalMulti<void, int>: use an AnonymousFunctor!
//As the return type of GenericFunctor does not matter in a signal, it is ok to use int for a void or anything else.
//For the signals, instead of passing arguments to GenericFunctor<int>, when it is GenericFunctor<void, int>, just don't pass the args :)


template<class ... Args>
class ArgsAnonymousFunctor;
class AnonymousFunctor : public SSDSet
{
public:
    inline explicit AnonymousFunctor(std::string sn, size_t fsl, std::list<std::string> sign) : SSDSet(sn, fsl, sign) {};
    template<class ... Args> inline void a_call(Args ... vals);
};

template<class ... Args>
class ArgsAnonymousFunctor : public AnonymousFunctor
{
public:
    inline explicit ArgsAnonymousFunctor(std::string sn = "Undefined", size_t fsl = 0, std::list<std::string> sign = {}) : AnonymousFunctor(sn, fsl, sign) {};
    inline virtual void aa_call(Args ...) = 0;

    //A day we could be able to reconstruct the original GenericFunctor from it's return type and functory_model...
    static const std::tuple<Args ...> functor_model;
};

//Void is a one-way path, doing the opposite is not possible as the function might return nothing!
template<class C = void, class ... Args>
class GenericFunctor : public ArgsAnonymousFunctor<Args ...>
{
public:
    using function_t = std::function<C(Args ...)>;
    inline explicit GenericFunctor(function_t func);
    inline GenericFunctor(std::string sn, function_t func);
    inline GenericFunctor(AbstractThread *, std::string sn, function_t func);
    inline GenericFunctor(AbstractThread *, function_t func);

    inline std::string get_type() override;
    inline C call(Args ... vals);
    inline void aa_call(Args ...) override;

    inline void set_thread(AbstractThread *t);

    inline explicit operator GenericFunctor<void, Args ...> *() {
        return new GenericFunctor<void, Args ...>(SSDSet::name(), ftor);
    }

    inline operator GenericFunctor<void, Args ...>() {
        return GenericFunctor<void, Args ...>(SSDSet::name(), ftor);
    }

    inline function_t &get() {return ftor;};

protected:
    static constexpr size_t gf_fsl {sizeof ... (Args)};
    AbstractThread *thread = nullptr;

private:
    function_t ftor;
    friend class GenericExecutor<C, Args ...>;
};

template<class C>
class GenericFunctor<C> : public ArgsAnonymousFunctor<>
{
public:
    using function_t = std::function<C()>;
    inline explicit GenericFunctor(function_t func);
    inline GenericFunctor(std::string sn, function_t func);
    inline GenericFunctor(AbstractThread *, std::string sn, function_t func);
    inline GenericFunctor(AbstractThread *, function_t func);

    inline std::string get_type() override;
    inline C call();
    inline void aa_call() override;

    inline void set_thread(AbstractThread *t);

    inline explicit operator GenericFunctor<void> *() {
        return new GenericFunctor<void>(SSDSet::name(), ftor);
    }

    inline operator GenericFunctor<void>() {
        return GenericFunctor<void>(SSDSet::name(), ftor);
    }

    inline explicit operator GenericExecutor<C> *() {
        return new GenericExecutor<C>(SSDSet::name(), ftor);
    }

    inline operator GenericExecutor<C>() {
        return GenericExecutor<C>(SSDSet::name(), ftor);
    }

    inline operator AbstractExecutor *() {
        return new GenericExecutor<C>(SSDSet::name(), ftor);
    }

    inline function_t &get() {return ftor;};

protected:
    AbstractThread *thread = nullptr;

private:
    function_t ftor;
    friend class GenericExecutor<C>;
};

#ifdef SIGSOT_TRACKING

class AbstractSignalTracking;

class SignalTracker
{
public:
    //The 3 next ones handle a proper deletion. The object is statically accessible,
    //but when the app ends, the object gets deleted, if a signal try to be removed
    //from the registered signals but the object is already deleted :/
    //So accessible() tells if the object is still alive, get() is to get the static
    //instance, and the destructor is used to update the accessible state. get() might
    //return a nullptr. Notice that calling ~SignalTracker on get() will generate undef
    //behaviour as new signals can be created and try to be registered (using what has
    //been deleted)!
    ~SignalTracker();
    static SignalTracker *get();
    static bool accessible();

    inline std::list<int> get_availables() {return _available_ones;};
    inline AbstractSignalTracking *get_signal(int id) {return mapped[id];};
    inline int next_id();

    inline void enable_track_on_registered(bool enable = true) {en_trk_on_reg = enable;};
    inline void track(int id, bool enable = true);

private:
    std::list<int> _available_ones;
    std::map<int, AbstractSignalTracking *> mapped;
    bool en_trk_on_reg = false;

    std::mutex mtx;

    inline void add_sig(int id, AbstractSignalTracking *sig);
    inline void remove_sig(int id);
    friend class AbstractSignalTracking;
};

class AbstractSignalTracking : public SSDSet
{
public:
    inline explicit AbstractSignalTracking(std::string sn, size_t fls, std::list<std::string> l);
    inline ~AbstractSignalTracking();

    inline int get_id() {return allocated_id;};

protected:
    bool tracking_enabled = false;

private:
    int allocated_id = -2;
    inline static int last_id = 0;
    friend class SignalTracker;
};
#endif

//Always use pointers for the slots, for anymouses because it is an abstract class, and for the generic ftor, because, any way,
//if a signal wants to use exclsive slot with checks like privated signal (one slot at a time and need to have the old functor
//to use another slot), or a mono that is one slot at a time and any other can stil the access (faster to check a ptr than a
//whole object)
template<class C = void, class ... Args>
class GenericSignal :
#ifdef SIGSOT_TRACKING
                     public AbstractSignalTracking
#else
                     public SSDSet
#endif
{
public:
    using function_t = std::function<C(Args ...)>;
    inline explicit GenericSignal(std::string sn = "Undefined") :
#ifdef SIGSOT_TRACKING
    AbstractSignalTracking(sn, gs_fsl, {typeid(Args).name() ...})
#else
    SSDSet(sn, gs_fsl, {typeid(Args).name() ...})
#endif
    {};

    inline virtual ~GenericSignal() {
#ifdef SIGSOT_TRACKING
        AbstractSignalTracking::~AbstractSignalTracking();
#endif
    }

    inline virtual void emit(Args ... vals);

    inline virtual void connect(GenericFunctor<C, Args ...> *) {};
    inline virtual void connect(function_t f) {connect(new GenericFunctor<C, Args ...>(f));};
    inline virtual void connect(function_t a, function_t b) {connect(a); connect(b);};
    inline virtual void connect(GenericFunctor<C, Args ...> *a, GenericFunctor<C, Args ...> *b) {connect(a); connect(b);};

    inline virtual void disconnect(GenericFunctor<C, Args ...> *) {};
    inline virtual void disconnect(function_t f) {disconnect(new GenericFunctor<C, Args ...>(f));};
    inline virtual void disconnect(function_t a, function_t b) {disconnect(a); disconnect(b);};
    inline virtual void disconnect(GenericFunctor<C, Args ...> *a, GenericFunctor<C, Args ...> *b) {disconnect(a); disconnect(b);};

    //As in a signal returns mean nothing, we can pass other return types too by using anonymouses! Handle them as separated, they are not GenericFunctor deductible.
    //But that is clearly not a reason to fully switch to anonymouses, while we can use the original GenericFunctor because it use less runtime checks.
    inline virtual void connect(AnonymousFunctor *) {};
    inline virtual void connect(AnonymousFunctor *a, AnonymousFunctor *b) {connect(a); connect(b);};
    inline virtual void disconnect(AnonymousFunctor *) {};
    inline virtual void disconnect(AnonymousFunctor *a, AnonymousFunctor *b) {disconnect(a); disconnect(b);};

protected:
    static constexpr size_t gs_fsl {sizeof ... (Args)};
    std::mutex mtx;

    inline bool tracked() {
#ifdef SIGSOT_TRACKING
        return tracking_enabled;
#else
        return false;
#endif
    }
};

template<class C>
class GenericSignal<C> :
#ifdef SIGSOT_TRACKING
                         public AbstractSignalTracking
#else
                         public SSDSet
#endif
{
public:
    using function_t = std::function<C()>;
    inline explicit GenericSignal(std::string sn = "Undefined") :
#ifdef SIGSOT_TRACKING
    AbstractSignalTracking(sn, gs_fsl, {})
#else
    SSDSet(sn, gs_fsl, {})
#endif
    {};

    inline ~GenericSignal() {
#ifdef SIGSOT_TRACKING
        //AbstractSignalTracking::~AbstractSignalTracking();
#endif
    }

    inline virtual void emit();

    inline virtual void connect(GenericFunctor<C> *) {};
    inline virtual void connect(function_t f) {connect(new GenericFunctor<C>(f));};
    inline virtual void connect(function_t a, function_t b) {connect(a); connect(b);};
    inline virtual void connect(GenericFunctor<C> *a, GenericFunctor<C> *b) {connect(a); connect(b);};

    inline virtual void disconnect(GenericFunctor<C> *) {};
    inline virtual void disconnect(function_t f) {disconnect(new GenericFunctor<C>(f));};
    inline virtual void disconnect(function_t a, function_t b) {disconnect(a); disconnect(b);};
    inline virtual void disconnect(GenericFunctor<C> *a, GenericFunctor<C> *b) {disconnect(a); disconnect(b);};

    inline virtual void connect(AnonymousFunctor *) {};
    inline virtual void connect(AnonymousFunctor *a, AnonymousFunctor *b) {connect(a); connect(b);};
    inline virtual void disconnect(AnonymousFunctor *) {};
    inline virtual void disconnect(AnonymousFunctor *a, AnonymousFunctor *b) {disconnect(a); disconnect(b);};

protected:
    static constexpr size_t gs_fsl {0};
    std::mutex mtx;

    inline bool tracked() {
#ifdef SIGSOT_TRACKING
        return tracking_enabled;
#else
        return false;
#endif
    }
};

template<class C = void, class ... Args>
class SignalMulti : public GenericSignal<C, Args ...>
{
public:
    using function_t = std::function<C(Args ...)>;
    inline explicit SignalMulti(std::string sn = "Undefined");
    inline ~SignalMulti() {
#ifdef SIGSOT_TRACKING
        this->GenericSignal<C, Args ...>::~GenericSignal();
#endif
    }

    inline void emit(Args ... vals) override;

    inline void connect(GenericFunctor<C, Args ...> *f) override;
    inline void disconnect(GenericFunctor<C, Args ...> *f) override;
    inline void connect(AnonymousFunctor *f) override;
    inline void disconnect(AnonymousFunctor *f) override;

private:
    std::list<GenericFunctor<C, Args ...> *> _callbacks;
    std::list<AnonymousFunctor *> _a_callbacks;
};

template<class C>
class SignalMulti<C> : public GenericSignal<C>
{
public:
    using function_t = std::function<C()>;
    inline explicit SignalMulti(std::string sn = "Undefined");
    inline ~SignalMulti() {
#ifdef SIGSOT_TRACKING
        //this->GenericSignal<C>::~GenericSignal();
#endif
    }

    inline void emit() override;

    inline void connect(GenericFunctor<C> *f) override;
    inline void disconnect(GenericFunctor<C> *f) override;
    inline void connect(AnonymousFunctor *f) override;
    inline void disconnect(AnonymousFunctor *f) override;

private:
    std::list<GenericFunctor<C> *> _callbacks;
    std::list<AnonymousFunctor *> _a_callbacks;
};



//Now here is the source.
/******** Functor ********/

//Abstract layer
template<class ... Args> inline
void AnonymousFunctor::a_call(Args ... vals)
{
    if (ArgsAnonymousFunctor<Args ...> *c = dynamic_cast<ArgsAnonymousFunctor<Args ...> *>(this)) {
        c->aa_call(vals ...);
    } else {
        std::list<std::string> sign_a {typeid(Args).name() ...};
        std::list<std::string> sign_b = signature();
        std::cout << "Anonymous function transformation failed: invalid arguments:\n" << "[" << name() << "] [";
        for (std::string v : sign_b) {
            std::cout << v << " - ";
        }
        std::cout << "] to [";
        for (std::string v : sign_a) {
            std::cout << v << " - ";
        }
        std::cout << "]" << std::endl;
    }
}

// The specialised one
template<class C> inline
GenericFunctor<C>::GenericFunctor(std::function<C()> func) : ArgsAnonymousFunctor<>("Undefined", 0, {})
{
    ftor = func;
    return;
}

template<class C> inline
GenericFunctor<C>::GenericFunctor(std::string sn, std::function<C()> func) : ArgsAnonymousFunctor<>(sn, 0, {})
{
    ftor = func;
}

template<class C> inline
GenericFunctor<C>::GenericFunctor(AbstractThread *t, std::string sn, std::function<C()> func) : ArgsAnonymousFunctor<>(sn, 0, {})
{
    ftor = func;
    thread = t;
}

template<class C> inline
GenericFunctor<C>::GenericFunctor(AbstractThread *t, std::function<C()> func) : ArgsAnonymousFunctor<>("Undefined", 0, {})
{
    ftor = func;
    thread = t;
}

template<class C> inline
std::string GenericFunctor<C>::get_type()
{
    int status;
    const char *t = typeid(this).name();
    char *dem = abi::__cxa_demangle(t,0,0,&status);
    if (status == 0) {
        return std::string(dem);
    } else {
        return std::string(t);
    }
}

template<class C> inline
C GenericFunctor<C>::call() {
    if (thread) {
        thread->add_callback(new GenericExecutor<C>(ftor));
    } else {
        return ftor();
    }
}

template<class C> inline
void GenericFunctor<C>::aa_call() {
    call();
}

//Then generic one
template<class C, class ... Args> inline
GenericFunctor<C, Args ...>::GenericFunctor(std::function<C(Args ...)> func) : ArgsAnonymousFunctor<Args ...>("Undefined", gf_fsl, {typeid(Args).name() ...})
{
    ftor = func;
}

template<class C, class ... Args> inline
GenericFunctor<C, Args ...>::GenericFunctor(std::string sn, std::function<C(Args ...)> func) : ArgsAnonymousFunctor<Args ...>(sn, gf_fsl, {typeid(Args).name() ...})
{
    ftor = func;
}

template<class C, class ... Args> inline
GenericFunctor<C, Args ...>::GenericFunctor(AbstractThread *t, std::string sn, std::function<C(Args ...)> func) : ArgsAnonymousFunctor<Args ...>(sn, gf_fsl, {typeid(Args).name() ...})
{
    ftor = func;
    thread = t;
}

template<class C, class ... Args> inline
GenericFunctor<C, Args ...>::GenericFunctor(AbstractThread *t, std::function<C(Args ...)> func) : ArgsAnonymousFunctor<Args ...>("Undefined", gf_fsl, {typeid(Args).name() ...})
{
    ftor = func;
    thread = t;
}

template<class C, class ... Args> inline
std::string GenericFunctor<C, Args ...>::get_type()
{
    int status;
    const char *t = typeid(this).name();
    char *dem = abi::__cxa_demangle(t,0,0,&status);
    if (status == 0) {
        return std::string(dem);
    } else {
        return std::string(t);
    }
}

template<class C, class ... Args> inline
C GenericFunctor<C, Args ...>::call(Args ... vals) {
    if (thread) {
        thread->add_callback(new GenericExecutor<C, Args ...>(ftor, vals ...));
    } else {
        return ftor(vals ...);
    }
}

template<class C, class ... Args> inline
void GenericFunctor<C, Args ...>::aa_call(Args ... vals)
{
    call(vals ...);
}



/******** Executor ********/
template<class C> inline
GenericExecutor<C>::GenericExecutor(std::function<C()> func) : AbstractExecutor(), _bind(func)
{
}

template<class C> inline
GenericExecutor<C>::GenericExecutor(std::string sn, std::function<C()> func) : AbstractExecutor(sn), _bind(func)
{
}

template<class C> inline
GenericExecutor<C>::GenericExecutor(GenericFunctor<C> *src) : AbstractExecutor(src->name() + "(xtor)"), _bind(src->get())
{
}

template<class C> inline
void GenericExecutor<C>::execute()
{
    _bind();
}

template<class C> inline
C GenericExecutor<C>::run()
{
    return _bind();
}

template<class C> inline
std::string GenericExecutor<C>::get_type()
{
    int status;
    const char *t = typeid(this).name();
    char *dem = abi::__cxa_demangle(t,0,0,&status);
    if (status == 0) {
        return std::string(dem);
    } else {
        return std::string(t);
    }
}


template<class C, class ... Args> inline
GenericExecutor<C, Args ...>::GenericExecutor(std::function<C(Args ...)> func, Args ... vals) : AbstractExecutor(), _bind(func, vals ...)
{
}

template<class C, class ... Args> inline
GenericExecutor<C, Args ...>::GenericExecutor(std::string sn, std::function<C(Args ...)> func, Args ... vals) : AbstractExecutor(sn), _bind(func, vals ...)
{
}

template<class C, class ... Args> inline
GenericExecutor<C, Args ...>::GenericExecutor(GenericFunctor<C, Args ...> *src, Args ... vals) : AbstractExecutor(src->name()), _bind(src->get(), vals ...)
{
}

template<class C, class ... Args> inline
void GenericExecutor<C, Args ...>::execute()
{
    _bind();
}

template<class C, class ... Args> inline
C GenericExecutor<C, Args ...>::run()
{
    return _bind();
}

template<class C, class ... Args> inline
std::string GenericExecutor<C, Args ...>::get_type()
{
    int status;
    const char *t = typeid(this).name();
    char *dem = abi::__cxa_demangle(t,0,0,&status);
    if (status == 0) {
        return std::string(dem);
    } else {
        return std::string(t);
    }
}



/***** Sig Tracking ******/
#ifdef SIGSOT_TRACKING
inline int SignalTracker::next_id()
{
    return AbstractSignalTracking::last_id;
}

inline void SignalTracker::add_sig(int id, AbstractSignalTracking *sig)
{
    mtx.lock();

    std::cout << "SIGSOT_TRACKING > [" << id << "] [" << ((SSDSet *)sig)->name() << "] [REGISTERED]" << std::endl;

    mapped.insert({id, sig});
    _available_ones.push_back(id);

    if (en_trk_on_reg) {
        track(id);
    }

    mtx.unlock();
}

inline void SignalTracker::remove_sig(int id)
{
    mtx.lock();
    _available_ones.remove(id);
    std::string sn = mapped[id]->name();
    mapped.erase(id);
    std::cout << "SIGSOT_TRACKING > [" << id << "] [" << sn << "] [UNREGISTERED]" << std::endl;
    mtx.unlock();
}

inline void SignalTracker::track(int id, bool enable)
{
    if (AbstractSignalTracking *t = mapped[id]) {
        t->tracking_enabled = enable;
        std::cout << "SIGSOT_TRACKING > [" << id << "] [" << ((SSDSet *)t)->name() << "] [TRACK] [" << (enable ? "ENABLED" : "DISABLED") << "]" << std::endl;
    }
}

AbstractSignalTracking::AbstractSignalTracking(std::string sn, size_t fsl, std::list<std::string> l) : SSDSet(sn, fsl, l)
{
    last_id++;
    allocated_id = last_id;
    SignalTracker::get()->add_sig(allocated_id, this);
}

inline AbstractSignalTracking::~AbstractSignalTracking()
{
    if (SignalTracker::accessible()) {
        SignalTracker::get()->remove_sig(allocated_id);
    }
}

#endif



/******** Sig Gen ********/
template<class C, class ... Args> inline
void GenericSignal<C, Args ...>::emit(Args ...)
{
#ifdef SIGSOT_TRACKING
    if (tracked()) {
        std::cout << "SIGSOT_TRACKING > [" << AbstractSignalTracking::get_id() << "] [" << name() << "] [EMITED]" << std::endl;
        std::cout << "                > SSCALL_OUTPUTS:" << std::endl;
    }
#endif
}

template<class C> inline
void GenericSignal<C>::emit()
{
#ifdef SIGSOT_TRACKING
    if (tracked()) {
        std::cout << "SIGSOT_TRACKING > [" << AbstractSignalTracking::get_id() << "] [" << name() << "] [EMITED]" << std::endl;
        std::cout << "                > SSCALL_OUTPUTS:" << std::endl;
    }
#endif
}



/******** Signals ********/
template<class C, class ... Args> inline
SignalMulti<C, Args ...>::SignalMulti(std::string sn) : GenericSignal<C, Args ...> (sn)
{
}

template<class C, class ... Args> inline
void SignalMulti<C, Args ...>::emit(Args ... vals)
{
    GenericSignal<C, Args ...>::emit(vals ...);
    for (GenericFunctor<C, Args ...> *ftor : _callbacks) {
        std::cout << "                > [CALLED] [" << ftor->name() << "] [" << ftor->get_type() << "]" << std::endl;
        ftor->call(vals ...);
    }
}

//Do not go too fast, use mutexes!
template<class C, class ... Args> inline
void SignalMulti<C, Args ...>::connect(GenericFunctor<C, Args ...> *cb)
{
    GenericSignal<C, Args ...>::mtx.lock();
    _callbacks.push_back(cb);
    GenericSignal<C, Args ...>::mtx.unlock();
}

template<class C, class ... Args> inline
void SignalMulti<C, Args ...>::disconnect(GenericFunctor<C, Args ...> *cb)
{
    GenericSignal<C, Args ...>::mtx.lock();
    _callbacks.remove(cb);
    GenericSignal<C, Args ...>::mtx.unlock();
}

template<class C, class ... Args> inline
void SignalMulti<C, Args ...>::connect(AnonymousFunctor *f)
{
    GenericSignal<C, Args ...>::mtx.lock();
    _a_callbacks.push_back(f);
    GenericSignal<C, Args ...>::mtx.unlock();
}

template<class C, class ... Args> inline
void SignalMulti<C, Args ...>::disconnect(AnonymousFunctor *f)
{
    GenericSignal<C, Args ...>::mtx.lock();
    _a_callbacks.remove(f);
    GenericSignal<C, Args ...>::mtx.unlock();
}

template<class C> inline
SignalMulti<C>::SignalMulti(std::string sn) : GenericSignal<C> (sn)
{
}

template<class C> inline
void SignalMulti<C>::emit()
{
    GenericSignal<C>::emit();
    for (GenericFunctor<C> *ftor : _callbacks) {
        std::cout << "                > [CALLED] [" << ftor->name() << "] [" << ftor->get_type() << "]" << std::endl;
        ftor->call();
    }
}

template<class C> inline
void SignalMulti<C>::connect(GenericFunctor<C> *cb)
{
    GenericSignal<C>::mtx.lock();
    _callbacks.push_back(cb);
    GenericSignal<C>::mtx.unlock();
}

template<class C> inline
void SignalMulti<C>::disconnect(GenericFunctor<C> *cb)
{
    GenericSignal<C>::mtx.lock();
    _callbacks.remove(cb);
    GenericSignal<C>::mtx.unlock();
}

template<class C> inline
void SignalMulti<C>::connect(AnonymousFunctor *f)
{
    GenericSignal<C>::mtx.lock();
    _a_callbacks.push_back(f);
    GenericSignal<C>::mtx.unlock();
}

template<class C> inline
void SignalMulti<C>::disconnect(AnonymousFunctor *f)
{
    GenericSignal<C>::mtx.lock();
    _a_callbacks.remove(f);
    GenericSignal<C>::mtx.unlock();
}
}
