# AZMCO Renderer Interface Audit (Phase 1 deliverable)

Source: `Source/AZX/RendererModule.Basic.hxx` (844 lines, 34 KB)

This is the **single most important document** for building a DX11/Vulkan backend. Every renderer backend — DX7, DX8, DX8M, Software, future DX11/Vulkan — implements this interface. It defines:

- The fixed-function state machine AZMCO sets up per draw call
- The vertex / packet data structures
- The host↔renderer callback contract
- The device capability reporting

## Architecture overview

```
mcity.exe (host, 32-bit)
   │
   │ loads dynamically
   ▼
dx8z.dll / dx11.dll / vk.dll (renderer backend)
   │
   │ uses D3D8/D3D11/Vulkan under the hood
   ▼
GPU
```

The host calls into the renderer via ~150 entry points (defined by `Renderer.Module.MSVC.def`). The renderer reports its capabilities back via `RendererModuleDescriptor` and uses callbacks to ask the host for things like window handles and memory allocation.

## What the renderer must implement

### 1. Vertex pipeline (`RendererModulePacket` and friends)

```cpp
struct RendererModulePacket {
    RendererModulePrimitiveType Type;   // PointList, LineList, TriangleList, etc.
    u32 FVF;                            // Flexible Vertex Format flags
    void* Vertexes;                     // RTLVX* or RTLVX2* array
    u32 VertexCount;
    u16* Indexes;
    u32 IndexCount;
};
```

**AZMCO defines two vertex structs** (in `Renderer.Basic.hxx`):

```cpp
struct RTLVX {
    f32x3 XYZ;     // position (transformed)
    f32 RHW;       // reciprocal homogeneous W (already projected)
    u32 Color;     // ARGB diffuse
    u32 Specular;  // ARGB specular
    f32x2 UV;      // single texture coord
};

struct RTLVX2 {
    f32x3 XYZ;
    f32 RHW;
    u32 Color;
    u32 Specular;
    f32x2 UV1;     // dual texture coords (multi-texture)
    f32x2 UV2;
};
```

**Key insight for DX11/Vulkan**: vertices are **already pre-transformed** (XYZ in clip space, RHW = 1/w already computed). The renderer doesn't need full MVP transform matrices per draw call — just a pass-through vertex shader. The host (mcity.exe) does all the transform+light math on CPU.

This is a **massive simplification** for modern backends. No vertex shader needs to handle matrices.

### 2. Primitive types (D3DPRIMITIVETYPE equivalents)

`RendererModulePrimitiveType`:
- `PointList` = 1 → `D3DPT_POINTLIST`
- `LineList` = 2 → `D3DPT_LINELIST`
- `LineStrip` = 3 → `D3DPT_LINESTRIP`
- `TriangleList` = 4 → `D3DPT_TRIANGLELIST`
- `TriangleStrip` = 5 → `D3DPT_TRIANGLESTRIP`
- `TriangleFan` = 6 → `D3DPT_TRIANGLEFAN`

DX11 equivalents: `D3D11_PRIMITIVE_TOPOLOGY_POINTLIST`, etc. — direct mapping.

### 3. Fixed-function state (the hard part)

The host calls `RENDERER_MODULE_STATE_SELECT_*` setters to configure the renderer before each draw call. There are **~80+ state selectors** in the API. Grouped by concern:

