#pragma once

#include <vector>
#include <deque>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class ThreadPool
{
public:
	ThreadPool(size_t threads, bool wait_on_quit = false)
		: wait_done(wait_on_quit)
	{
		for (size_t i = 0; i < threads; ++i)
			workers.emplace_back([this] {
			for (;;)
			{
				std::function<void()> task;

				{
					std::unique_lock<std::mutex> lock_group(task_group_mutex);

					wokers_condition.wait(lock_group, [&] {
						if (stop && !wait_done)
							return true;

						for (size_t loop = 0; loop < task_group.size(); ++loop)
						{
							if (current_group_iter == task_group.end())
								current_group_iter = task_group.begin();

							if (!current_group_iter->second.empty())
							{
								task = std::move(current_group_iter->second.front());
								current_group_iter->second.pop_front();
								++current_group_iter;
								return true;
							}
							else
							{
								++current_group_iter;
							}
						}

						return stop;
					});
				}

				if (task) task();
				else break;	// 退出条件一定是 (stop && !wait_done) || (stop && task_group.empty())
			}
		});
	}

	~ThreadPool()
	{
		{
			std::lock_guard<std::mutex> lock(task_group_mutex);
			stop = true;
		}
		wokers_condition.notify_all();
		for (auto &woker : workers)
			woker.join();
	}

	void init_group(uint32_t group_id)
	{
		std::lock_guard<std::mutex> lock_group(task_group_mutex);
		task_group[group_id];
	}

	template<class F, class... Args>
	auto push_front_task(uint32_t group_id, F&& f, Args&&... args)
	{
		return enqueue(group_id, true, f, args...);
	}

	template<class F, class... Args>
	auto push_back_task(uint32_t group_id, F&& f, Args&&... args)
	{
		return enqueue(group_id, false, f, args...);
	}

	void remove_group(uint32_t group_id)
	{
		std::lock_guard<std::mutex> lock_group(task_group_mutex);
		task_group.erase(group_id);
	}

private:
	template<class F, class... Args>
	auto enqueue(uint32_t group_id, bool to_front, F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
	{
		using return_type = typename std::result_of<F(Args...)>::type;

		std::lock_guard<std::mutex> lock_group(task_group_mutex);
		auto iter = task_group.find(group_id);
		if (iter == task_group.end())
			throw std::runtime_error("not found group id on ThreadPool");

		auto task = std::make_shared< std::packaged_task<return_type()> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);

		std::future<return_type> res = task->get_future();
		{
			if (stop)
				throw std::runtime_error("enqueue on stopped ThreadPool");

			if (to_front)
				iter->second.emplace_front([task]() { (*task)(); });
			else
				iter->second.emplace_back([task]() { (*task)(); });
		}
		wokers_condition.notify_one();
		return res;
	}


private:
	bool stop = false;

	// 停止时是否等待队列中的任务执行完毕
	bool wait_done = false;

	std::condition_variable wokers_condition;
	std::vector< std::thread > workers;

	std::mutex task_group_mutex;
	std::map< uint32_t, std::deque<std::function<void()>> > task_group;
	std::map< uint32_t, std::deque<std::function<void()>> >::iterator current_group_iter = task_group.begin();
};
