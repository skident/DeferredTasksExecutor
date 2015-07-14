#include "DeferredTasksExecutor.h"

#include "DeferredTask.h"
#include <chrono>
#include <thread>

#define __DBG__
#ifdef __DBG__
#include <iostream>

std::mutex mu_stream;
#endif

using namespace std;

unsigned int DeferredTasksExecutor::m_MaxThreadsCount = lim_default_max_threads;


//Comparer for tasks priority
bool Comparer::operator() (const std::shared_ptr<DeferredTask> lhs, const std::shared_ptr<DeferredTask> rhs)
{
	return (lhs->GetPriority() < rhs->GetPriority());
}


DeferredTasksExecutor* DeferredTasksExecutor::Get()
{
	static DeferredTasksExecutor inst;
	return &inst;

	/*if (!m_self)
		m_self = new DeferredTasksExecutor();
	return m_self;*/
}

//Constructor
DeferredTasksExecutor::DeferredTasksExecutor()
	: m_StartedThreadsCount(0)
	, m_bNeedStopProcess(false)
{
	//calculate max thread count on current PC
	m_MaxThreadsCount = std::thread::hardware_concurrency();
	if (m_MaxThreadsCount > lim_default_max_threads)
		m_MaxThreadsCount--; //exclude main thread

	#ifdef __DBG__
	{
		lock_guard<mutex> lg(mu_stream);
		cout << "!!!!!!Max available slots: " << m_MaxThreadsCount << "!!!!!!" << endl << endl;
	}
	#endif
}


//Destructor
DeferredTasksExecutor::~DeferredTasksExecutor()
{
	ReleaseAllTasks();
}


//Stop process and delete all unexecuted tasks
void DeferredTasksExecutor::ReleaseAllTasks()
{
	Stop();

	//remove all unexecuted tasks
	lock_guard<mutex> lg(mu_queue);
	while (!m_TasksPool.empty())
	{
		//DeferredTask* ptr = m_TasksPool.top();
		m_TasksPool.pop();
		//delete ptr;
	}

	#ifdef __DBG__
	{
		lock_guard<mutex> lg(mu_stream);
		cout << "~~~~~~~All tasks are deleted~~~~~~~" << endl << endl;
	}
	#endif
}


//Start process of executing tasks
void DeferredTasksExecutor::Start()
{
	#ifdef __DBG__
	{
		lock_guard<mutex> lg(mu_stream);
		cout << "~~~~~~~Executing tasks has been started~~~~~~~" << endl << endl;
	}
	#endif

	{
		//lock_guard<mutex> lg(mu_stop);
		m_bNeedStopProcess.store(false);
	}

	thread t(&DeferredTasksExecutor::Process, this);
	t.detach();
}


//Stop process of executing tasks
void DeferredTasksExecutor::Stop()
{
	#ifdef __DBG__
	{
		lock_guard<mutex> lg(mu_stream);
		cout << "~~~~~~~Send signal to stop executing~~~~~~~" << endl << endl;
	}
	#endif

	m_bNeedStopProcess.store(true);

	//wait until active tasks finished their work
	while (true)
	{
		if (m_StartedThreadsCount.load() == 0)
			break;

		//TODO: conditional variable
		this_thread::sleep_for(chrono::microseconds(lim_time_for_sleep_ms));
	}

	#ifdef __DBG__
	{
		lock_guard<mutex> lg(mu_stream);
		cout << "~~~~~~~Executing tasks has been stopped~~~~~~~" << endl << endl;
	}
	#endif

}


//Add task to queue
void DeferredTasksExecutor::AddTask(std::shared_ptr<DeferredTask> pTask)
{
	if (m_bNeedStopProcess.load())
		return;

	{	//set status InQueue for current task
		lock_guard<mutex> lg(mu_status);
		m_StatusMap[pTask->GetTaskId()] = stat_InQueue;
	}

	{	//add task to pool
		lock_guard<mutex> lg(mu_queue);
		m_TasksPool.push(pTask);
	}
}


//Cancel task, only for unexecuted tasks (status stat_InQueue)
DeferredTasksExecutor::eStatus DeferredTasksExecutor::CancelTask(int taskId)
{
	lock_guard<mutex> lg(mu_status);

	//find task by ID
	auto it = m_StatusMap.find(taskId);
	if (it == m_StatusMap.end())
		throw std::exception("Invalid task id");

	if (it->second == stat_InQueue)
	{
		it->second = stat_Cancelled;	//set cancelled status for task

		#ifdef __DBG__
		{
			lock_guard<mutex> lg(mu_stream);
			cout << "Task #" << taskId << " CANCELLED" << endl;
		}
		#endif
	}

	return it->second;
}


//Get task from queue and start it if we have free thread 
void DeferredTasksExecutor::Process()
{
	bool isTasksExist = false;

	while (true)
	{
		//stop executing tasks 
		if (m_bNeedStopProcess.load())
			break;
		
		{	//Do we have some tasks in queue?
			lock_guard<mutex> lg(mu_queue);
			isTasksExist = m_TasksPool.empty();
		}

		//have no tasks - sleep
		if (isTasksExist)
		{
			this_thread::sleep_for(chrono::milliseconds(lim_time_for_sleep_ms));
			continue;
		}
		else
		{
			//if we have free thread then start new task
			if (IsFreeSlot())
			{
				m_StartedThreadsCount++;
				thread t(&DeferredTasksExecutor::StartTask, this);
				t.detach();
				continue;
			}
		}
	}
}


//Get task from queue and execute it
void DeferredTasksExecutor::StartTask()
{
	std::shared_ptr<DeferredTask> pTask;
	{
		lock_guard<mutex> lg(mu_queue);
		if (!m_TasksPool.empty())
		{
			pTask = m_TasksPool.top();
			m_TasksPool.pop();
		}
	}

	//have òo task
	if (!pTask.get())
	{
		m_StartedThreadsCount--;
		return;
	}

	int taskId		= pTask->GetTaskId();
	int taskPrior	= pTask->GetPriority();
	#ifdef __DBG__
	{
		lock_guard<mutex> lg(mu_stream);
		cout << "Task #" << taskId << " (priority = " << taskPrior << ") " << " STARTED" << endl;
	}
	#endif

	//start only tasks with statuses stat_InQueue
	if (GetTaskStatus(taskId) == stat_InQueue)
	{
		SetTaskStatus(taskId, stat_Processing);	//change task status to stat_Processing
		pTask->Run();
		SetTaskStatus(taskId, stat_Done);		//change task status to stat_Done
	}

	m_StartedThreadsCount--;

	#ifdef __DBG__
	{
		lock_guard<mutex> lg(mu_stream);
		cout << "Task #" << taskId << " (priority = " << taskPrior << ") " << " FINISHED" << endl;
	}
	#endif
}


//Get task stats by ID
DeferredTasksExecutor::eStatus DeferredTasksExecutor::GetTaskStatus(int taskId) const
{
	lock_guard<mutex> lg(mu_status);
	
	auto it = m_StatusMap.find(taskId);
	if (it == m_StatusMap.end())
		throw std::exception("Invalid task id");
	return it->second;
}


//Set task status by ID
void DeferredTasksExecutor::SetTaskStatus(int taskId, DeferredTasksExecutor::eStatus status)
{
	lock_guard<mutex> lg(mu_status);

	auto it = m_StatusMap.find(taskId);
	if (it == m_StatusMap.end())
		throw std::exception("Invalid task id");
	it->second = status;
}


//Is available free resources for process new task
bool DeferredTasksExecutor::IsFreeSlot()
{
	return (m_StartedThreadsCount.load() < m_MaxThreadsCount);
}