#### Transform & lighting (state 420-449)
| Selector | Value | DX11 equivalent |
|---|---|---|
| `SELECT_TRANSFORM_WORLD` | D3DTS_WORLD | `cbuffer PerFrame.World` |
| `SELECT_TRANSFORM_VIEW` | D3DTS_VIEW | `cbuffer PerFrame.View` |
| `SELECT_TRANSFORM_PROJECTION` | D3DTS_PROJECTION | `cbuffer PerFrame.Projection` |
| `SELECT_TRANSFORM_MULTIPLY_WORLD` | n/a | `World = World * M` |
| `ACQUIRE_TRANSFORM_WORLD/VIEW/PROJECTION` | read back | `Map` + read |
| `SELECT_LIGHT` | D3DLIGHT8 × 7 | `cbuffer PerFrame.Lights[7]` |
| `SELECT_LIGHT_STATE` | BOOL × 7 | `cbuffer PerFrame.LightEnabled[7]` |
| `SELECT_CURRENT_MATERIAL` | D3DMATERIAL8 | `cbuffer PerObject.Material` |
| `SELECT_AMBIENT_STATE` | u32 ARGB | `cbuffer PerFrame.Ambient` |
| `SELECT_CLIPPING_STATE` | BOOL | `bClipEnabled` |
| `SELECT_LIGHTING_STATE` | BOOL | `bLightingEnabled` |

**DX11 mapping**: all of these go into a single `cbuffer PerFrame` updated once per frame (or per draw call for materials). Vertex shader does all lighting math. About 8-16 unique shader permutations to cover all combinations.

#### Rasterizer state (state 1-29)
| Selector | Value | DX11 equivalent |
|---|---|---|
| `SELECT_CULL_STATE` | NONE/CCW/CW | `RSSetState(CullMode)` |
| `SELECT_DEPTH_STATE` | INACTIVE/ACTIVE/ACTIVE_W | `OMSetDepthStencilState` |
| `SELECT_DEPTH_FUNCTION` | 8 modes | `D3D11_COMPARISON_FUNC` |
| `SELECT_DITHER_STATE` | INACTIVE/ACTIVE | `RSSetState(DitherEnabled)` |
| `SELECT_SHADE_STATE` | FLAT/GOURAUD/GOURAUD_SPECULAR | shader permutation |
| `SELECT_FILL_MODE` | SOLID/WIRE | `D3D11_FILL_MODE` |
| `SELECT_LINE_WIDTH` | f32 | `RSSetState` (not all GPUs support) |
| `SELECT_LINE_DOUBLE_STATE` | BOOL | n/a in DX11 — fake with 2 draws |
| `SELECT_HINT_STATE` | INACTIVE/ACTIVE | hint, ignored |

#### Texture stage state (state 41, 73-74, 406, etc.)
This is the **biggest mapping task**. AZMCO sets texture stage state per-stage (up to 8 stages). For each stage:

| Selector | Value | DX11 equivalent |
|---|---|---|
| `SELECT_TEXTURE_STAGE_BLEND_STATE` | 18 enum values (NORMAL, ADD, MODULATE, etc.) | shader permutation (modulate/add/blend/decal/etc.) |
| `SELECT_TEXTURE_ADDRESS_STATE` | CLAMP/WRAP/MIRROR | `D3D11_TEXTURE_ADDRESS_MODE` |
| `SELECT_TEXTURE_FILTER_STATE` | POINT/LINEAR/ANISOTROPY | `D3D11_FILTER` |
| `SELECT_TEXTURE_MIP_FILTER_STATE` | NONE/POINT/LINEAR | `D3D11_FILTER` |
| `SELECT_TEXTURE_STAGE_STATE` | full state set | `PSSetSamplers` + shader |
| `SELECT_SOURCE_BLEND_STATE` | 10 enum values | `D3D11_BLEND` |
| `SELECT_DESTINATION_BLEND_STATE` | 10 enum values | `D3D11_BLEND` |
| `SELECT_BUMP_MAPPING_*` | matrix, scale, offset | shader uniforms |

**Strategy for DX11**: precompile 8-16 shader variants covering common stage combinations. Map each `RENDERER_MODULE_TEXTURE_STAGE_BLEND_*` to a shader variant.

