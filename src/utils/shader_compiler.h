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
#endif

#include <dxcapi.h>
#include <spirv_glsl.hpp>

#include <utils/log.h>


using Microsoft::WRL::ComPtr;

namespace midgard::utils::shader {

inline std::vector<uint32_t> compileHlslToSpirv(
    const std::string &sourceCode,
    const std::wstring &entryPoint,
    const std::wstring &targetProfile)
{
    
    ComPtr<IDxcUtils> utils;
    ComPtr<IDxcCompiler3> compiler;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

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
        L"-fvk-use-dx-layout",
        L"-Zpr",
    #ifdef _DEBUG
        L"-Od",
        L"-Zi",
    #else
        L"-O3",
    #endif
    };

    ComPtr<IDxcResult> result;
    compiler->Compile(&sourceBuffer, arguments.data(), (uint32_t)arguments.size(),
                        nullptr, IID_PPV_ARGS(&result));

    ComPtr<IDxcBlobUtf8> errors;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    if (errors && errors->GetStringLength() > 0) {
        ENGINE_LOG_ERROR("DXC Warning/Error:\n{}", errors->GetStringPointer());
    }
    HRESULT status;
    result->GetStatus(&status);
    if (FAILED(status)) {
        return {};
    }

    ComPtr<IDxcBlob> spirvBlob;
    result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirvBlob), nullptr);
    const uint32_t *pStart =
        reinterpret_cast<const uint32_t *>(spirvBlob->GetBufferPointer());
    size_t sizeInWords = spirvBlob->GetBufferSize() / sizeof(uint32_t);
    return std::vector<uint32_t>(pStart, pStart + sizeInWords);
}

inline std::vector<uint32_t>compileHlslToSpirv(
    const std::string &sourceCode,
    const std::string &entryPoint,
    const std::string &targetProfile) 
{
    std::wstring wEntryPoint(entryPoint.begin(), entryPoint.end());
    std::wstring wTargetProfile(targetProfile.begin(), targetProfile.end());
    return compileHlslToSpirv(sourceCode, wEntryPoint, wTargetProfile);
}

inline std::string transpileSpirvToGlsl(const std::vector<uint32_t> &spirvBinary) {
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

    try {
        return glslCompiler.compile();
    } catch (const spirv_cross::CompilerError &e) {
        ENGINE_LOG_ERROR("Erreur SPIRV-Cross : {}", e.what());
        return "";
    }
}

}; // namespace midgard::utils::shader
