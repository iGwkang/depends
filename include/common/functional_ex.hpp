//
// Created by Gwkang on 2022/6/12.
//

#pragma once

#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

template<typename Func, typename Tuple, std::size_t ... I, typename ...Args, std::enable_if_t<!std::is_member_function_pointer<Func>::value, int> = 0>
constexpr
decltype(auto) bind_front_impl(Func&& f, Tuple&& fargs, std::index_sequence<I...>, Args && ... args)
{
	return (std::forward<Func>(f))(std::get<I>(fargs)..., std::forward<decltype(args)>(args)...);
}

template<typename Func, typename Tuple, std::size_t ... I, typename ...Args, std::enable_if_t<std::is_member_function_pointer<Func>::value, int> = 0>
constexpr
decltype(auto) bind_front_impl(Func&& f, Tuple&& fargs, std::index_sequence<I...>, Args && ... args)
{
	return std::mem_fn(std::forward<Func>(f))(std::get<I>(fargs)..., std::forward<decltype(args)>(args)...);
}

template <typename Func, typename ...Args>
constexpr
decltype(auto) bind_front(Func&& f, Args && ... args)
{
	// std::forward_as_tuple 会将 args 的引用保存在 tuple中，不安全，因此用std::make_tuple将参数拷贝一份
	// 如果要保持传入参数的引用，参考 std::bind，使用 std::ref 包装
	// 不加 mutable 会调用 operator() const，加了mutable之后，调用 operator()，但lambda无法赋值给const变量
	return[f, fargs = std::make_tuple(std::forward<Args>(args)...)](auto &&...args) -> decltype(auto)
	{
		// 此时 fargs 相当于闭包类型的成员变量，调用 f 时直接传入，不能转发进f（如果转发进f，则lambda调用一次之后, fargs中参数的生命周期结束）
		// 不用 tuple_cat 将 fargs、args 打包到一起，是因为 fargs 中可能存在不允许copy的对象
		// 如果用tuple_cat打包到一起，意味着 fargs 要 forward 到新的tuple中，那么 fargs 中的参数生命周期结束
		// const_cast<Func&&>(f) 为了正确处理仿函数，与入参类型保持一直
		// 直接写 args... 会导致无法转发不能拷贝的对象，无法正确转发参数
		return bind_front_impl(const_cast<Func&&>(f), const_cast<decltype(fargs)&>(fargs), std::index_sequence_for<Args...>(), std::forward<decltype(args)>(args)...);
	};
}