#### Blending & alpha (state 10, 56, 64, 104, 149-152)
| Selector | Value | DX11 equivalent |
|---|---|---|
| `SELECT_ALPHA_BLEND_STATE` | NONE/ACTIVE | `OMSetBlendState` enable |
| `SELECT_BLEND_STATE` | 4 modes | `D3D11_BLEND` × 2 (src/dst) |
| `SELECT_SOURCE_BLEND_STATE` | 10 modes | `SrcBlend` |
| `SELECT_DESTINATION_BLEND_STATE` | 10 modes | `DestBlend` |
| `SELECT_ALPHA_TEST_STATE` | 6 modes | shader `discard` if alpha < threshold |
| `SELECT_ALPHA_FUNCTION` | 8 modes | shader comparison |
| `SELECT_BLEND_FACTOR` | u32 ARGB | `OMSetBlendState(..., BlendFactor)` |

#### Fog (state 14, 15, 20-23, 105)
| Selector | Value | DX11 equivalent |
|---|---|---|
| `SELECT_FOG_STATE` | INACTIVE/ACTIVE/ACTIVE_LINEAR/EXP/EXP2 | shader uniforms |
| `SELECT_FOG_COLOR` | u32 ARGB | `cbuffer` uniform |
| `SELECT_FOG_DENSITY` | f32 | `cbuffer` uniform |
| `SELECT_FOG_START` | f32 | `cbuffer` uniform |
| `SELECT_FOG_END` | f32 | `cbuffer` uniform |
| `SELECT_FOG_ALPHAS` | u32 | shader uniform |

#### Lighting caps (state 436)
`RendererModuleTransformAndLightCapabilites`:
```cpp
struct {
    BOOL IsActive;
    u32 MaxActiveLights;
    u32 MaxUserClipPlanes;
    u32 MaxVertexBlendMatrices;
    BOOL IsTransformLightBufferSystemMemoryAvailable;
    BOOL IsTransformLightBufferVideoMemoryAvailable;
};
```

DX11 always reports: 8 max lights, 6 max clip planes, 4 matrices. No need to query.

### 4. Device capabilities (`RendererModuleDeviceCapabilities7/8`)

The renderer fills this struct so the host knows what features are supported:

```cpp
struct RendererModuleDeviceCapabilities8 {
    BOOL IsAccelerated;            // hardware accelerated?
    u32 RendererDepthBits;         // depth buffer bits
    u32 RenderScreenBits;          // back buffer bits
    BOOL IsDepthAvailable;
    BOOL IsDitherAvailable;
    BOOL IsWBufferAvailable;       // n/a in DX11, report FALSE
    BOOL IsWindowModeAvailable;
    BOOL IsPerspectiveTextures;
    BOOL IsAlphaBlending;
    BOOL IsAntiAliasingAvailable;
    BOOL IsAnisotropyAvailable;
    u32 MaxAnisotropy;             // 16 is safe
    BOOL IsGammaAvailable;
    BOOL IsStencilBufferAvailable;
    BOOL IsSpecularBlending;
    u32 MinTextureWidth;          // 1
    u32 MaxTextureWidth;          // 8192 typical
    u32 MaximumSimultaneousTextures;  // 1 for DX11 (multi-texture via array, not multi-stage)
    f32 MaxTextureRepeat;
    // ...
};
```

DX11 always reports "yes" to most things; WBuffer = FALSE; MaxAnisotropy = 16.

### 5. Lifecycle entry points

From `Renderer.Module.MSVC.def`, the host calls these by ordinal:

```
@1  AcquireDescriptor           — returns RendererModuleDescriptor
@2  ClearGameWindow             — clear back buffer
@3  ClipGameWindow              — set clip rect
@4  CreateGameWindow            — create device + window
@5  DestroyGameWindow           — destroy device
@6  DrawLine
@7  DrawLineMesh
@8  DrawLineStrip
@9  DrawPoints
@10 DrawRectangle
@11 DrawRectangles
@12 DrawTriangle
@13 DrawTriangleFan
@14 DrawTriangleMesh
@15 DrawTriangleStrip
@16 (etc.)
@35 AcquireRendererInstance
@36 LockWindow
... etc
```

**For DX11 backend (Phase 2 skeleton)**: stub all of these to return success/failure as appropriate. Only `CreateGameWindow` / `DestroyGameWindow` need actual implementation in skeleton phase.

### 6. Callback contract

