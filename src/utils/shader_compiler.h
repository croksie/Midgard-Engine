#pragma once
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <unknwn.h>
#include <windows.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
#else
template <typename T> using ComPtr = CComPtr<T>;
#endif

#include <dxcapi.h>
#include <spirv_glsl.hpp>
#include <utils/log.h>

namespace midgard::utils::shader {

inline std::vector<uint32_t> compileHlslToSpirv(const std::string& sourceCode, const std::wstring& entryPoint,
                                                const std::wstring& targetProfile) {

    ComPtr<IDxcUtils> utils;
    ComPtr<IDxcCompiler3> compiler;
    HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    if (FAILED(hr) || !utils) {
        ENGINE_LOG_ERROR("Failed to create DxcUtils instance: hr = 0x{:08x}", static_cast<uint32_t>(hr));
        return {};
    }

    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    if (FAILED(hr) || !compiler) {
        ENGINE_LOG_ERROR("Failed to create DxcCompiler instance: hr = 0x{:08x}", static_cast<uint32_t>(hr));
        return {};
    }

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = sourceCode.data();
    sourceBuffer.Size = sourceCode.size();
    sourceBuffer.Encoding = DXC_CP_UTF8;

    std::vector<LPCWSTR> arguments = {
        L"-E",
        entryPoint.c_str(),
        L"-T",
        targetProfile.c_str(),
        L"-spirv",
        L"-fspv-target-env=vulkan1.3",
        L"-Zpr",
#ifndef NDEBUG
        L"-Od",
        L"-Zi",
#else
        L"-O3",
#endif
    };

    ComPtr<IDxcResult> result;
    hr = compiler->Compile(&sourceBuffer, arguments.data(), static_cast<uint32_t>(arguments.size()), nullptr, IID_PPV_ARGS(&result));
    if (FAILED(hr) || !result) {
        ENGINE_LOG_ERROR("DXC compile call failed: hr = 0x{:08x}", static_cast<uint32_t>(hr));
        return {};
    }

    ComPtr<IDxcBlobUtf8> errors;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    if (errors && errors->GetStringLength() > 0) {
        ENGINE_LOG_ERROR("DXC Warning/Error:\n{}", errors->GetStringPointer());
    }
    HRESULT status = S_OK;
    result->GetStatus(&status);
    if (FAILED(status)) {
        ENGINE_LOG_ERROR("DXC compilation failed with status 0x{:08x}", static_cast<uint32_t>(status));
        return {};
    }

    ComPtr<IDxcBlob> spirvBlob;
    hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirvBlob), nullptr);
    if (FAILED(hr) || !spirvBlob || spirvBlob->GetBufferSize() == 0) {
        ENGINE_LOG_ERROR("Failed to get DXC output object: hr = 0x{:08x}", static_cast<uint32_t>(hr));
        return {};
    }

    const uint32_t* pStart = reinterpret_cast<const uint32_t*>(spirvBlob->GetBufferPointer());
    size_t sizeInWords = spirvBlob->GetBufferSize() / sizeof(uint32_t);
    return std::vector<uint32_t>(pStart, pStart + sizeInWords);
}

inline std::vector<uint32_t> compileHlslToSpirv(const std::string& sourceCode, const std::string& entryPoint,
                                                const std::string& targetProfile) {
    std::wstring wEntryPoint(entryPoint.begin(), entryPoint.end());
    std::wstring wTargetProfile(targetProfile.begin(), targetProfile.end());
    return compileHlslToSpirv(sourceCode, wEntryPoint, wTargetProfile);
}

inline std::string transpileSpirvToGlsl(const std::vector<uint32_t>& spirvBinary) {
    if (spirvBinary.empty())
        return "";
    spirv_cross::CompilerGLSL glslCompiler(spirvBinary);

    spirv_cross::CompilerGLSL::Options options;
    options.version = 450;
    options.es = false;
    options.enable_420pack_extension = true; // activate layout(binding = X)
    options.emit_line_directives = false;
    glslCompiler.set_common_options(options);

    glslCompiler.build_dummy_sampler_for_combined_images();
    glslCompiler.build_combined_image_samplers();

    auto resources = glslCompiler.get_shader_resources();
    for (const auto& ubo : resources.uniform_buffers) {
        uint32_t set = glslCompiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
        if (set == 1) {
            uint32_t binding = glslCompiler.get_decoration(ubo.id, spv::DecorationBinding);
            glslCompiler.set_decoration(ubo.id, spv::DecorationBinding, binding + 1);
        }
    }

    for (const auto& pcb : resources.push_constant_buffers) {
        glslCompiler.set_name(pcb.id, "u_Local");
        glslCompiler.set_name(pcb.base_type_id, "LocalData");
        glslCompiler.set_member_name(pcb.base_type_id, 0, "model");
    }

    try {
        return glslCompiler.compile();
    } catch (const spirv_cross::CompilerError& e) {
        ENGINE_LOG_ERROR("Erreur SPIRV-Cross : {}", e.what());
        return "";
    }
}

} // namespace midgard::utils::shader
