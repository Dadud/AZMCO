// PassThrough.hlsl
// Phase 4 pass-through shader for AZMCO DX11 backend.
//
// Strategy: AZMCO pre-transforms vertices on the CPU (XYZ is in clip space, RHW
// is 1/w already computed). The vertex shader just outputs position and passes
// through color + UV. The pixel shader samples texture (or just outputs color
// for non-textured draws) and modulates with vertex color.
//
// One shader covers most cases for Phase 4. Stage state permutations
// (modulate/decal/add/etc.) will be added in Phase 4+ as a second shader.

struct VSInput {
    float3 XYZ    : POSITION;
    float  RHW    : PSIZE;     // reuse PSIZE semantic for RHW
    uint   Color  : COLOR;
    uint   Spec   : COLOR1;
    float2 UV     : TEXCOORD0;
};

struct VSOutput {
    float4 Pos    : SV_POSITION;
    float4 Color  : COLOR0;
    float4 Spec   : COLOR1;
    float2 UV     : TEXCOORD0;
    float  RHW    : TEXCOORD1;  // pass RHW through for depth override
};

VSOutput VSMain(VSInput input) {
    VSOutput output;
    output.Pos   = float4(input.XYZ, input.RHW);  // RHW is w, pos = (xyz, w)
    output.Color = float4(
        ((input.Color >> 0)  & 0xFF) / 255.0,    // B (DX is BGRA in DWORD)
        ((input.Color >> 8)  & 0xFF) / 255.0,
        ((input.Color >> 16) & 0xFF) / 255.0,
        ((input.Color >> 24) & 0xFF) / 255.0);
    output.Spec  = float4(
        ((input.Spec >> 0)  & 0xFF) / 255.0,
        ((input.Spec >> 8)  & 0xFF) / 255.0,
        ((input.Spec >> 16) & 0xFF) / 255.0,
        ((input.Spec >> 24) & 0xFF) / 255.0);
    output.UV    = input.UV;
    output.RHW   = input.RHW;
    return output;
}

Texture2D    ShaderTexture : register(t0);
SamplerState ShaderSampler : register(s0);

float4 PSMain(VSOutput input) : SV_TARGET {
    // Phase 4 simplified: texture is optional, sample if bound
    float4 texColor = ShaderTexture.Sample(ShaderSampler, input.UV);
    return texColor * input.Color;
}
