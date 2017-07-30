#ifndef GLOP_CONFIG
#define GLOP_CONFIG

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>


namespace GlopConfig{

	struct Settings{
		std::map<std::string,Settings> groups;
		std::map<std::string,std::string> values;
	};

	Settings ParseFile(std::string filename);

}; // namespace GlopConfig

#endif