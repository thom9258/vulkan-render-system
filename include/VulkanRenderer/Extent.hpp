#pragma once

#include <type_traits>
#include <utility>
#include <cstdint>

template<typename T>
requires std::is_trivial_v<T>
struct Extent
{
	using ValueType = std::remove_cvref_t<T>;
	
	constexpr Extent() = default;
	constexpr ~Extent() = default;

	template <typename U>
	requires std::convertible_to<U, ValueType>
	constexpr Extent(Extent<U> rhs)
	{
		w = rhs.w;
		h = rhs.h;
	}

	template <typename U>
	requires std::convertible_to<U, ValueType>
	constexpr Extent<ValueType> operator=(Extent<U> rhs)
	{
		w = rhs.w;
		h = rhs.h;
	}

	constexpr Extent(ValueType w, ValueType h)
		: w(w), h(h)
	{
	}
	
	constexpr Extent operator+(Extent rhs)
	{
		return Extent(w + rhs.w, h + rhs.h);
	}

	constexpr Extent& operator+=(Extent rhs)
	{
		*this = Extent(w, h) + rhs;
		return *this;
	}

	constexpr Extent operator-(Extent rhs)
	{
		return Extent(w - rhs.w, h - rhs.h);
	}

	constexpr Extent& operator-=(Extent rhs)
	{
		*this = Extent(w, h) - rhs;
		return *this;
	}
	
	constexpr Extent operator/(double rhs)
	{
		return Extent(w / rhs, h / rhs);
	}

	constexpr Extent& operator/=(double rhs)
	{
		*this = Extent(w, h) / rhs;
		return *this;
	}
	
	constexpr Extent operator*(double rhs)
	{
		return Extent(w * rhs, h * rhs);
	}

	constexpr Extent& operator*=(double rhs)
	{
		*this = Extent(w, h) * rhs;
		return *this;
	}
	
	ValueType width() const noexcept {return w; }
	ValueType height() const noexcept {return h; }
		
	ValueType w;
	ValueType h;
};
	
using U32Extent = Extent<uint32_t>;
using I32Extent = Extent<int32_t>;
