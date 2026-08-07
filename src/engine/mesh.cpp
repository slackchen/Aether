#include "engine/mesh.h"

namespace aether::engine {

void Mesh::set_vertices(const f32* data, u32 vertex_count, u32 stride_bytes,
                        const std::vector<rhi::VertexAttribute>& attributes) {
    vertices_.assign(data, data + vertex_count * stride_bytes / sizeof(f32));
    vertex_count_ = vertex_count;
    vertex_layout_.stride = stride_bytes;
    vertex_layout_.attribute_count = static_cast<u32>(attributes.size());
    attributes_ = attributes;
    vertex_layout_.attributes = attributes_.data();
}

void Mesh::set_indices(const u32* data, u32 index_count) {
    indices_.assign(data, data + index_count);
}

bool Mesh::upload(rhi::RHIDevice* device) {
    if (!device || vertices_.empty()) return false;

    rhi::BufferDesc vb_desc;
    vb_desc.size = vertices_.size() * sizeof(f32);
    vb_desc.usage = (u32)rhi::BufferUsage::Vertex | (u32)rhi::BufferUsage::CopyDst;
    vb_desc.memory_type = rhi::BufferMemoryType::DeviceLocal;
    vertex_buffer_ = device->create_buffer(vb_desc, vertices_.data());
    if (!vertex_buffer_) return false;

    if (!indices_.empty()) {
        rhi::BufferDesc ib_desc;
        ib_desc.size = indices_.size() * sizeof(u32);
        ib_desc.usage = (u32)rhi::BufferUsage::Index | (u32)rhi::BufferUsage::CopyDst;
        ib_desc.memory_type = rhi::BufferMemoryType::DeviceLocal;
        index_buffer_ = device->create_buffer(ib_desc, indices_.data());
        if (!index_buffer_) return false;
    }
    return true;
}

}
