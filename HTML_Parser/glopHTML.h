#ifndef GLOP_HTML
#define GLOP_HTML

#include <string>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <utility>
#include <stdio.h>

using std::string;
using std::vector;
using std::pair;
using std::fstream;
using std::map;

struct TAG{
	string name; // eg img or a
	vector<string> content; // eg the text inside a <p></p> or <a></a> but split for contained tags, like an image halfway inside a <p></p>
	map<string,string> params; // like src in <img src=''/>
	string classname;
	string id;
	TAG* parent;
	vector<TAG*> children; //array of children tags
};
class GlopHTML_consumer;

class GlopHTML{
public:
	GlopHTML();

	void parseHTML(string html);
	void parseFile(string filename);
	void parseFile(FILE* fd);
	void parseFile(fstream stream);

	~GlopHTML();
	void clear();

	/* information about the tree */
	int numberOfClass(string classname);
	bool haveTagId(string id);

	/* information of the current node */
	string getCurrentName();
	vector<string> getCurrentContent();
	map<string,string> getCurrentParams();
	string getCurrentClassname();
	string getCurrentId();
	/* changing the current node */
	void setCurrentName(string name);
	void setCurrentContent(vector<string>);
	void setCurrentParam(string key,string value);
	void setCurrentClassname(string classname); // change the current node to be of classname
	void setCurrentId(string id); // change the id of the current node and boot out another node if it has the same id
	void removeCurrentParam(string key);
	void deleteCurrentTag();

	/* working with child tags */
	int getNumberChildren();
	void addChildTag(string name);
	void addChildTag(string name,int index);
	string getChildName(int index);
	string getChildClassname(int index);
	string getChildId(int index);
	void deleteChildTag(int index);

	/* Navigation of the tree */
	void moveToHead(); // reset to the begining
	void moveToFirstClass(std::string classname);
	void moveToNextClass(std::string classname);
	void moveToId(std::string id);
	void moveToCurrentChild(int childNumber); //index value of the child in the current tag
	void moveToParent(); // move to the parent of the current node

protected:
	void deleteCurrentTag(TAG*);
	
	map<string,TAG*> ids; // pointer to tags via their id
	map<string,vector<TAG*> > classes; // pointer to tags via their classname

	TAG* headNode;
	TAG* currentNode;
};

//Helper class for the main html class
//it is meant to eat one char at a time
//and then slowly fill out the tree
class GlopHTML_consumer{
public:
	TAG *head,*curt;
	map<string,TAG*> ids; // pointer to tags via their id
	map<string,vector<TAG*> > classes; // pointer to tags via their classname
	string html;
	int position;
	string getNextTagToken();
	TAG* parseTagToken(string);
	string parseForContentUntilNextTag();
	bool parseHTML_recursive();
	
	GlopHTML_consumer();
	void parseHTML(string);
	TAG* getHead();
	map<string,TAG*> getIds();
	map<string,vector<TAG*> > getClasses();
};

#endif