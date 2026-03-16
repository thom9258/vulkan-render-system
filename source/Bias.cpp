#include <VulkanRenderer/Bias.hpp>

#include <algorithm>

namespace animation {

Bias::Bias(ValueType v) : m_value{std::clamp(v, 0.0, 1.0)} {}
auto Bias::get() const noexcept -> ValueType { return m_value; }
auto Bias::min() noexcept -> Bias { return Bias(0.0f); }
auto Bias::max() noexcept -> Bias { return Bias(1.0f); }


TotalTicks::TotalTicks(ValueType v) : m_value{v} {}
auto TotalTicks::get() const noexcept -> ValueType { return m_value; }

TicksPerSecond::TicksPerSecond(ValueType v) : m_value{v} {}
auto TicksPerSecond::get() const noexcept -> ValueType { return m_value; }

}    
