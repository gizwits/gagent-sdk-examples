#pragma once
#include <array>
#include <string_view>
namespace gagent::platform {
// ESP app-description date/time use __DATE__ / __TIME__. Legacy bTime is YYYYMMDDhhmmss.
inline std::array<char,15> build_timestamp(std::string_view date,std::string_view time) noexcept {
    std::array<char,15> result{};
    if(date.size()!=11 || time.size()!=8 || date[3]!=' ' || date[6]!=' ' || time[2]!=':' || time[5]!=':')return result;
    constexpr std::string_view months="JanFebMarAprMayJunJulAugSepOctNovDec";
    unsigned month=0;
    for(unsigned i=0;i<12;++i)if(date.substr(0,3)==months.substr(i*3,3)){month=i+1;break;}
    if(!month)return result;
    const auto digit=[](char c){return c>='0' && c<='9';};
    for(unsigned i=7;i<11;++i)if(!digit(date[i]))return result;
    if((date[4]!=' ' && !digit(date[4])) || !digit(date[5]))return result;
    for(unsigned i:{0u,1u,3u,4u,6u,7u})if(!digit(time[i]))return result;
    const unsigned day=(date[4]==' '?0:unsigned(date[4]-'0'))*10+unsigned(date[5]-'0');
    if(!day || day>31 || (time[0]-'0')*10+time[1]-'0'>23 ||
       (time[3]-'0')*10+time[4]-'0'>59 || (time[6]-'0')*10+time[7]-'0'>59)return result;
    for(unsigned i=0;i<4;++i)result[i]=date[7+i];
    result[4]=char('0'+month/10);result[5]=char('0'+month%10);
    result[6]=char('0'+day/10);result[7]=char('0'+day%10);
    result[8]=time[0];result[9]=time[1];result[10]=time[3];result[11]=time[4];result[12]=time[6];result[13]=time[7];
    return result;
}
}
