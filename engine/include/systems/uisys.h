#ifndef UISys_h__
#define UISys_h__

#include "System.h"
#include <functional>
#include <vector>

class UISys:
	public System
{
public:
	typedef std::function<void(void)> Task;

	std::vector<Task> tasks;
	UISys();
	~UISys();

	void addTask(Task task);

	void start() override;

	void process() override;

private:

};
#endif // UISys_h__
