/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
#include <iostream> 
using namespace std;

ThreadPool::ThreadPool(size_t numThreads) : workers(numThreads), shutdown(false), activeWorkers(0){
    dt = std::thread([this] { dispatcher(); });

    for (size_t i = 0; i < numThreads; ++i) {
        workers[i].id = i;
        workers[i].available = true;
        workers[i].thunk = nullptr;
        workers[i].wt = std::thread([this, i]() { worker(i); });
    }
    workerSemaphore.signal(); 
}


void ThreadPool::schedule(const function<void(void)>& thunk) {
    {
        std::lock_guard<std::mutex> lock(taskMutex);
        taskQueue.push(thunk);
        ++activeWorkers;
        dispatcherSemaphore.signal(); 
    }
    return;
}


void ThreadPool::wait() {
    std::lock_guard<std::mutex> lock(taskMutex);
	
    while(!taskQueue.empty()){
    	taskQueue_cv.wait(taskMutex);
    }

    activeWorkers_lck.lock();
    while(activeWorkers){
    	activeWorkers_cv.wait(activeWorkers_lck);	
    } 
    activeWorkers_lck.unlock();   

}


ThreadPool::~ThreadPool() {
    wait();

	{
		std::lock_guard<std::mutex> lock(shutdown_lck);
		shutdown = true; 
	}

	dispatcherSemaphore.signal();
    dt.join();

    for (auto& worker : workers) {
        worker.sem_i.signal(); 
        worker.wt.join();
    }		
}


void ThreadPool::dispatcher() {
    while (true) {
        dispatcherSemaphore.wait();
        workerSemaphore.wait(); 

        taskMutex.lock();
        if (shutdown && taskQueue.empty()) { 
            taskMutex.unlock();
            break;
        }
        taskMutex.unlock();

        std::function<void(void)> task;

        {
	        std::lock_guard<std::mutex> lock(taskMutex);
	        task = taskQueue.front(); 
	        taskQueue.pop();
	        if(taskQueue.empty()){
	        	taskQueue_cv.notify_all();
	        }
	    }

		for(auto& worker : workers){
			if(worker.available){

				{	
					std::lock_guard<std::mutex> lock(available_lck);
					worker.available = false;
				}

			    worker.thunk = task; 
			    worker.sem_i.signal();

			    break;   
            }

        }

     }


}


void ThreadPool::worker(size_t workerID) {
  while (true) {
  	Worker& worker = workers[workerID];
    worker.sem_i.wait();

    available_lck.lock();
    if (shutdown && worker.available) {
        available_lck.unlock();
        break;
    }
    available_lck.unlock();

    std::function<void(void)> task;
    task = worker.thunk;
    worker.thunk = nullptr;
    task();

    {
    	std::lock_guard<std::mutex> lock(available_lck);
    	worker.available = true;
    }
   

    {
      std::lock_guard<std::mutex> lock(activeWorkers_lck);
      --activeWorkers;
    }

    if(activeWorkers == 0){
      	activeWorkers_cv.notify_all();
    }

    workerSemaphore.signal();

    }
}
