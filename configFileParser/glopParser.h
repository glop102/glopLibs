#ifndef GLOP_CONFIG_PARSER
#define GLOP_CONFIG_PARSER

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>


namespace ConfigParser{

	struct Settings{
		std::map<std::string,Settings> groups;
		std::map<std::string,std::string> values;
	};

	Settings ParseFile(std::string filename);

}; // namespace ConfigParser

#endif