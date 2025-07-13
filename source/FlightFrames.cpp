#include "FlightFrames.hpp"

constexpr FlightFrames::FlightFrames()
	: m_current{0}
{
}

constexpr auto FlightFrames::current()
	const noexcept -> CurrentFlightFrame
{
	return m_current;
}

auto FlightFrames::operator++() 
	noexcept -> FlightFrames&
{
	m_current = CurrentFlightFrame{*m_current + 1};
	if (*m_current >= *FlightFrames::max) {
		CurrentFlightFrame{0};
	}

	return *this;
}
