#include "DeferredTasksExecutor.h"
#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include "DeferredTask.h"
#include <memory>

using namespace std;

//first type of tasks
void func1(int param1, int param2)
{
	int param3 = param1 + param2;
	this_thread::sleep_for(chrono::milliseconds(param3));
}

//second type of tasks
void func2()
{
	this_thread::sleep_for(chrono::milliseconds(7));
}

//third type of tasks
void func3(string str)
{
	string _str = str;
	this_thread::sleep_for(chrono::milliseconds(2));
}


int main()
{
	DeferredTasksExecutor* executor = DeferredTasksExecutor::Get();
	executor->Start();	//start to process tasks

	default_random_engine dre(50);
	uniform_int_distribution<int> id(1, 100);

	int priority	= 0;
	int taskId		= 0;

	for (int i = 0; i < 100; i++)
	{
		std::shared_ptr<DeferredTask> pTask;
		try
		{
			priority = id(dre);
			if (i % 2)
				pTask.reset(new DeferredTask(func1, i, 2));	
			else if (i % 5)
				pTask.reset(new DeferredTask(func2));
			else 
				pTask.reset(new DeferredTask(func3, "Some string"));

			pTask->SetPriority(priority);	//set random priority
			taskId = pTask->GetTaskId();
			executor->AddTask(move(pTask));		//add task to queue

			//cancel some tasks
			if (i % 7)
				executor->CancelTask(taskId); 
		}
		catch (exception& ex)
		{
			cout << "Exception caught: " << ex.what() << endl;
		}
		catch (...)
		{
			cout << "Unhandled exception caught" << endl;
		}
	}
		
	this_thread::sleep_for(chrono::milliseconds(50));
	executor->Stop();

	executor->ReleaseAllTasks();
	
	getchar();
	
	return 0;
}