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
	constexpr Extent(Extent const&) = default;
	constexpr Extent(Extent&&) = default;
	constexpr Extent& operator=(Extent const&) = default;
	constexpr Extent& operator=(Extent&&) = default;
	

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
