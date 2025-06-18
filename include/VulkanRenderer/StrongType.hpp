#pragma once

#include <type_traits>
#include <utility>


template<typename T, typename Tag>
requires std::copyable<T>
struct StrongType
{
	using ValueType = std::remove_cvref_t<T>;
	
	constexpr StrongType() = delete;
	constexpr ~StrongType() = default;
	constexpr StrongType(StrongType const&) = default;
	constexpr StrongType(StrongType&&) = default;
	constexpr StrongType& operator=(StrongType const&) = default;
	constexpr StrongType& operator=(StrongType&&) = default;
	

	template <typename U>
	explicit constexpr StrongType(U const& u)
		: value(u)
	{
	}

	template <typename U>
	requires std::convertible_to<U, ValueType>
	explicit constexpr StrongType(U&& u)
		: value(std::forward<ValueType>(u))
	{
	}
	
	template <typename... Args>
	//requires !std::is_trivial_v<ValueType>
	explicit constexpr StrongType(std::in_place_t, Args&&... args)
		: value(std::forward<Args>(args)...)
	{
	}
	
	template <typename... Args>
//	requires !std::is_trivial_v<ValueType>
	static constexpr auto create(Args&&... args)
		-> StrongType
	{
		return StrongType(std::in_place, std::forward<Args>(args)...);
	}
	
	constexpr auto get()
		noexcept -> ValueType&
	{
		return value;
	}

	constexpr auto get()
		const noexcept -> ValueType const&
	{
		return value;
	}

	constexpr auto ptr()
		const noexcept -> ValueType*
	{
		return &value;
	}
	
	constexpr auto operator*()
		noexcept -> ValueType&
	{
		return value;
	}
	
	constexpr auto operator*()
		const noexcept -> ValueType const&
	{
		return value;
	}



	ValueType value;
};
