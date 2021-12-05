//
// defer.hpp
// ~~~~~~~~~
// Copyright (c) 2021 Gwkang All rights reserved.
//

#ifndef DEFER_HPP
#define DEFER_HPP

#include <list>
#include <functional>

#define STRINGCAT_HELPER(x, y)  x ## y
#define STRINGCAT(x, y)  STRINGCAT_HELPER(x, y)

#define init_defer_var_name __defer_stack__

#define defer_stack_type DeferStack
#define defer_member_func add_defer
#define defer_exec_func exec
#define defer_helper_func DeferStack::defer_helper
#define defer_exec_helper_func DeferStack::exec_helper

//#ifdef return
//#error"return is defined!"
//#undef return
//#endif

//#define return if (defer_exec_helper_func(init_defer_var_name), true) return

#define defer_var_name STRINGCAT(__defer__, __LINE__)
#define defer_helper_name STRINGCAT(__helper__, __LINE__)

#define defer_define_helper(var_name) \
        defer_stack_type var_name; \
        defer_helper_func(var_name, init_defer_var_name)


// 定义局部 DeferStack 对象
#define init_defer_func_stack() defer_stack_type init_defer_var_name

// 表达式（如果没使用init_defer_func_stack，则引用捕获，若使用了init_defer_func_stack，则值捕获）
//#ifdef return
//#define defer_expr(x) defer_expr_ref(x)
//#else
//#define defer_expr(x) \
//        defer_stack_type defer_var_name; \
//        defer_stack_type &defer_helper_name = defer_helper_func(defer_var_name, init_defer_var_name); \
//        if (&defer_helper_name == &defer_var_name) defer_helper_name+[&]{x;}; \
//        else defer_helper_name+[=]()mutable{x;};
//#endif // return

// 表达式（引用捕获）
#define defer_expr_ref(x) defer_define_helper(defer_var_name)+[&]{x;}

// 表达式（值捕获）
#define defer_expr_val(x) defer_define_helper(defer_var_name)+[=]()mutable{x;}

// 函数
#define defer_func(func, ...) defer_define_helper(defer_var_name).defer_member_func(func, ##__VA_ARGS__)

// lambda
#define defer_lambda defer_define_helper(defer_var_name)+


// 不使用，稻草人标识符
enum { init_defer_var_name };

class DeferStack
{
	std::list<std::function<void()>> defer_func_stack;
public:
	~DeferStack() { exec(); }

	void exec()
	{
		while (!defer_func_stack.empty())
		{
			defer_func_stack.front()();
			defer_func_stack.pop_front();
		}
	}

	template<typename Function, typename ... Args>
	inline void add_defer(Function && f, Args && ... args)
	{
		defer_func_stack.emplace_front(std::bind(f, std::forward<Args>(args)...));
	}

	template<typename Function>
	inline void operator+(Function && f)
	{
		defer_func_stack.emplace_front(f);
	}

	static inline DeferStack &defer_helper(DeferStack &, DeferStack &local_defer)
	{
		return local_defer;
	}

	static inline DeferStack &defer_helper(DeferStack &new_defer, const decltype(::init_defer_var_name) &)
	{
		return new_defer;
	}

	//static inline void exec_helper(DeferStack &local_defer)
	//{
	//	local_defer.exec();
	//}

	// constexpr 处理 常量表达式函数 编译错误
	// static constexpr void exec_helper(const decltype(::init_defer_var_name) &) { }
};

#endif // DEFER_HPP
