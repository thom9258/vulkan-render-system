#pragma once

#include <SDL2/SDL.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <source_location>

#include "Extent.hpp"
	
struct Logger
{
	enum class Type { Info, Warn, Error, Fatal};
	using LoggerLog = std::function<void(std::source_location, Logger::Type, std::string)>;
	LoggerLog log = [] (std::source_location, Type, std::string) {};
	
	void info(std::source_location loc, std::string msg)
	{
		log(loc, Type::Info, msg);
	}
	void warn(std::source_location loc, std::string msg)
	{
		log(loc, Type::Warn, msg);
	}
	void error(std::source_location loc, std::string msg)
	{
		log(loc, Type::Error, msg);
	}
	void fatal(std::source_location loc, std::string msg)
	{
		log(loc, Type::Fatal, msg);
	}
};

struct WindowConfig
{
	std::string name = "unnamed rendering window";
	U32Extent size{1200, 800};
	std::optional<I32Extent> position;
	std::optional<std::int32_t> fps;
};



//TODO: We need a minimal context that is only devices otherwise we get roundabout dependencies
// in presentationcontext when creating render targets that requires textures that thne needs the presentation context.
// implement context and make presentaitoncontext into a Presentor class instead.

namespace Render
{

class Context
{
public:
    explicit Context(WindowConfig const& config, Logger logger);
    ~Context();
	
	U32Extent get_window_extent() const noexcept;
	U32Extent window_resize_event_triggered() noexcept;
	void wait_until_idle() noexcept;

	class Impl;
	std::unique_ptr<Impl> impl;
};

}
