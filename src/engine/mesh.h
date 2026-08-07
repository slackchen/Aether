#pragma once

#include "core/platform.h"
#include "rhi/rhi.h"
#include <memory>
#include <vector>

namespace aether::engine {

class Mesh {
public:
    void set_vertices(const f32* data, u32 vertex_count, u32 stride_bytes,
                      const std::vector<rhi::VertexAttribute>& attributes);
    void set_indices(const u32* data, u32 index_count);

    bool upload(rhi::RHIDevice* device);
    bool is_uploaded() const { return vertex_buffer_ != nullptr; }

    std::shared_ptr<rhi::RHIBuffer> vertex_buffer() const { return vertex_buffer_; }
    std::shared_ptr<rhi::RHIBuffer> index_buffer() const { return index_buffer_; }
    const rhi::VertexBufferLayout& vertex_layout() const { return vertex_layout_; }
    u32 vertex_count() const { return vertex_count_; }
    u32 index_count() const { return static_cast<u32>(indices_.size()); }
    bool has_indices() const { return !indices_.empty(); }

private:
    std::vector<f32> vertices_;
    std::vector<u32> indices_;
    u32 vertex_count_ = 0;
    rhi::VertexBufferLayout vertex_layout_;
    std::vector<rhi::VertexAttribute> attributes_;
    std::shared_ptr<rhi::RHIBuffer> vertex_buffer_;
    std::shared_ptr<rhi::RHIBuffer> index_buffer_;
};

}