The renderer can call back into the host via:

```cpp
typedef const HWND (STDCALLAPI* RENDERERMODULEACQUIREWINDOWLAMBDA)(void);
typedef const u32  (STDCALLAPI* RENDERERMODULEEXECUTECALLBACK)(...);
typedef void*      (STDCALLAPI* RENDERERMODULEALLOCATEMEMORYLAMBDA)(const u32 size);
typedef const void (STDCALLAPI* RENDERERMODULELOGLAMBDA)(const u32 severity, const char* message);
// ... and more
```

These are stored in `RendererModuleLambdaContainer` and given to the renderer at init. Modern backends use these sparingly — only `Log` is commonly invoked.

### 7. Memory model

The host passes vertex/index data in-place. The renderer **must not store the pointers** beyond the immediate draw call. The data is on the host's stack or in a buffer it controls.

For DX11: copy vertex data into a `ID3D11Buffer` per draw call (or use `Map` with `D3D11_MAP_WRITE_DISCARD` on a persistent buffer). Same for indices.

## Pixel format mapping

| AZMCO format | DX11 format | Notes |
|---|---|---|
| `R5G5B5` | `DXGI_FORMAT_B5G5R5A1_UNORM` | pad to RGBA |
| `R5G6B5` | `DXGI_FORMAT_B5G6R5_UNORM` | native |
| `A8R8G8B8` | `DXGI_FORMAT_B8G8R8A8_UNORM` | byte-swap |
| `P8` (paletted) | `DXGI_FORMAT_R8G8B8A8_UNORM` | upload via lookup table |
| `DXT1` | `BC1_UNORM` | direct |
| `DXT3` | `BC2_UNORM` | direct |
| `DXT5` | `BC3_UNORM` | direct |
| `YUV2` | `DXGI_FORMAT_YUY2` | native DX11 |
| `A1R5G5B5` | `DXGI_FORMAT_B5G5R5A1_UNORM` | rearrange |

## State combos that will explode the shader permutation count

The dangerous combinations:
- 8 texture stages × 18 stage blend modes = **144 shader variants** if naively cross-product
- 4 shade modes (FLAT/GOURAUD/GOURAUD_SPECULAR) × 4 light counts (0, 1, multiple) × fog on/off = **24 vertex shader variants**
- 6 alpha test modes × 10 blend modes = **60 alpha pipeline variants**

**Mitigation**: actually most stages beyond stage 0 are unused (AZMCO is a 2001 game, rarely used multi-texturing). Profile first, then build only what's needed. Expected final count: **8-16 vertex shaders, 16-32 pixel shaders** for full coverage.

## What this means for Phase 2 (skeleton)

The DX11 skeleton needs:
- Define the same 150 entry points with `extern "C"` exports (same names as MSVC.def)
- Map each to a stub that logs and returns success/failure
- Implement `AcquireDescriptor` returning a valid `RendererModuleDescriptor`
- Implement `CreateGameWindow` to set up DX11 device + swap chain
- Implement `DestroyGameWindow` to tear down
- Skip all draw entry points for now

That's ~150 lines of stub code + 100 lines of DX11 device setup.

## What this means for Phase 4 (fixed-function replacement)

Need to build:
- 1 generic vertex shader (pass-through with light + transform support)
- ~8-16 pixel shader variants covering common stage/blend combos
- 8-16 blend state objects pre-created and reused
- 8-16 depth/stencil state objects
- 8-16 rasterizer state objects (cull + fill modes)

All keyed off `(shade_mode, light_count, stage_blend_mode, alpha_test_mode)` tuple.

## Status

- ✅ File read end-to-end
- ✅ State catalog built (above)
- ✅ DX11 mapping produced
- 🔜 Live shader strategy: profile which combos are actually used during a typical MCO frame
- 🔜 Begin Phase 2 skeleton

**Next**: copy `R.DirectX.8.0.A/` to `R.DirectX.11.0.A/`, replace D3D8 with D3D11 stubs, build, ship PR #2.