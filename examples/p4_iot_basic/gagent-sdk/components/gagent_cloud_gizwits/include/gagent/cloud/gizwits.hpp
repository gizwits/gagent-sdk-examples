#pragma once
#include "gagent/cloud/provider.hpp"
#include "cloud_json.hpp"
namespace gagent::identity {Renewal renew_credentials(runtime::Boot&,Random) noexcept;}
namespace gagent::ota {bool check_request(const storage::Snapshot&,const CheckMetadata&,cloud::Request&) noexcept;}
namespace gagent::mqtt {
bool configure(const storage::Snapshot&,const cloud::Response&,bool,Configuration&) noexcept;
Protocol& gizwits_protocol() noexcept; // Stateless wire codec; sessions live in mqtt::Service.
}
namespace gagent::cloud {
ConfigurationScope gizwits_configuration() noexcept;
const ProviderPolicy& gizwits_policy() noexcept;
// All HTTP application wire compatibility is here; transports only move bounded requests/responses.
class Codec final : public HttpProtocol {
public:
    ConfigurationScope configuration_scope() const noexcept override {return gizwits_configuration();}
    Codec(storage::Md5 md5,Cipher encrypt,Cipher decrypt,Json json,UpdateJson update=nullptr) noexcept : md5_(md5),encrypt_(encrypt),decrypt_(decrypt),json_(json),update_(update) {}
    bool request(Operation,const storage::Snapshot&,const Metadata&,Request&) noexcept;
    bool response(Operation,int http_status,std::string_view body,std::string_view product_secret,Response&) noexcept;
    bool initial(const storage::Snapshot&,Operation&) const noexcept override;
    Preparation prepare(Operation,const storage::Snapshot&,const Metadata&,bool,Address&,Request&,Response&) noexcept override;
    bool decode(Operation,int,std::string_view,const storage::Snapshot&,Response&) noexcept override;
    bool commit(Operation,runtime::Boot&,const Response&) noexcept override;
    CleanupState cleanup(const storage::Snapshot&) const noexcept override;
    std::uint64_t refresh_interval() const noexcept override {return provision_refresh_ms;}
    bool ota_checks() const noexcept override {return true;}
    bool multi_ota_checks() const noexcept override {return true;}
    bool prepare_update(const storage::Snapshot&,const ota::CheckMetadata&,bool,Address&,Request&) noexcept override;
    bool decode_update(std::string_view body,ota::CheckReply& out) noexcept override {return update_ && update_(body,out);}
    identity::Renewal renew(runtime::Boot& boot,identity::Random random) noexcept override {return identity::renew_credentials(boot,random);}
private:
    bool random_code(const storage::Snapshot&,std::string_view product_key) noexcept;
    storage::Md5 md5_;Cipher encrypt_,decrypt_;Json json_;UpdateJson update_;
    static constexpr std::size_t response_wire_limit=4096;
    struct Buffers {std::array<std::uint8_t,response_wire_limit/2> scratch{},plain{};};
    std::array<char,33> random_{};
};
struct GizwitsOptions {};
class Gizwits {
public:
    Gizwits(const GizwitsOptions&,const Dependencies& d) noexcept : Gizwits(d.md5,d.encrypt,d.decrypt) {}
    Gizwits(storage::Md5 md5,Cipher encrypt,Cipher decrypt,Json json=platform::cloud_json,UpdateJson update=platform::ota_check_json) noexcept
        : http_(md5,encrypt,decrypt,json,update),provider_{http_,mqtt::gizwits_protocol(),gizwits_policy()} {}
    Gizwits(const Gizwits&)=delete;
    Gizwits& operator=(const Gizwits&)=delete;
    const Provider& provider() const noexcept {return provider_;}
private:
    Codec http_;
    Provider provider_;
};
}
