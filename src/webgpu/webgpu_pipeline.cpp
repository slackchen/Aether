#include "webgpu/webgpu_internal.h"
#include <cstdio>
#include <cstring>

namespace aether::webgpu {

WebGPUCommandEncoder::WebGPUCommandEncoder(WGPUDevice dev, WGPUQueue q)
    : device(dev), queue(q) {
    WGPUCommandEncoderDescriptor desc = {};
    desc.label = make_string_view("MainCommandEncoder");
    encoder = wgpuDeviceCreateCommandEncoder(device, &desc);
}

WebGPUCommandEncoder::~WebGPUCommandEncoder() {
    if (render_pass) wgpuRenderPassEncoderRelease(render_pass);
    if (command_buffer) wgpuCommandBufferRelease(command_buffer);
    if (encoder) wgpuCommandEncoderRelease(encoder);
}

void WebGPUCommandEncoder::begin_render_pass(const rhi::RenderPassDesc& desc) {
    WGPURenderPassDescriptor rp_desc = {};
    std::vector<WGPURenderPassColorAttachment> color_attachments;
    WGPURenderPassDepthStencilAttachment depth_attachment = {};

    color_attachments.resize(desc.color_attachment_count);
    for (u32 i = 0; i < desc.color_attachment_count; i++) {
        const auto& a = desc.color_attachments[i];
        auto* view = static_cast<WebGPUTextureView*>(a.view);
        color_attachments[i].view = view->view;
        color_attachments[i].resolveTarget = nullptr;
        color_attachments[i].depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        color_attachments[i].loadOp = convert_load_op(a.load_op);
        color_attachments[i].storeOp = convert_store_op(a.store_op);
        color_attachments[i].clearValue = {a.clear_color[0], a.clear_color[1], a.clear_color[2], a.clear_color[3]};
    }

    rp_desc.colorAttachmentCount = desc.color_attachment_count;
    rp_desc.colorAttachments = color_attachments.data();

    if (desc.depth_attachment) {
        const auto& da = *desc.depth_attachment;
        auto* view = static_cast<WebGPUTextureView*>(da.view);
        depth_attachment.view = view->view;
        depth_attachment.depthLoadOp = convert_load_op(da.load_op);
        depth_attachment.depthStoreOp = convert_store_op(da.store_op);
        depth_attachment.depthClearValue = da.clear_depth;
        depth_attachment.depthReadOnly = false;
        depth_attachment.stencilLoadOp = WGPULoadOp_Undefined;
        depth_attachment.stencilStoreOp = WGPUStoreOp_Undefined;
        depth_attachment.stencilClearValue = 0;
        depth_attachment.stencilReadOnly = true;
        rp_desc.depthStencilAttachment = &depth_attachment;
    }

    render_pass = wgpuCommandEncoderBeginRenderPass(encoder, &rp_desc);
}

void WebGPUCommandEncoder::end_render_pass() {
    if (render_pass) {
        wgpuRenderPassEncoderEnd(render_pass);
        wgpuRenderPassEncoderRelease(render_pass);
        render_pass = nullptr;
    }
}

void WebGPUCommandEncoder::set_pipeline(rhi::RHIRenderPipeline* pipeline) {
    if (!render_pass || !pipeline) return;
    auto* p = static_cast<WebGPURenderPipeline*>(pipeline);
    wgpuRenderPassEncoderSetPipeline(render_pass, p->pipeline);
}

void WebGPUCommandEncoder::set_bind_group(u32 group_index, rhi::RHIBindGroup* bind_group) {
    if (!render_pass || !bind_group) return;
    auto* bg = static_cast<WebGPUBindGroup*>(bind_group);
    wgpuRenderPassEncoderSetBindGroup(render_pass, group_index, bg->bind_group, 0, nullptr);
}

void WebGPUCommandEncoder::set_vertex_buffer(u32 slot, rhi::RHIBuffer* buffer, u64 offset) {
    if (!render_pass || !buffer) return;
    auto* b = static_cast<WebGPUBuffer*>(buffer);
    wgpuRenderPassEncoderSetVertexBuffer(render_pass, slot, b->buffer, offset, b->buffer_size - offset);
}

void WebGPUCommandEncoder::set_index_buffer(rhi::RHIBuffer* buffer, u64 offset, rhi::IndexFormat format) {
    if (!render_pass || !buffer) return;
    auto* b = static_cast<WebGPUBuffer*>(buffer);
    WGPUIndexFormat wgpu_format = format == rhi::IndexFormat::Uint16 ? WGPUIndexFormat_Uint16 : WGPUIndexFormat_Uint32;
    wgpuRenderPassEncoderSetIndexBuffer(render_pass, b->buffer, wgpu_format, offset, b->buffer_size - offset);
}

void WebGPUCommandEncoder::draw(u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
    if (!render_pass) return;
    wgpuRenderPassEncoderDraw(render_pass, vertex_count, instance_count, first_vertex, first_instance);
}

void WebGPUCommandEncoder::draw_indexed(u32 index_count, u32 instance_count, u32 first_index, i32 vertex_offset, u32 first_instance) {
    if (!render_pass) return;
    wgpuRenderPassEncoderDrawIndexed(render_pass, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void WebGPUCommandEncoder::finish() {
    if (render_pass) {
        end_render_pass();
    }
    if (encoder && !command_buffer) {
        WGPUCommandBufferDescriptor desc = {};
        desc.label = make_string_view("MainCommandBuffer");
        command_buffer = wgpuCommandEncoderFinish(encoder, &desc);
    }
}

void WebGPUCommandEncoder::submit() {
    if (!command_buffer) finish();
    if (command_buffer) {
        wgpuQueueSubmit(queue, 1, &command_buffer);
    }
}

}
