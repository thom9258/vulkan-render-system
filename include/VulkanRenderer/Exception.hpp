#pragma once

#include <exception>
#include <format>
#include <filesystem>
#include <string>

struct Exception : public std::exception
{
	Exception(const char* what) : _what(what) {}
	Exception(std::string const& what) : _what(what) {}
	Exception(std::string&& what) : _what(std::move(what)) {}

	virtual const char* what() const noexcept override
	{
		return _what.c_str();
	}

	std::string _what;
};

struct InvalidPath : public Exception
{
	InvalidPath(std::filesystem::path path)
		: Exception(std::format("Invalid Path: {}", path.string()))
		, path{path}
	{}

	std::filesystem::path path;
};
