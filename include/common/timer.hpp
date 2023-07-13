//
// Created by Gwkang on 2021/12/08.
//

#pragma once

#include <cstdint>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <memory>
#include <functional>
#include <queue>
#include <unordered_map>

class TimerExecutor
{
	struct TimerTask
	{
		TimerTask() = default;

		TimerTask(int64_t next_run_time, std::function<void()> &&task, uint32_t interval = 0)
			: next_run_time(next_run_time), task(std::move(task)), interval(interval)
		{

		}

		bool stoped{};

		uint32_t interval{};

		uint64_t timer_id{};

		// 下次执行的时间
		int64_t next_run_time{};

		std::function<void()> task;

		static bool timer_task_priority(const std::shared_ptr<TimerTask> & lhs, const std::shared_ptr<TimerTask> &rhs)
		{
			return lhs->next_run_time > rhs->next_run_time;
		}
	};

	using TimerTaskPtr = std::shared_ptr<TimerTask>;
public:
	TimerExecutor(size_t threads = 1)
		: thread_count(threads)
		, workers(threads)
	{
		start();
	}

	~TimerExecutor()
	{
		stop();
	}

	template<class Func, class... Args>
	uint64_t timeout(uint32_t ms, Func &&func, Args &&... args)
	{
		uint64_t id = timer_id_incr.fetch_add(1);
		auto new_task = std::make_shared<TimerTask>(now_us() + ms * 1000, std::bind(std::forward<Func>(func), std::forward<Args>(args) ...));
		new_task->timer_id = id;
		{
			std::unique_lock<std::mutex> lock(task_mutex);
			tasks.emplace(new_task);
			timers[id] = new_task;
		}
		task_condition.notify_one();
		return id;
	}

	template<class Func, class... Args>
	uint64_t interval(uint32_t ms, Func &&func, Args &&... args)
	{
		uint64_t id = timer_id_incr.fetch_add(1);
		auto new_task = std::make_shared<TimerTask>(now_us() + ms * 1000, std::bind(std::forward<Func>(func), std::forward<Args>(args) ...), ms * 1000);
		new_task->timer_id = id;
		{

			std::unique_lock<std::mutex> lock(task_mutex);
			tasks.emplace(new_task);
			timers[id] = new_task;
		}
		task_condition.notify_one();
		return id;
	}

	void remove_task(int64_t timer_id)
	{
		{
			std::unique_lock<std::mutex> lock(task_mutex);
			auto it = timers.find(timer_id);
			if (it != timers.end())
			{
				it->second->stoped = true;
				timers.erase(it);
			}
		}
		task_condition.notify_one();
	}

	void set_interval(int64_t timer_id, uint32_t ms)
	{
		{
			std::unique_lock<std::mutex> lock(task_mutex);
			auto it = timers.find(timer_id);
			if (it != timers.end())
			{
				it->second->interval = ms * 1000;
			}
		}
		task_condition.notify_one();
	}

	void start()
	{
		if (!active.exchange(true))
		{
			for (size_t i = 0; i < thread_count; ++i)
			{
				workers[i] = std::thread(&TimerExecutor::internal_thread, this);
			}
		}

	}

	void stop()
	{
		if (active.exchange(false))
		{
			task_condition.notify_all();
			for (auto & worker : workers)
				if (worker.joinable())
					worker.join();
		}
	}

private:
	void internal_thread()
	{
		for (;;)
		{
			std::shared_ptr<TimerTask> task;
			bool trigger_task = false;
			{
				std::unique_lock<std::mutex> lock(task_mutex);

				if (tasks.empty())
					task_condition.wait(lock, [&] { return !active || !tasks.empty(); });

				if (!active)
					break;

				while (!tasks.empty() && tasks.top()->stoped)
				{
					tasks.pop();
				}

				if (tasks.empty())
					continue;

				int64_t wait_time = std::max<int64_t>(now_us() - tasks.top()->next_run_time, 0);
				trigger_task = task_condition.wait_for(lock, std::chrono::microseconds(wait_time), [&] {
					if (!active || tasks.empty())
						return true;

					const std::shared_ptr<TimerTask> &first_task = tasks.top();
					if (now_us() >= first_task->next_run_time)
					{
						task = first_task;
						tasks.pop();
						return true;
					}
					return false;
				});

				if (!active)
					break;
			}
			if (trigger_task)
			{
				task->task();
				if (task->interval > 0)
				{
					task->next_run_time = now_us() + task->interval;
					std::unique_lock<std::mutex> lock(task_mutex);
					tasks.emplace(std::move(task));
				}
				else
				{
					std::unique_lock<std::mutex> lock(task_mutex);
					timers.erase(task->timer_id);
				}
			}
		}
	}

	static int64_t now_us()
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}

private:

	std::atomic_bool active{ false };
	const size_t thread_count;

	std::atomic_uint64_t timer_id_incr{ 1 };

	std::condition_variable task_condition;
	std::mutex task_mutex;
	std::priority_queue<TimerTaskPtr, std::vector<TimerTaskPtr>, decltype(&TimerTask::timer_task_priority)> tasks{ &TimerTask::timer_task_priority };
	std::unordered_map<uint64_t, TimerTaskPtr> timers;

	std::vector<std::thread> workers;
};

class Timer
{
	static TimerExecutor s_timer_executor;
public:
	Timer(bool repeat = false, TimerExecutor &timer_executor = s_timer_executor)
		: timer_executor_(timer_executor)
		, repeat_(repeat)
	{

	}

	~Timer()
	{
		stop();
	}

	template<class Func, class... Args>
	void start(uint32_t ms, Func &&func, Args &&... args)
	{
		if (running_->exchange(true))
			return;

		interval_ = ms;
		task_ = std::bind(std::forward<Func>(func), std::forward<Args>(args) ...);
		if (repeat_)
			timer_id_ = timer_executor_.interval(ms, task_);
		else
			timer_id_ = timer_executor_.timeout(ms, [task = task_, r = running_] {
			if (*r)
			{
				task();
				*r = false;
			}
		});
	}

	void stop()
	{
		if (!running_->exchange(false))
			return;

		timer_executor_.remove_task(timer_id_);
	}

	void restart()
	{
		if (running_->exchange(true))
			return;

		if (repeat_)
			timer_id_ = timer_executor_.interval(interval_, task_);
		else
			timer_id_ = timer_executor_.timeout(interval_, [task = task_, r = running_] {
			if (*r)
			{
				task();
				*r = false;
			}
		});
	}

	void set_interval(uint32_t ms)
	{
		if (ms == 0 || interval_ == ms || !running_ || !repeat_)
			return;

		interval_ = ms;
		timer_executor_.set_interval(timer_id_, interval_);
	}

private:
	TimerExecutor & timer_executor_;
	uint64_t timer_id_{};
	bool repeat_{};
	uint32_t interval_{};
	std::shared_ptr <std::atomic_bool> running_{ std::make_shared<std::atomic_bool>(false) };
	std::function<void()> task_;
};

TimerExecutor Timer::s_timer_executor;
