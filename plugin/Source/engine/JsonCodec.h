#pragma once
#include <array>
#include <charconv>
#include <concepts>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace composer::jsonDetail {
inline constexpr size_t maximumBytes=32*1024*1024;
struct Invalid {};
// Return one Unicode scalar's byte width; never accept replacement/overlong forms.
inline size_t utf8Width(std::string_view s,size_t i){
    auto c=static_cast<unsigned char>(s[i]);
    if(c<128){if(c==0)throw Invalid{};return 1;}
    size_t n=c>=0xc2&&c<=0xdf?2:c>=0xe0&&c<=0xef?3:c>=0xf0&&c<=0xf4?4:0;
    if(n==0||n>s.size()-i)throw Invalid{};
    for(size_t j=1;j<n;++j)if((static_cast<unsigned char>(s[i+j])&0xc0)!=0x80)throw Invalid{};
    auto second=static_cast<unsigned char>(s[i+1]);
    if((c==0xe0&&second<0xa0)||(c==0xed&&second>=0xa0)||(c==0xf0&&second<0x90)||(c==0xf4&&second>=0x90))throw Invalid{};
    return n;
}
struct Reader {
    std::string_view input;size_t pos=0,notes=0,controls=0;
    void space(){while(pos<input.size()&&(input[pos]==' '||input[pos]=='\r'||input[pos]=='\n'||input[pos]=='\t'))++pos;}
    bool take(char c){space();if(pos<input.size()&&input[pos]==c){++pos;return true;}return false;}
    void expect(char c){if(!take(c))throw Invalid{};}
    bool null(){space();if(input.substr(pos,4)=="null"){pos+=4;return true;}return false;}
    uint32_t hex4(){
        if(input.size()-pos<4)throw Invalid{};
        uint32_t value=0;
        for(int i=0;i<4;++i){char c=input[pos++];int n=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;if(n<0)throw Invalid{};value=value*16+uint32_t(n);}
        return value;
    }
    std::string string(size_t maximum){
        expect('"');std::string out;
        auto append=[&](std::string_view text){if(text.size()>maximum-out.size())throw Invalid{};out.append(text);};
        while(pos<input.size()){
            auto c=static_cast<unsigned char>(input[pos++]);if(c=='"')return out;if(c<32)throw Invalid{};
            if(c=='\\'){
                if(pos==input.size())throw Invalid{};
                char escape=input[pos++];
                if(escape=='u'){
                    uint32_t scalar=hex4();
                    if(scalar>=0xd800&&scalar<=0xdbff){if(input.substr(pos,2)!="\\u")throw Invalid{};pos+=2;auto low=hex4();if(low<0xdc00||low>0xdfff)throw Invalid{};scalar=0x10000+(scalar-0xd800)*1024+(low-0xdc00);}
                    else if(scalar>=0xdc00&&scalar<=0xdfff)throw Invalid{};
                    if(scalar==0)throw Invalid{};
                    char bytes[4];size_t n=0;
                    if(scalar<128)bytes[n++]=char(scalar);
                    else if(scalar<2048){bytes[n++]=char(0xc0|(scalar>>6));bytes[n++]=char(0x80|(scalar&63));}
                    else if(scalar<65536){bytes[n++]=char(0xe0|(scalar>>12));bytes[n++]=char(0x80|((scalar>>6)&63));bytes[n++]=char(0x80|(scalar&63));}
                    else{bytes[n++]=char(0xf0|(scalar>>18));bytes[n++]=char(0x80|((scalar>>12)&63));bytes[n++]=char(0x80|((scalar>>6)&63));bytes[n++]=char(0x80|(scalar&63));}
                    append({bytes,n});
                }else{
                    char decoded=0;
                    switch(escape){case '"':decoded='"';break;case '\\':decoded='\\';break;case '/':decoded='/';break;case 'b':decoded='\b';break;case 'f':decoded='\f';break;case 'n':decoded='\n';break;case 'r':decoded='\r';break;case 't':decoded='\t';break;default:throw Invalid{};}
                    append({&decoded,1});
                }
            }else{
                const auto start=pos-1;auto n=utf8Width(input,start);append(input.substr(start,n));pos=start+n;
            }
        }
        throw Invalid{};
    }
    int64_t integer(int64_t low=INT64_MIN,int64_t high=INT64_MAX){
        space();size_t start=pos;if(pos<input.size()&&input[pos]=='-')++pos;
        if(pos==input.size()||input[pos]<'0'||input[pos]>'9')throw Invalid{};
        if(input[pos]=='0')++pos;else while(pos<input.size()&&input[pos]>='0'&&input[pos]<='9')++pos;
        int64_t value=0;auto parsed=std::from_chars(input.data()+start,input.data()+pos,value);
        if(parsed.ec!=std::errc{}||parsed.ptr!=input.data()+pos||value<low||value>high)throw Invalid{};
        return value;
    }
    template<class Fn>void object(std::initializer_list<std::string_view> names,Fn field){
        expect('{');uint64_t seen=0;
        if(!take('}'))for(;;){
            auto name=string(32);size_t index=0;for(auto allowed:names){if(allowed==name)break;++index;}
            if(index==names.size()||(seen&(uint64_t(1)<<index)))throw Invalid{};
            seen|=uint64_t(1)<<index;expect(':');field(index);
            if(take('}'))break;
            expect(',');
        }
        if(seen!=(uint64_t(1)<<names.size())-1)throw Invalid{};
    }
    template<class T,class Fn>std::vector<T> array(size_t maximum,Fn element){
        expect('[');std::vector<T> result;
        if(!take(']'))for(;;){
            if(result.size()==maximum)throw Invalid{};
            result.push_back(element());
            if(take(']'))break;
            expect(',');
        }
        return result;
    }
    bool done(){space();return pos==input.size();}
};
struct Writer {
    std::string output;
    void raw(std::string_view s){if(s.size()>maximumBytes-output.size())throw Invalid{};output.append(s);}
    void string(std::string_view s){
        raw("\"");constexpr char hex[]="0123456789abcdef";
        for(size_t i=0;i<s.size();){auto c=static_cast<unsigned char>(s[i]);auto n=utf8Width(s,i);
            if(c=='"')raw("\\\"");else if(c=='\\')raw("\\\\");
            else if(c<32){char escaped[]={'\\','u','0','0',hex[c>>4],hex[c&15]};raw({escaped,6});}
            else raw(s.substr(i,n));
            i+=n;
        }
        raw("\"");
    }
    template<std::integral T>void integer(T n){char text[32];auto end=std::to_chars(text,text+sizeof(text),n);if(end.ec!=std::errc{})throw Invalid{};raw({text,size_t(end.ptr-text)});}
    void key(std::string_view name){if(output.back()!='{')raw(",");string(name);raw(":");}
};
}
