
#Config File Util

#Config File Format

```
setting1=Something
#Comment 1
setting 2 = Something Else #Hello World
group1{
	setting 3          = Some Other Thing
	group2{
		setting4 = stuf and things
		group3{setting5=other things}
	}
}
```

##ParseFile

Is a simple parser to be used in other projects. It only handles a simple config file type, does not parse the values for you, and has no error checking for you. You have to implement your own safeguards for data in the config file, especially if you are doing something like getting an int from a field (you have to check that it is an int before converting, etc)

##SaveToFile

Is a simple writer to be used in other projects. It writes the struct out to the given filename for later use of being read in.