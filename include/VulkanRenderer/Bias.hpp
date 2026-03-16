#pragma once

#include <memory>

namespace animation {

class Bias {
public:
  using ValueType = double;
  explicit Bias(ValueType v);
  ~Bias() = default;
  Bias(const Bias &) = default;
  Bias(Bias &&) = default;
  Bias &operator=(const Bias &) = default;
  Bias &operator=(Bias &&) = default;
  auto get() const noexcept -> ValueType;

  static auto min() noexcept -> Bias;
  static auto max() noexcept -> Bias;

private:
  ValueType m_value;
};


class TotalTicks {
public:
  using ValueType = double;
  explicit TotalTicks(ValueType v);
  ~TotalTicks() = default;
  TotalTicks(const TotalTicks &) = default;
  TotalTicks(TotalTicks &&) = default;
  TotalTicks &operator=(const TotalTicks &) = default;
  TotalTicks &operator=(TotalTicks &&) = default;
  auto get() const noexcept -> ValueType;

private:
  ValueType m_value;
};

class TicksPerSecond {
public:
  using ValueType = double;
  explicit TicksPerSecond(ValueType v);
  ~TicksPerSecond() = default;
  TicksPerSecond(const TicksPerSecond &) = default;
  TicksPerSecond(TicksPerSecond &&) = default;
  TicksPerSecond &operator=(const TicksPerSecond &) = default;
  TicksPerSecond &operator=(TicksPerSecond &&) = default;
  auto get() const noexcept -> ValueType;

private:
  ValueType m_value;
};

}    
