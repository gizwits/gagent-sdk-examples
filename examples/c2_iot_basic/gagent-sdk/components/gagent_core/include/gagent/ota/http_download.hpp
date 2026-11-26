#pragma once
#include <cstdint>
#include <cstring>
#include <string_view>

namespace gagent::ota {
// Bounded owned headers: IDF frees callback strings immediately. A strong
// validator, when available, pins the representation across TCP connections.
// Without one, the controller still verifies the original whole-file digest.
struct ResumeHeaders {
    char etag[128]{},range[96]{},encoding[32]{};
    bool invalid=false,etag_unusable=false;
    static bool equal(std::string_view a,std::string_view b) noexcept {
        if(a.size()!=b.size())return false;
        for(std::size_t i=0;i<a.size();++i)if((a[i]|32)!=(b[i]|32))return false;
        return true;
    }
    void add(const char* name,const char* value) noexcept {
        char* dest=nullptr;std::size_t capacity=0;
        if(equal(name,"ETag")){dest=etag;capacity=sizeof(etag);}
        else if(equal(name,"Content-Range")){dest=range;capacity=sizeof(range);}
        else if(equal(name,"Content-Encoding")){dest=encoding;capacity=sizeof(encoding);}
        if(!dest)return;
        const auto n=strnlen(value,capacity);
        if(!n || n>=capacity || (dest[0] && std::strcmp(dest,value))){
            if(dest==etag)etag_unusable=true;else invalid=true;
            return;
        }
        std::memcpy(dest,value,n+1);
    }
    bool identity() const noexcept {return !invalid && (!encoding[0] || equal(encoding,"identity"));}
    bool strong() const noexcept {
        const auto n=std::strlen(etag);
        if(etag_unusable || n<2 || etag[0]!='"' || etag[n-1]!='"')return false;
        for(std::size_t i=1;i+1<n;++i)if(static_cast<unsigned char>(etag[i])<0x21 ||
            static_cast<unsigned char>(etag[i])>0x7e || etag[i]=='"')return false;
        return true;
    }
    bool matches(std::uint32_t offset,std::uint32_t total) const noexcept {
        std::string_view text=range;
        if(text.substr(0,6)!="bytes ")return false;
        text.remove_prefix(6);
        const auto number=[&](std::uint32_t& value){
            value=0;if(text.empty() || text.front()<'0' || text.front()>'9')return false;
            while(!text.empty() && text.front()>='0' && text.front()<='9'){
                const auto digit=unsigned(text.front()-'0');
                if(value>(UINT32_MAX-digit)/10)return false;
                value=value*10+digit;text.remove_prefix(1);
            }
            return true;
        };
        const auto separator=[&](char c){if(text.empty() || text.front()!=c)return false;text.remove_prefix(1);return true;};
        std::uint32_t first,last,size;
        return number(first) && separator('-') && number(last) && separator('/') && number(size) &&
            text.empty() && total && first==offset && last==total-1 && size==total && first<=last;
    }
};
// Shared HTTP response policy for module, MCU and peripheral downloads.
enum class DownloadResponse { start, resume, restart, retry, retry_full, reject };
inline DownloadResponse download_response(const ResumeHeaders& headers,int status,std::int64_t length,
    std::uint32_t offset,std::uint32_t total,bool accepted,bool full_request,const char* etag) noexcept {
    if(accepted && !full_request && (status==400 || status==405 || status==416 || status==501))
        return DownloadResponse::retry_full;
    if(status==408 || status==429 || status==500 || status==502 || status==503 || status==504)
        return DownloadResponse::retry;
    if(length<0 || !headers.identity())return DownloadResponse::reject;
    if(status==200){
        if((length && total && length!=total) || (headers.range[0] && !headers.matches(0,total)))
            return DownloadResponse::reject;
        return accepted?DownloadResponse::restart:DownloadResponse::start;
    }
    if(accepted && !full_request && status==206 && headers.matches(offset,total) &&
       (!length || length==std::int64_t(total-offset)) &&
       (!etag[0] || (headers.strong() && !std::strcmp(etag,headers.etag))))return DownloadResponse::resume;
    return DownloadResponse::reject;
}
inline constexpr unsigned download_retries=3;
inline constexpr unsigned download_retry_delay_ms(unsigned retry) noexcept {return 1000u<<retry;}
}
