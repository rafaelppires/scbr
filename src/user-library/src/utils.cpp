#include <utils.h>

//------------------------------------------------------------------------------
void Stats::begin() {
    start_ = high_resolution_clock::now();
}

//------------------------------------------------------------------------------
double Stats::end() {
    high_resolution_clock::time_point now = high_resolution_clock::now();
    double ret = boost::chrono::duration_cast<nanoseconds>(now - start_).count()
                                                                     / divisor_;
    accum_( ret );
    return ret;
}

//------------------------------------------------------------------------------
void Stats::print_summary( const std::string &title ) {
    //if( title.size() ) std::cout << title << "\n";
    //std::cout << "Samples: " << boost::accumulators::count(accum_) << "\n"
    //    << "Mean: " << mean(accum_) << "\n"
    //    << "Std. dev: " << sqrt(variance(accum_)) << "\n"
    //    << "Sum: " << sum(accum_) << "\n\n";
    std::cout << boost::accumulators::count(accum_) << "\t"
              << mean(accum_) << "\t"
              << sum(accum_) << "\t";
}

//------------------------------------------------------------------------------
unsigned Stats::count() {
    return boost::accumulators::count(accum_);
}

//------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

static long perf_event_open( struct perf_event_attr *hw_event, pid_t pid,
                             int cpu, int group_cachemiss_fd, unsigned long flags ) {
   int ret;
   ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_cachemiss_fd, flags);
   return ret;
}

void Logger::init( std::string outfname ) {
    if( log_.is_open() ) log_.close();
    log_.open( outfname.c_str() );
    if( !log_.good() ) {
        fprintf(stderr, "Unable to open logs\n");
        exit( EXIT_FAILURE );
    }

    struct perf_event_attr pe1, pe2;
    memset(&pe1, 0, sizeof(struct perf_event_attr));
    memset(&pe2, 0, sizeof(struct perf_event_attr));

    pe1.type = PERF_TYPE_HARDWARE;
    pe1.size = sizeof(struct perf_event_attr);
    pe1.config = PERF_COUNT_HW_CACHE_MISSES;
    pe1.disabled = 1;
    pe1.exclude_kernel = 1;
    pe1.exclude_hv = 1;

    cachemiss_fd = perf_event_open(&pe1, 0, -1, -1, 8);
    if (cachemiss_fd == -1) {
        fprintf(stderr, "Error opening (Cache misses profiler). Ignoring...\n");
    } else {
        pe2.type = PERF_TYPE_HARDWARE;
        pe2.size = sizeof(struct perf_event_attr);
        pe2.config = PERF_COUNT_HW_CACHE_REFERENCES;
        pe2.disabled = 1;
        pe2.exclude_kernel = 1;
        pe2.exclude_hv = 1;

        cacheref_fd = perf_event_open(&pe2, 0, -1, -1, 8);
        if( cacheref_fd == -1 ) {
            fprintf(stderr, "2 Error opening (Cache references profiler)\n");
            exit(EXIT_FAILURE);
        }
    }
}

//------------------------------------------------------------------------------
void Logger::close() {
    ::close(cachemiss_fd);
    ::close(cacheref_fd);
    log_.close();
}

//------------------------------------------------------------------------------
void Logger::start() {
    if( cachemiss_fd != -1 ) {
        ioctl(cachemiss_fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(cachemiss_fd, PERF_EVENT_IOC_ENABLE, 0);
    }
    ioctl(cacheref_fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(cacheref_fd, PERF_EVENT_IOC_ENABLE, 0);

    timing_.begin();
}

//------------------------------------------------------------------------------
#include <sys/resource.h>
void Logger::finish() {
    long long misscount, refcount;
    double time = timing_.end();
    ioctl(cacheref_fd, PERF_EVENT_IOC_DISABLE, 0);

    size_t r1 = -1,
           r2 = read(cacheref_fd, &refcount, sizeof(long long));

    if( cachemiss_fd != -1 ) {
        ioctl(cachemiss_fd, PERF_EVENT_IOC_DISABLE, 0);
        r1 = read(cachemiss_fd, &misscount, sizeof(long long));
    }

    if( r1 == -1 || r2 == -1 ) {
       fprintf(stderr, "Couldn't read counters %lu %lu\n", r1, r2);
    }

    std::ifstream /*stat("/proc/self/stat"),*/ statm("/proc/self/statm");
    std::string ss, sm;
    //std::getline( stat, ss );
    std::getline( statm, sm );

    /*int pid, ppid, pgrp, session, tty_nr, tpgid, exit_signal, processor;
    std::string comm;
    char state;
    unsigned flags, rt_priority, policy;
    long unsigned minflt, cminflt, majflt, cmajflt, utime, stime, vsize, rsslim,
                  startcode, endcode, startstack, kstkesp, kstkeip, signal,
                  blocked, sigignore, sigcatch, wchan, nswap, cnswap,
                  guest_time, start_data, end_data, start_brk, arg_start,
                  arg_end, env_start, env_end, exit_code;
    long int cutime, cstime, priority, nice, num_threads, itrealvalue, rss,
             cguest_time;
    long long unsigned starttime, delayacct_blkio_ticks;*/

    std::stringstream /*general,*/ memory;
    //general.str( ss );
    memory.str( sm );

    /*general >> pid      >> comm     >> state    >> ppid     >> pgrp
            >> session  >> tty_nr   >> tpgid    >> flags    >> minflt

            >> cminflt  >> majflt   >> cmajflt  >> utime    >> stime
            >> cutime   >> cstime   >> priority >> nice     >> num_threads

            >> itrealvalue  >> starttime >> vsize >> rss    >> rsslim
            >> startcode >> endcode >> startstack >> kstkesp >> kstkeip

            >> signal   >> blocked  >> sigignore >> sigcatch >> wchan
            >> nswap    >> cnswap   >> exit_signal >> processor >> rt_priority

            >> policy >> delayacct_blkio_ticks >> guest_time >> cguest_time
            >> start_data >> end_data >> start_brk >> arg_start >> arg_end
            >> env_start

            >> env_end  >> exit_code;*/

    unsigned size, resident, share, text, lib, data, dt;
    memory  >> size >> resident >> share >> text >> lib >> data >> dt;


    if( log_.good() ) {
        /*printf(".");
        std::ifstream status("/proc/self/status"), smaps("/proc/self/smaps"); std::string l,m;
        for(int i = 0; i < 22; ++i) std::getline(status,l);
        for(int i = 0; i < 2; ++i) std::getline(smaps,m);*/
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
 

        log_ << time << "\t" << misscount << "\t" << refcount << "\t"
             << (data*4/1024.) << "\t" << usage.ru_minflt << "\t" << usage.ru_majflt << "\n";
    } 
}

//------------------------------------------------------------------------------

