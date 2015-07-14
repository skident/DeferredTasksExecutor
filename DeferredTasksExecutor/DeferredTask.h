#pragma once

#include <functional>

//#define __DBG__
#ifdef __DBG__
#include <iostream>
#endif


class DeferredTask
{
private:
	enum eLimits
	{
		lim_min_priority = 0,
		lim_max_priority = 100,
	};

	std::function<void()>	m_lambda;	// wrapper for function which have to invoked later
	int						m_priority;	// task priority
	int						m_uId;		// task unique ID

	static int	GenerateUniqueId();		// Generate new unique ID for task


public:
	// Template constructor with parameter pack. First argument is func and other args are parameters for func.
	template<class Fn, class... Args>
	DeferredTask(Fn fn, Args... arg)
		: m_priority(lim_min_priority)	//set default priority for task (the lowest)
		, m_uId(GenerateUniqueId())		//gen id for task
	{
		//store func to lambda func (it'll be invoked in future)
		m_lambda = [=]()
		{
			fn(arg...);
		};

		#ifdef __DBG__
		std::cout << "Constr Task id: " << m_uId << std::endl;
		#endif
	}

	~DeferredTask();						// Destructor

	void		SetPriority(int priority);	// Set task priority 
	int			GetPriority() const;		// Get task priority
	int			GetTaskId() const;			// Get task unique ID
	void		Run();						// invoke lambda function
};

