#pragma once
// Temporary heap cache for the peripheral HTTP Range source.
// Module/MCU streaming uses EspHttp's existing 512-byte read buffer.
// Override only in host tests that exercise smaller cache boundaries.
#ifndef GAGENT_OTA_DOWNLOAD_BUFFER_SIZE
#define GAGENT_OTA_DOWNLOAD_BUFFER_SIZE 16384
#endif
