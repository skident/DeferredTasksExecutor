#pragma once

#include <queue>
#include <map>
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>

class DeferredTask;

//Task priority comparator
class Comparer
{
public:
	bool operator() (const std::shared_ptr<DeferredTask> lhs, const std::shared_ptr<DeferredTask> rhs);
};


class DeferredTasksExecutor
{
public:
	// Possible statuses for tasks
	enum eStatus
	{
		stat_InQueue,		// Task added to queue and waits its turn
		stat_Processing,	// Task already started 
		stat_Cancelled,		// Task cancelled (only for tasks with status stat_InQueue)
		stat_Done,			// Task successfully done
	};

	enum eLimits
	{
		lim_default_max_threads = 2,
		lim_time_for_sleep_ms	= 5,
	};


	static DeferredTasksExecutor*	Get();


	void							Start();							// Start executing tasks
	void							Stop();								// Wait until all active tasks finishes their work
	void							AddTask(std::shared_ptr<DeferredTask> spTask);		// Add task to queue
	void							ReleaseAllTasks();
	eStatus							CancelTask(int taskId);				// Try to cancel task
	eStatus							GetTaskStatus(int taskId) const;	// Return task status by its ID


private:
	mutable std::mutex				mu_queue;							// mutex for work with queue (add, get, delete elements)
	mutable std::mutex				mu_status;							// mutex for get/set status of task

	static unsigned int				m_MaxThreadsCount;					// Max count of threads on current PC
	std::atomic<unsigned int>		m_StartedThreadsCount;				// Count of active threads (tasks)
	std::atomic<bool>				m_bNeedStopProcess;					// Flag which signalizes that we need stop work

	std::map<int, eStatus>			m_StatusMap;						// Container which contains status for every task  (even task already deleted)
	std::priority_queue<std::shared_ptr<DeferredTask>,
						std::vector<std::shared_ptr<DeferredTask>>,
						Comparer> 
									m_TasksPool;						// Container for tasks


	DeferredTasksExecutor();
	virtual ~DeferredTasksExecutor();


	void							SetTaskStatus(int taskId, eStatus status);	
	bool							IsFreeSlot();						// Do we have free slot for start new thread
	void							StartTask();						// Try to start next task
	void							Process();							// Start tasks processing in new thread
};
