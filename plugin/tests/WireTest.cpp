#include "../Source/Wire.h"
#include <iostream>
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::abort();}}
int main(){
 auto floating=wire::document("{\"start\":0.0}");check(floating["start"].isDouble(),"floating JSON state representation changed");
 auto huge=wire::document("{\"x\":18446744073709551676}");check(huge.isObject()&&double(huge["x"])>1e19&&!wire::integer(huge["x"],0,127),"integer wrapped before validation");
 auto scaled=wire::document("{\"x\":1000000000000000000000000000000000e-30}");check(scaled.isObject()&&double(scaled["x"])==1000,"valid scaled mantissa changed");
 auto exact=wire::document("{\"seed\":4294967295,\"version\":1.0,\"tiny\":1e-300,\"text\":\"te\\u0073t\"}");check(exact.isObject()&&wire::integer(exact["seed"],0,4294967295.0)&&wire::integer(exact["version"],1,1)&&double(exact["tiny"])>0&&exact["text"].toString()=="test","valid JSON values changed");
 for(auto text:{"{\"n\":01}","{\"n\":1.}","{\"n\":1e}","{\"n\":NaN}","{\"s\":\"te\\qst\"}","{\"n\":1,}","{\"n\":[1,]}","{} {}","{}garbage","{\"n\":1e9999}","{\"n\":--1}"})check(!wire::document(text).isObject(),"malformed JSON accepted");
 std::string nul="{}";nul+='\0';nul+="garbage";check(!wire::document(nul).isObject(),"NUL suffix hidden");
 std::string bad="{\"s\":\"";bad+=char(0xc0);bad+=char(0xaf);bad+="\"}";check(!wire::document(bad).isObject(),"invalid UTF8 accepted");
 auto deep=std::string("{\"x\":")+std::string(40,'[')+"0"+std::string(40,']')+"}";check(!wire::document(deep).isObject(),"unbounded nesting accepted");
 auto enormous=std::string("{\"x\":")+std::string(1000000,'9')+"}";check(!wire::document(enormous).isObject(),"unrepresentable number accepted");
 std::cout<<"strict byte JSON, numeric canonicalization, escapes, UTF8 and nesting passed\n";
}
