#include "glopHTML.h"

GlopHTML::GlopHTML(){
	headNode = NULL;
	currentNode = NULL;
}
GlopHTML::~GlopHTML(){
	clear();
}
void GlopHTML::clear(){
	deleteCurrentTag(headNode);
	headNode = NULL;
	currentNode = NULL;
}

void GlopHTML::parseFile(std::string filename){
	FILE* fd = fopen(filename.c_str(),"r");
	parseFile(fd);
}
void GlopHTML::parseFile(FILE* fd){
	if(fd == NULL){
		headNode = NULL;
		currentNode = NULL;
		return;
	}
	string html;
	char buf[1000];

	while(!feof(fd)){
		fgets(buf,1000,fd);
		html+=buf;
	}

	parseHTML(html);
}
void GlopHTML::parseHTML(string html){
	GlopHTML_consumer con;
	con.parseHTML(html);
	headNode = con.getHead();
	currentNode = headNode;
	ids=con.getIds();
	classes=con.getClasses();
}

/******************************/
/* information about the tree */
/******************************/
int GlopHTML::numberOfClass(string classname){
	if(classes.count(classname)==0) return 0;
	return classes[classname].size();
}
bool GlopHTML::haveTagId(string id){
	return ids.count(id);
}

/***********************************/
/* information of the current node */
/***********************************/
string GlopHTML::getCurrentName(){
	if(currentNode==NULL)return "";
	return currentNode->name;
}
vector<string> GlopHTML::getCurrentContent(){
	if(currentNode==NULL)return vector<string>();
	return currentNode->content;
}
map<string,string> GlopHTML::getCurrentParams(){
	if(currentNode==NULL)return map<string,string>();
	return currentNode->params;
}
string GlopHTML::getCurrentClassname(){
	if(currentNode==NULL)return "";
	return currentNode->classname;
}
string GlopHTML::getCurrentId(){
	if(currentNode==NULL)return "";
	return currentNode->id;
}

/*****************************/
/* changing the current node */
/*****************************/
void GlopHTML::setCurrentName(string name){
	if(currentNode==NULL)return;
	currentNode->name = name;
}
void GlopHTML::setCurrentContent(vector<string> content){
	if(currentNode==NULL)return;
	currentNode->content = content;
}
void GlopHTML::setCurrentParam(string key,string value){
	if(currentNode==NULL)return;
	currentNode->params[key]=value;
}
void GlopHTML::setCurrentClassname(string classname){
	if(currentNode==NULL)return;
	if(currentNode->classname != ""){
		//lets remove ourselves if we have a class
		std::vector<TAG*> &list = classes[currentNode->classname];
		for(int x=0; x<list.size(); x++){
			if(list.at(x)==currentNode){
				list.erase(list.begin()+x);
				break;
			}
		}
	}
	currentNode->classname=classname;
	classes[classname].push_back(currentNode);
}
void GlopHTML::setCurrentId(string id){
	if(currentNode==NULL)return;
	// change the id of the current node and boot out another node if it has the same id
	if(ids.count(id)){
		ids[id]->id=""; //make the current node with the id not have one
	}
	currentNode->id = id;
	ids[id]=currentNode;
}
void GlopHTML::removeCurrentParam(string key){
	if(currentNode==NULL)return;
	currentNode->params.erase(key);
}
void GlopHTML::deleteCurrentTag(){
	if(currentNode==NULL)return;
	TAG* next = currentNode->parent;
	deleteCurrentTag(currentNode);
	currentNode=next;
}
void GlopHTML::deleteCurrentTag(TAG* tag){
	if(tag==NULL)return;
	//recursivly remove the children first
	while(tag->children.size() > 0)
		deleteCurrentTag(tag->children[0]);

	//find ourselves in our parents list and remove ourselves
	if(tag->parent != NULL){
		for(int x=0; x<tag->parent->children.size(); x++){
			if(tag->parent->children[x]==tag){
				tag->parent->children.erase(tag->parent->children.begin()+x);
				break;
			}
		}
	}
		

	//lets remove ourselves if we have an id
	if(tag->id!=""){
		ids.erase(tag->id);
	}

	//lets remove ourselves if we have a class
	if(tag->classname != ""){
		std::vector<TAG*> &list = classes[tag->classname];
		for(int x=0; x<list.size(); x++){
			if(list.at(x)==tag){
				list.erase(list.begin()+x);
				break;
			}
		}
	}

	//and now that we did the cleanup, we can remove ourselves
	delete tag;
}

/***************************/
/* working with child tags */
/***************************/
int GlopHTML::getNumberChildren(){
	if(currentNode==NULL)return 0;
	return currentNode->children.size();
}
void GlopHTML::addChildTag(string name){
	if(currentNode==NULL)return;
	TAG *temp = new TAG;
	temp->name = name;
	currentNode->children.push_back(temp);
}
void GlopHTML::addChildTag(string name,int index){
	if(currentNode==NULL)return;
	TAG *temp = new TAG;
	temp->name = name;
	currentNode->children.insert(currentNode->children.begin()+index,temp);
}
string GlopHTML::getChildName(int index){
	if(currentNode==NULL)return "";
	return currentNode->children[index]->name;
}
string GlopHTML::getChildClassname(int index){
	if(currentNode==NULL)return "";
	return currentNode->children[index]->classname;
}
string GlopHTML::getChildId(int index){
	if(currentNode==NULL)return "";
	return currentNode->children[index]->id;
}
void GlopHTML::deleteChildTag(int index){
	if(currentNode==NULL)return;
	deleteCurrentTag(currentNode->children[index]);
}

