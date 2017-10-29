#ifndef GLOP_CONFIG
#define GLOP_CONFIG

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <iterator>
#include <algorithm>

namespace GlopConfig{

	struct Settings{
		std::map<std::string,Settings> groups;
		std::map<std::string,std::string> values;

		//these are convinence functions that try parsing
		//but return the default if there is an error
		double getValueAsDouble(std::string , double def);
		int getValueAsInt(std::string , int def);
		std::string getValueAsString(std::string , std::string def);
		bool getValueAsBool(std::string , bool def);
	};

	Settings ParseFile(std::string filename);
	void SaveToFile(std::string filename,Settings& settings);


}; // namespace GlopConfig

#endif