#include "gizwits_product.hpp"
#include "hardware.hpp"
#include <cstdlib>
namespace gagent::board::hardware {
bool initialize() noexcept { return false; }
bool pressed() noexcept { return false; }
bool show(ui::Color) noexcept { return false; }
void restart() noexcept { std::abort(); }
}
#include "gagent/protocol/device.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
#ifdef GAGENT_CAMERA_ENABLED
#include "gagent/camera/service.hpp"
namespace gagent::camera {
struct FakeCamera final : Control {
    unsigned calls=0; std::uint8_t operation=0, result=0;
    void sync(bool,bool,const storage::Snapshot*,const std::array<std::uint8_t,6>&) noexcept override {}
    std::uint8_t request(std::uint8_t op) noexcept override {++calls;operation=op;return result;}
    Status status() const noexcept override {return {};}
    bool idle() const noexcept override {return true;}
} fake_camera;
Control& service() noexcept {return fake_camera;}
}
#endif
using namespace gagent;
namespace dp = board::datapoints;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%d: %s\n", __LINE__, #x); std::abort(); } } while (0)
struct Route final : device::Route {
    unsigned broadcasts=0;
    std::vector<std::uint8_t> bytes;
    protocol::Status send(const protocol::Context&, const protocol::Frame&) noexcept override { return protocol::Status::ok; }
    device::Broadcast broadcast(const protocol::Frame& frame) noexcept override {
        ++broadcasts; bytes.assign(frame.payload.data,frame.payload.data+frame.payload.size);
        device::Broadcast result; result.sessions[0]=1; result.count=1; return result;
    }
    void failed(std::uint32_t) noexcept override { std::abort(); }
};
int main() {
    board::product::initialize();
    device::Soc endpoint(board::product::control); device::Router router(endpoint);
    Route cloud; CHECK(router.bind(protocol::Channel::mqtt,cloud));
    reports::Service reports(router,nullptr,nullptr); CHECK(reports.attach());
    board::product::tick(reports,100); CHECK(cloud.broadcasts==1);
    board::product::tick(reports,1100); CHECK(cloud.broadcasts==1);
    board::product::tick(reports,10100); CHECK(cloud.broadcasts==2);
    dp::DataPoints state{}; state.value_feed_portion=7;
    dp::Fields fields{}; fields[std::size_t(dp::Field::dp_feed_portion)]=true;
    std::array<std::uint8_t,dp::max_payload> input{},output{},expected{};
    std::size_t length=0,written=0,size=0;
    CHECK(dp::encode_status(state,fields,dp::report_action,input.data(),input.size(),length)==protocol::Status::ok);
    input[0]=dp::write_action;
    CHECK(board::product::control({input.data(),length},output.data(),output.size(),written)==protocol::Status::ok && written==0);
    board::product::tick(reports,10200); CHECK(cloud.broadcasts==2);
    board::product::tick(reports,11100); CHECK(cloud.broadcasts==3);
    const std::uint8_t query[]={dp::read_action,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    CHECK(board::product::control({query,sizeof(query)},output.data(),output.size(),written)==protocol::Status::ok);
    CHECK(dp::encode_status(state,dp::all_fields(),dp::read_response_action,expected.data(),expected.size(),size)==protocol::Status::ok);
    CHECK(written==size && output==expected);
    // A truncated write must leave the previous state intact.
    CHECK(board::product::control({input.data(),length-1},output.data(),output.size(),written)==protocol::Status::malformed);
    CHECK(board::product::control({query,sizeof(query)},output.data(),output.size(),written)==protocol::Status::ok && output==expected);
    fields={}; fields[std::size_t(dp::Field::dp_feeding)]=true; state.value_feeding=true;
    CHECK(dp::encode_status(state,fields,dp::report_action,input.data(),input.size(),length)==protocol::Status::ok);
    input[0]=dp::write_action;
    CHECK(board::product::control({input.data(),length},output.data(),output.size(),written)==protocol::Status::malformed);
    board::product::initialize();
    CHECK(board::product::control({query,sizeof(query)},output.data(),output.size(),written)==protocol::Status::ok && output!=expected);
    // 摄像头请求被拒绝时不提交数据点；重复开始不重复入队，停止可取消启动。
    fields={}; fields[std::size_t(dp::Field::dp_start_stream)]=true;
    state={}; state.value_start_stream=true;
    CHECK(dp::encode_status(state,fields,dp::report_action,input.data(),input.size(),length)==protocol::Status::ok);
    input[0]=dp::write_action;
#ifdef GAGENT_CAMERA_ENABLED
    camera::fake_camera.result=3;
#endif
    CHECK(board::product::control({input.data(),length},output.data(),output.size(),written)==protocol::Status::handler_error);
    CHECK(board::product::control({query,sizeof(query)},output.data(),output.size(),written)==protocol::Status::ok);
    const auto before=output;
#ifdef GAGENT_CAMERA_ENABLED
    camera::fake_camera.result=0;
    CHECK(board::product::control({input.data(),length},output.data(),output.size(),written)==protocol::Status::ok);
    CHECK(camera::fake_camera.calls==2 && camera::fake_camera.operation==1);
    CHECK(board::product::control({input.data(),length},output.data(),output.size(),written)==protocol::Status::ok);
    CHECK(camera::fake_camera.calls==2);
    state.value_start_stream=false;
    CHECK(dp::encode_status(state,fields,dp::report_action,input.data(),input.size(),length)==protocol::Status::ok);
    input[0]=dp::write_action;
    CHECK(board::product::control({input.data(),length},output.data(),output.size(),written)==protocol::Status::ok);
    CHECK(camera::fake_camera.calls==3 && camera::fake_camera.operation==2);
#endif
    CHECK(board::product::control({query,sizeof(query)},output.data(),output.size(),written)==protocol::Status::ok && output==before);
    std::puts("SDK template control/query/report, rate limit, rejection and reset passed");
}
