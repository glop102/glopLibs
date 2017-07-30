#include <string>
#include <iterator>
#include "glopConfig.h"

using namespace GlopConfig;

void printSettings(Settings& s,std::string prefix){
	{ // print the current level values first
		auto itt = s.values.begin();
		while(itt!=s.values.end()){
			printf("%s%s : %s\n",prefix.c_str(),itt->first.c_str(),itt->second.c_str());
			++itt;
		}
	}
	{ // recurse down into the groups
		auto itt = s.groups.begin();
		while(itt!=s.groups.end()){
			printf("%sGROUP %s\n",prefix.c_str(),itt->first.c_str());
			printSettings(itt->second,prefix+"    ");
			++itt;
		}
	}
}

int main(void){
	Settings s = ParseFile("test_settings.conf");
	printSettings(s,"");
	return 0;
}