#include "DeferredTask.h"


//Destructor
DeferredTask::~DeferredTask()
{
	#ifdef __DBG__
	std::cout << "Destr Task id: " << m_uId << std::endl;
	#endif
}

//Set task priority (0..100)
void DeferredTask::SetPriority(int priority)
{
	if (priority < lim_min_priority)
		priority = lim_min_priority;
	else if (priority > lim_max_priority)
		priority = lim_max_priority;

	m_priority = priority;
}

//Invoke stored function
void DeferredTask::Run()
{
	m_lambda();
}

//Get task priority
int DeferredTask::GetPriority() const
{
	return m_priority;
}

//Get task ID
int DeferredTask::GetTaskId() const
{
	return m_uId;
}

//Generate unique task ID
int DeferredTask::GenerateUniqueId()
{
	static int uid = 0;
	return ++uid;
}


