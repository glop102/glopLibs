
# Config File Util

I wanted a simple to add config file system for my programs. This is a set of two functions that parse a file or save to a file. It expects the format of the config to be as shown in the example below. It is fairly tolerant of the whitespace in the file, as shown with the silly example of group3. It stores all the values in a recursive set of structs, each struct comprised of "groups" and "values". To keep it simple, everything is stored in strings.

## Config File Format

```
setting1=Something
#Comment 1
setting 2 = Something Else #Comment 2
group1{
	setting 3          = Some Other Thing
	group2{
		setting4 = stuf and things
		group3{setting5=other things}
	}
}
```

## ParseFile

Is a simple parser to be used in other projects. It only handles the simple config file type above, does not parse the values for you (everthing is saved as a string), and has no error checking for you. You have to implement your own safeguards for data in the config file if you are doing something like getting an int from a field (you have to check that it is an int before converting, etc).

```c++
#include <string>
#include "glopConfig.h"

using namespace GlopConfig;

int main(void){
	Settings s = ParseFile("test_settings.conf");
	printf("%s\n",s.values["setting 2"].c_str());
	printf("%s\n",s.groups["group1"].values["setting 3"].c_str());
	return 0;
}
```

## SaveToFile

Is a simple writer to be used in other projects. It writes the struct out to the given filename for later use. All spacing is automaticly done, using 4 spaces for indentation.

```c++
#include <string>
#include "glopConfig.h"

using namespace GlopConfig;

int main(void){
	Settings s;
	s.values["A parameter"] = "a value";
	s.groups["a group"].values["of some paramerters"] = "more values";
	SaveToFile("test_settings_written.conf",s);
	return 0;
}
```

## A Few Minor Examples

### Simple Traversal and Printing

```c++
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
```