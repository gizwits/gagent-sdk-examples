"""Generate the camera-only H.264 1.3.0 reference-buffer fix in the build tree.

Only one encoder is owned by the MCU camera worker. Retaining its DMA reference
buffer across stop/start avoids the fragmentation seen in esp-cam. Never edits
managed_components or changes encoders in camera-disabled builds.
"""
import argparse
from pathlib import Path

def patched(text):
    marker='static const char *TAG = "H264_ENC.HW.SET";'
    free='''        if (param->ref) {
            esp_h264_free(param->ref);
        }'''
    alloc='    param->ref = (uint8_t *)esp_h264_aligned_calloc(16, 1, max_refame_buffer_size(param->mb_width), &actual_size, ESP_H264_MEM_INTERNAL);'
    for value in (marker,free,alloc):
        if text.count(value)!=1:raise ValueError('Unsupported H.264 source; review the pinned camera patch')
    text=text.replace(marker,marker+'''
// One camera encoder; preserve its released DMA reference buffer for reconnect.
static uint8_t *gagent_ref;
static uint32_t gagent_ref_size;
''')
    text=text.replace(free,'''        if (param->ref) {
            if (!gagent_ref) {
                gagent_ref=param->ref;
                gagent_ref_size=max_refame_buffer_size(param->mb_width);
            } else esp_h264_free(param->ref);
        }''')
    text=text.replace(alloc,'''    const uint32_t ref_size=max_refame_buffer_size(param->mb_width);
    if (gagent_ref && gagent_ref_size!=ref_size) {
        esp_h264_free(gagent_ref);gagent_ref=NULL;gagent_ref_size=0;
    }
    if (gagent_ref) {
        param->ref=gagent_ref;gagent_ref=NULL;gagent_ref_size=0;
        memset(param->ref,0,ref_size);
        esp_h264_cache_check_and_writeback(param->ref,ref_size);
    } else {
        param->ref = (uint8_t *)esp_h264_aligned_calloc(16, 1, ref_size, &actual_size, ESP_H264_MEM_INTERNAL);
    }''')
    return text
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
    value=patched(a.source.read_text());a.output.parent.mkdir(parents=True,exist_ok=True)
    if not a.output.exists() or a.output.read_text()!=value:a.output.write_text(value)
