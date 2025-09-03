#pragma once

#include "glm.hpp"

namespace Render
{

auto stringify(glm::mat4 m)
	-> std::string
{
	return glm::to_string(m);
}
	
auto stringify(glm::vec3 v)
	-> std::string
{
	return glm::to_string(v);
}
	
auto stringify(glm::quat q)
	-> std::string
{
	return glm::to_string(q);
}
	
auto slurp_textfile(std::filesystem::path const path)
{
	std::ifstream fs(path.string());
	std::string content;
	fs.seekg(0, std::ios::end);   
	content.reserve(fs.tellg());
	fs.seekg(0, std::ios::beg);

	content.assign((std::istreambuf_iterator<char>(fs)),
				   std::istreambuf_iterator<char>());

	return content;
}
	



}