/**************************/
/* Navigation of the tree */
/**************************/
void GlopHTML::moveToHead(){
	currentNode = headNode;
}
void GlopHTML::moveToFirstClass(std::string classname){
	if(classes.count(classname) == 0) return;
	currentNode = classes[classname][0];
}
void GlopHTML::moveToNextClass(std::string classname){
	if(classes.count(classname) == 0) return;
	for(int x=0; x<classes[classname].size()-1; x++){ // find where we are now
		if(classes[classname][x]==currentNode)
			currentNode = classes[classname][x+1]; // move to the one after the current
	}
}
void GlopHTML::moveToId(std::string id){
	if(ids.count(id)){
		currentNode=ids[id];
	}
}
void GlopHTML::moveToCurrentChild(int index){
	if(currentNode==NULL)return;
	currentNode = currentNode->children[index];
}
void GlopHTML::moveToParent(){
	if(currentNode!=NULL){
		currentNode=currentNode->parent;
	}
}


















GlopHTML_consumer::GlopHTML_consumer(){
	head=NULL;
	curt=NULL;
	position=0;
}
void GlopHTML_consumer::parseHTML(string HHHHH){
	head=NULL;
	curt=NULL;
	html = HHHHH;
	position=0;

	parseHTML_recursive();
}
TAG* GlopHTML_consumer::getHead(){
	return head;
}
map<string,TAG*> GlopHTML_consumer::getIds(){
	return ids;
}
map<string,vector<TAG*> > GlopHTML_consumer::getClasses(){
	return classes;
}
bool GlopHTML_consumer::parseHTML_recursive(){
	string curtToken;
	TAG* tempTag;

	//try to find a valid tag
	curtToken = getNextTagToken();
	if(curtToken=="")return true;
	if(curtToken[0]=='/') return true;

	//try to make a valid tag
	tempTag = parseTagToken(curtToken);
	if(tempTag==NULL)return true;
	tempTag->parent=curt;
	if(curt!=NULL)curt->children.push_back(tempTag);
	else head=tempTag;
	curt=tempTag;

	//parse for content and sub-tags now
	bool isEndTag=false;
	while(!isEndTag){
		curt->content.push_back(parseForContentUntilNextTag());
		isEndTag = parseHTML_recursive();
	}
	if(curt!=NULL)curt=curt->parent;
	return false;
}

string GlopHTML_consumer::getNextTagToken(){
	int start = html.find("<",position);
	if(start == string::npos) return "";
	else start+=1; // move past the char to get to the content

	int end   = html.find(">",start);
	if(end == string::npos) end=html.length();

	string tag = html.substr(start,end-start);

	if(tag.find("!--")==0){ // this is a comment and we need to ignore it
		end = html.find("-->",position);
		if(end == string::npos) end=html.length();
		else end+=3; // length of -->
		if(end > html.length()) end=html.length();
		position=end;
		tag = getNextTagToken();
	}

	position=end;
	return tag;
}

TAG* GlopHTML_consumer::parseTagToken(string tag){
	if(tag == "") return NULL;
	TAG* temp = new TAG;

	int curt=0;
	while(tag[curt]==' ' && curt<tag.length())curt++;

	while(tag[curt]!=' ' && curt<tag.length()){
		temp->name+=tag[curt];
		curt++;
	}

	string key,value;
	while(curt<tag.length()){
		key="";
		value="";
		//eat any whitespace
		while(tag[curt]==' ' && curt<tag.length())curt++;
		//get the key
		while(tag[curt]!=' ' && tag[curt]!='=' && curt<tag.length()){
			key+=tag[curt];
			curt++;
		}

		//eat the whitespace and = between the key and value and also mark the quote mark being used
		while( (tag[curt]==' '||tag[curt]=='=')  && curt<tag.length()) curt++;
		char quotes=tag[curt];
		curt++; // skip the quote mark
		while( tag[curt]!=quotes && curt<tag.length()){
			if(tag[curt]=='\\') // skip backslashes
				curt++;
			value+=tag[curt];
			curt++;
		}
		curt++; // skip the quote mark
		temp->params[key]=value;
	}
	if(temp->params.count("class")>0){
		temp->classname = temp->params["class"];
		temp->params.erase("class");
		classes[temp->classname].push_back(temp);
	}
	if(temp->params.count("id")>0){
		temp->id = temp->params["id"];
		temp->params.erase("id");
		ids[temp->id]=temp;
	}
	return temp;
}

string GlopHTML_consumer::parseForContentUntilNextTag(){
	if(html[position]=='>') position++; // make sure we are past the end of the tag
	if(html[position]=='<') return "";
	int end = html.find('<',position);
	if(end==string::npos)end=html.length();
	string segment=html.substr(position,end-position);
	position = end;
	return segment;
}