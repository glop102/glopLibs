#include <gtest/gtest.h>
#include "glopHTML.h"

TEST(html_consumer,getNextTag_basic){
	GlopHTML_consumer con;
	con.html = "<html></html>";
	con.position=0;
	string out = con.getNextTagToken();
	ASSERT_EQ(out,"html");
	out = con.getNextTagToken();
	ASSERT_EQ(out,"/html");
	out = con.getNextTagToken();
	ASSERT_EQ(out,"");
}

TEST(html_consumer,getNextTag_basic_missing){
	GlopHTML_consumer con;
	con.html = "<html></html";
	con.position=0;
	string out = con.getNextTagToken();
	ASSERT_EQ(out,"html");
	out = con.getNextTagToken();
	ASSERT_EQ(out,"/html");
	out = con.getNextTagToken();
	ASSERT_EQ(out,"");
}
TEST(html_consumer,getNextTag_comment){
	GlopHTML_consumer con;
	con.html = "<!-- <html> --><rofl>";
	con.position=0;
	string out = con.getNextTagToken();
	ASSERT_EQ(out,"rofl");
}
TEST(html_consumer,getNextTag_comment_advanced){
	GlopHTML_consumer con;
	con.html = "<!-- <html> --> <!-- > -- __--> <rofl>";
	con.position=0;
	string out = con.getNextTagToken();
	ASSERT_EQ(out,"rofl");
}

TEST(html_consumer,parseTagToken_empty){
	GlopHTML_consumer con;
	string html = "";
	TAG* temp = con.parseTagToken(html);
	EXPECT_EQ(temp,(TAG*)NULL);
	delete temp;
}
TEST(html_consumer,parseTagToken_basic){
	GlopHTML_consumer con;
	string html = "html par=\"val\"";
	TAG* temp = con.parseTagToken(html);
	EXPECT_EQ(temp->name,"html");
	EXPECT_EQ(temp->params.size(),1);
	EXPECT_EQ(temp->params.begin()->first,"par");
	EXPECT_EQ(temp->params.begin()->second,"val");
	delete temp;
}
TEST(html_consumer,parseTagToken_multiple){
	GlopHTML_consumer con;
	string html = "html par = \"val\" qrs = ' tuv '";
	TAG* temp = con.parseTagToken(html);
	EXPECT_EQ(temp->name,"html");
	EXPECT_EQ(temp->params.size(),2);
	auto itt = temp->params.begin();
	EXPECT_EQ(itt->first,"par");
	EXPECT_EQ(itt->second,"val");
	itt++;
	EXPECT_EQ(itt->first,"qrs");
	EXPECT_EQ(itt->second," tuv ");
	delete temp;
}
TEST(html_consumer,parseTagToken_class_and_id){
	GlopHTML_consumer con;
	string html = "html class = \"school\" id = 'liscense'";
	TAG* temp = con.parseTagToken(html);
	EXPECT_EQ(temp->name,"html");
	EXPECT_EQ(temp->params.size(),0);

	EXPECT_EQ(temp->classname,"school");
	EXPECT_EQ(temp->id,"liscense");
	delete temp;
}

TEST(html_consumer,getContent_basic){
	GlopHTML_consumer con;
	con.html = "<html>Some Basic Content</html>";
	con.position=0;
	string temp = con.getNextTagToken();
	ASSERT_EQ(temp,"html");
	temp = con.parseForContentUntilNextTag();
	EXPECT_EQ(temp,"Some Basic Content");
	temp = con.getNextTagToken();
	EXPECT_EQ(temp,"/html");
}
TEST(html_consumer,getContent_subTags){
	GlopHTML_consumer con;
	con.html = "<html>Some <a></a> Basic Content</html>";
	con.position=0;
	string temp = con.getNextTagToken();
	ASSERT_EQ(temp,"html");
	temp = con.parseForContentUntilNextTag();
	EXPECT_EQ(temp,"Some ");
	temp = con.getNextTagToken();
	EXPECT_EQ(temp,"a");
	temp = con.parseForContentUntilNextTag();
	EXPECT_EQ(temp,"");
	temp = con.getNextTagToken();
	EXPECT_EQ(temp,"/a");
	temp = con.parseForContentUntilNextTag();
	EXPECT_EQ(temp," Basic Content");
	temp = con.getNextTagToken();
	EXPECT_EQ(temp,"/html");
}
TEST(html_consumer, parsing_withEndingtags){
	GlopHTML_consumer con;
	con.parseHTML("<html><a>link</a><p>content</p></html>");
	TAG* temp = con.getHead();
	ASSERT_NE(temp,(TAG*)NULL);
	ASSERT_EQ(temp->children.size(),2);

	ASSERT_EQ(temp->children[0]->content.size(),1);
	ASSERT_EQ(temp->children[0]->content[0],"link");
	ASSERT_EQ(temp->children[1]->content.size(),1);
	ASSERT_EQ(temp->children[1]->content[0],"content");

	ASSERT_EQ(temp->children[0]->parent,temp);
	ASSERT_EQ(temp->children[1]->parent,temp);
}
TEST(html_consumer, parsing_withImgTag){
	GlopHTML_consumer con;
	con.parseHTML("<html><img src='location'><img src='nowhere'></html>");
	TAG* temp = con.getHead();
	ASSERT_NE(temp,(TAG*)NULL);
	ASSERT_EQ(temp->children.size(),1);

	ASSERT_EQ(temp->children[0]->content.size(),1);
	ASSERT_EQ(temp->children[0]->content[0],"");
	ASSERT_EQ(temp->children[1]->content.size(),1);
	ASSERT_EQ(temp->children[1]->content[0],"");

	ASSERT_EQ(temp->children[0]->parent,temp);
	ASSERT_EQ(temp->children[1]->parent,temp);
}

TEST(html_client,basic_walking){
	GlopHTML g;
	g.parseHTML("<html><a>link</a><p>content</p></html>");
	ASSERT_EQ(g.getNumberChildren(),2);
	ASSERT_EQ(g.getChildName(0),"a");
	ASSERT_EQ(g.getChildName(1),"p");
	g.moveToCurrentChild(0);
	EXPECT_EQ(g.getCurrentName(),"a");
	g.moveToParent();
	EXPECT_EQ(g.getCurrentName(),"html");
}
TEST(html_client,moving_to_a_class){
	GlopHTML g;
	g.parseHTML("<html><a class=\"first\">link</a><p class='second'>content</p></html>");
	ASSERT_EQ(g.getNumberChildren(),2);
	ASSERT_EQ(g.getChildName(0),"a");
	ASSERT_EQ(g.getChildName(1),"p");
	g.moveToFirstClass("first");
	EXPECT_EQ(g.getCurrentName(),"a");
	g.moveToFirstClass("second");
	EXPECT_EQ(g.getCurrentName(),"p");
	g.moveToHead();
	EXPECT_EQ(g.getCurrentName(),"html");
}
TEST(html_client,moving_to_an_id){
	GlopHTML g;
	g.parseHTML("<html><a id=\"first\">link</a><p id='second'>content</p></html>");
	ASSERT_EQ(g.getNumberChildren(),2);
	ASSERT_EQ(g.getChildName(0),"a");
	ASSERT_EQ(g.getChildName(1),"p");
	g.moveToId("first");
	EXPECT_EQ(g.getCurrentName(),"a");
	g.moveToId("second");
	EXPECT_EQ(g.getCurrentName(),"p");
	g.moveToHead();
	EXPECT_EQ(g.getCurrentName(),"html");
}