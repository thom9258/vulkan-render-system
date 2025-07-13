#pragma once

#include <VulkanRenderer/StrongType.hpp>

#include <array>
#include <cstdint>

using MaxFlightFrames = StrongType<uint32_t, struct MaxFlightFramesTag>;
using CurrentFlightFrame = StrongType<uint32_t, struct CurrentFlightFrameTag>;


struct FlightFrames
{
	MaxFlightFrames static constexpr max = MaxFlightFrames{3};

	constexpr FlightFrames();

	constexpr auto current() 
		const noexcept -> CurrentFlightFrame;
	
	auto operator++()
	noexcept -> FlightFrames&;
	
private:
	CurrentFlightFrame m_current;
};

template <typename T>
using FlightFramesArray = std::array<T, *FlightFrames::max>;
