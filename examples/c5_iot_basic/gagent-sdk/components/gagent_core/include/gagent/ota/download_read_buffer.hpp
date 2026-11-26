#pragma once
#include "gagent/ota/buffer_memory.hpp"
#include "gagent/ota/download.hpp"
#include <memory>
#include <new>

namespace gagent::ota {
// Owned by an active download, with no allocation required for the fallback.
class DownloadReadBuffer {
public:
    void prepare() noexcept {
        if(buffer_memory_available(cloud::download_block_limit))
            large_.reset(new(std::nothrow) char[cloud::download_block_limit]);
    }
    bool shrink_if_needed() noexcept {
        if(!large_ || buffer_memory_available(0))return false;
        large_.reset();
        return true;
    }
    char* data() noexcept {return large_?large_.get():fallback_;}
    std::size_t size() const noexcept {return large_?cloud::download_block_limit:sizeof(fallback_);}
private:
    char fallback_[512];
    std::unique_ptr<char[]> large_;
};
}
