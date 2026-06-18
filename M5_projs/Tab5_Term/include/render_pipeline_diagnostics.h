#pragma once

#include <stddef.h>
#include <stdint.h>

namespace render_pipeline {

struct Stats {
    uint32_t uart_read_bytes;
    uint32_t software_buffered_bytes;
    uint32_t software_dropped_bytes;
    uint32_t rendered_bytes;
    uint32_t mirrored_bytes;
    uint32_t render_batches;
    bool mirror_enabled;
    size_t software_rx_depth;
    size_t software_rx_capacity;
    size_t software_rx_max_depth;
};

Stats stats();
void resetStats();
bool mirrorEnabled();
void setMirrorEnabled(bool enabled);

} // namespace render_pipeline
