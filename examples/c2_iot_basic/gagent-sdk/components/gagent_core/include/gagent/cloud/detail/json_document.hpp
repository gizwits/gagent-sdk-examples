#pragma once
#include "gagent/cloud/mqtt.hpp"
#include "cJSON.h"
#include <cmath>
#include <cstring>
namespace gagent::platform::json {
inline bool preflight(std::string_view s) noexcept {
    constexpr auto limit=mqtt::wire_limit>4096?mqtt::wire_limit:4096;
    if(s.empty() || s.size()>limit)return false;
    unsigned depth=0;bool string=false,escape=false;
    for(std::size_t i=0;i<s.size();++i){const unsigned char c=s[i];
        if(!c)return false;
        if(string){
            if(escape){if(c=='u' && s.substr(i+1,4)=="0000")return false;escape=false;}
            else if(c=='\\')escape=true;
            else if(c=='"')string=false;
            else if(c<32)return false;
        }else if(c=='"')string=true;
        else if(c=='{' || c=='['){if(++depth>16)return false;}
        else if(c=='}' || c==']'){if(!depth)return false;--depth;}
    }
    return !string && !escape && depth==0;
}
inline bool unique(const cJSON* node) noexcept {
    if(cJSON_IsObject(node))for(const auto* a=node->child;a;a=a->next)for(const auto* b=a->next;b;b=b->next)
        if(!a->string || !b->string || std::strcmp(a->string,b->string)==0)return false;
    for(const auto* child=node->child;child;child=child->next)if(!unique(child))return false;
    return true;
}
inline bool token(std::string_view s) noexcept {
    for(unsigned char c:s)if(!((c>='0'&&c<='9') || (c>='a'&&c<='z') || (c>='A'&&c<='Z') || c=='-' || c=='_' || c=='.'))return false;
    return true;
}
template<std::size_t N> bool text(const cJSON* node,std::array<char,N>& out,std::size_t minimum,bool hostname=false) noexcept {
    if(!cJSON_IsString(node) || !node->valuestring)return false;
    const auto size=std::strlen(node->valuestring);
    if(size<minimum || size>=N || !(hostname?cloud::valid_host({node->valuestring,size}):token({node->valuestring,size})))return false;
    std::memcpy(out.data(),node->valuestring,size);return true;
}
inline bool integer(const cJSON* node,double low,double high,std::int64_t& output) noexcept {
    double value;
    if(cJSON_IsNumber(node))value=node->valuedouble;
    else if(cJSON_IsString(node) && node->valuestring){
        const char* p=node->valuestring;bool negative=*p=='-';if(negative)++p;
        if(!*p)return false;
        value=0;
        while(*p){if(*p<'0' || *p>'9' || value>900719925474099.0)return false;value=value*10+(*p++-'0');}
        if(negative)value=-value;
    }else return false;
    if(!std::isfinite(value) || value<low || value>high || value!=std::floor(value))return false;
    output=static_cast<std::int64_t>(value);return true;
}
class Document {
public:
    explicit Document(std::string_view body) noexcept {
        if(!preflight(body))return;
        const char* end=nullptr;root_=cJSON_ParseWithLengthOpts(body.data(),body.size(),&end,0);
        if(!root_)return;
        bool valid=cJSON_IsObject(root_) && unique(root_) && end;
        if(valid)for(const auto* p=end;p<body.data()+body.size();++p)if(*p!=' ' && *p!='\t' && *p!='\r' && *p!='\n'){valid=false;break;}
        if(!valid){cJSON_Delete(root_);root_=nullptr;}
    }
    ~Document(){cJSON_Delete(root_);}
    Document(const Document&)=delete;
    Document& operator=(const Document&)=delete;
    const cJSON* get() const noexcept {return root_;}
private:
    cJSON* root_=nullptr;
};

}
