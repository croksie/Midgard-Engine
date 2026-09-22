// =============================================================================
// Combined Base Shader (HLSL 6.0+)
// Contains both VSMain and PSMain entry points
// =============================================================================

// --- Common data ---

struct GlobalData {
    float4x4 view;
    float4x4 projection;
};

[[vk::binding(0, 0)]]
ConstantBuffer<GlobalData> u_Global : register(b0, space0);

struct VSOutput {
    float4 pos : SV_Position;
    [[vk::location(0)]] float3 normal : NORMAL;
    [[vk::location(1)]] float2 texCoord : TEXCOORD0;
};

// --- Vertex Stage ---

struct VSInputStandard {
    [[vk::location(0)]] float3 pos : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] float2 texCoord : TEXCOORD0;
};

struct LocalData {
    float4x4 model;
};

[[vk::push_constant]]
LocalData u_Local;

VSOutput VSMain(VSInputStandard input) {
    VSOutput output;
    float4 pos = float4(input.pos, 1.0f);
    pos = mul(pos, u_Local.model);
    pos = mul(pos, u_Global.view);
    pos = mul(pos, u_Global.projection);
    output.pos = pos;
    output.normal = input.normal;
    output.texCoord = input.texCoord;
    return output;
}

// --- Vertex Stage : Instance ---

struct VSInputInstanced {
    [[vk::location(0)]] float3 pos : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] float2 texCoord : TEXCOORD0;
    [[vk::location(3)]] float4x4 instanceModel : INSTANCE_MODEL; // Spans locations 3, 4, 5, 6
};


VSOutput VSMainInstanced(VSInputInstanced input) {
    VSOutput output;
    float4 pos = float4(input.pos, 1.0f);
    pos = mul(pos, input.instanceModel);
    pos = mul(pos, u_Global.view);
    pos = mul(pos, u_Global.projection);
    output.pos = pos;
    output.normal = input.normal;
    output.texCoord = input.texCoord;
    return output;
}

// --- Fragment Stage ---

struct PSInput {
    [[vk::location(0)]] float3 normal : NORMAL;
    [[vk::location(1)]] float2 texCoord : TEXCOORD0;
};

[[vk::combinedImageSampler]]
[[vk::binding(0, 1)]]
Texture2D u_Texture : register(t0, space1);

[[vk::combinedImageSampler]]
[[vk::binding(0, 1)]]
SamplerState u_Sampler : register(s0, space1);

float4 PSMain(PSInput input) : SV_Target {

    float4 texColor = u_Texture.Sample(u_Sampler, input.texCoord);

    float3 lightDir = normalize(float3(0.5f, 1.0f, -0.5f));
    float3 normal = normalize(input.normal);

    float diffuse = max(dot(normal, lightDir), 0.0f);
    float ambient = 0.2f;
    float lighting = ambient + diffuse * 0.8f;
    
    return float4(texColor.rgb * lighting, texColor.a);
}



// Alias for standard entry point 'main'
VSOutput main(VSInputStandard input) {
    return VSMain(input);
}