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