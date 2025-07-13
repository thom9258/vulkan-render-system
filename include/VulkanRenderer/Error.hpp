#pragma once

#include <exception>
#include <format>

struct Exception
	: public std::exception
{
	Exception(cosnt std::string& what) : _what{what} {}
	Exception(std::string&& what) : _what{std::move(what)} {}
	Exception(const char* what) : _what{what} {}

	const char* what() const noexcept override
	{
		return _what.str();
	}

	std::string _what;
};

struct InvalidPath
	: public Exception
{
	InvalidPath(std::filesystem::path path)
		: Exception(std::format("Invalid Path: {}", path.string()))
	{
	}
};


