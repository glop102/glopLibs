#Config File Parser

Is a simple parser to be used in other projects. It only handles a simple config file type, does not parse the values for you, and has no error checking for you. You have to implement your own safeguards for data in the config file, especially if you are doing something like getting an int from a field (you have to check that it is an int before converting, etc)