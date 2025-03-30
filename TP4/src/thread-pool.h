#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>     // for size_t
#include <functional>  // for the function template used in the schedule signature
#include <thread>      // for thread
#include <vector>      // for vector
#include <queue>
#include <mutex>
#include <condition_variable>
#include "Semaphore.h"

class ThreadPool {
 public:
  ThreadPool(size_t numThreads);

  void schedule(const std::function<void(void)>& thunk);
  void wait();

  ~ThreadPool();
  
 private:

 	struct Worker {
    	size_t id;
    	bool available;
    	std::function<void(void)> thunk;
    	std::thread wt;
    	Semaphore sem_i;
    	
  	};

  std::thread dt;                
  std::vector<Worker> workers;  
  
  std::queue<std::function<void(void)>> taskQueue;

  std::mutex taskMutex;
  std::mutex shutdown_lck; 
  std::mutex activeWorkers_lck;
  std::mutex available_lck;

  std::condition_variable_any activeWorkers_cv; 
  std::condition_variable_any taskQueue_cv;

  bool shutdown;
  size_t activeWorkers;
  Semaphore dispatcherSemaphore; 
  Semaphore workerSemaphore;
 
  void dispatcher();
  void worker(size_t workerID);

  ThreadPool(const ThreadPool& original) = delete;
  ThreadPool& operator=(const ThreadPool& rhs) = delete;
};

#endif