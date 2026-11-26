#pragma once
#include "gagent/cloud/file_upload.hpp"
namespace gagent::cloud {
// One HTTP multipart POST to /v2/files/upload/async. No filesystem, camera
// dependency, full-file copy, Base64 or download-after-upload verification.
class GizwitsFileUpload final : public FileUploadProtocol {
public:
    explicit GizwitsFileUpload(storage::Md5 md5) noexcept:md5_(md5) {}
    bool prepare(const storage::Snapshot&,std::uint64_t,std::string_view,std::size_t,
        std::string_view,std::string_view,FileUploadRequest&) noexcept override;
    bool response(int,std::string_view,char*,std::size_t) noexcept override;
private:
    storage::Md5 md5_;
};
}
