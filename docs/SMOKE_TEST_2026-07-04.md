# DX11 Backend Smoke Test (2026-07-04)

**Status**: Builds clean, exports correct, mcity crashes during init

## What was fixed

1. **Export name decoration** — MinGW gcc linker with .def as direct input
   preserves the `_THRASH_xxx@N` stdcall decorations that mcity looks for.
   MSVC link.exe cannot do this (strips @N from def LHS).

2. **Function signatures** — All entry points now use the correct argument
   types per the upstream DX8 source (not my placeholders). For example:
   - `CreateGameWindow(u32 width, u32 height, u32 format, u32 options)` — 4 u32 args
   - `Init(void)` — no args (was wrongly `Init(void*,void*,void*)`)
   - `DestroyGameWindow(u32 indx)` — single u32 arg
   - `SyncGameWindow(u32 type)` — single u32 arg

3. **Descriptor fields** — All match the original EA dx8z.dll:
   - `Name = "DX8 3rash"`
   - `SubType = 0` (RENDERER_MODULE_VERSION_104)
   - `DXV = 8` (DirectX 8)
   - `Author = "Mike Ockenden, Thursday 05:09PM Aug 30, 2001"`
   - `DeviceName = ""` (empty)
   - `Size = 208`, `Version = 115`, `Signature = 0x44334438`

4. **DllMain** — Entry point function renamed from `Main` to `DllMain`
   so Windows actually calls DLL_PROCESS_ATTACH.

## What's still broken

mcity.exe crashes at `read attempted @0x0000000C` (NULL pointer + 0xC offset).
The crash happens immediately after `_THRASH_init@0` returns.

### Why

mcity's call sequence after loading dx8z.dll:
1. `AcquireDescriptor()` → returns pointer to descriptor struct
2. `Init()` → mcity expects a state pointer / device count
3. mcity reads `return_value + 0xC` as if it were a function pointer
4. If our `Init` returns 0 (NULL), `0 + 0xC = 0xC` → access violation
5. If our `Init` returns 1, `1 + 0xC = 0xD` (also crashes)

The crash always hits 0xC because mcity derefs `state + 0xC` (probably a vtable or function pointer table).

### Fix needed

The `Init` function must set up the renderer state struct that mcity expects
to read. The upstream's `Init` does this by:
1. Creating Direct3D8 instance
2. Acquiring device count + formats
3. Initializing texture state states
4. Calling lambdas (callbacks into mcity)
5. Setting `State.DX.IsInit = TRUE`
6. Returning `State.Devices.Count`

For our DX11 backend to work, we need to either:
1. **Implement real DX11 init** that returns a valid state pointer
2. **Stub it but return a fake state pointer** that has a valid vtable at offset 0xC
3. **Find the right return value** that mcity handles gracefully

The cleanest fix is Phase 4 work: implement the real state struct with all
the function pointers mcity expects to call through it.

## Files

- `~/projects/azmco-dev/build/mingw32-release-dx11/dx11.dll` — MinGW build (50 KB)
- `~/projects/azmco-dev/scripts/build_dx11.sh` — Updated to pass .def directly to gcc
- `~/projects/azmco-dev/upstream/Source/R.DirectX.11.0.A/Module.cxx` — Updated signatures
- `~/projects/azmco-dev/upstream/Source/R.DirectX.11.0.A/Main.cxx` — DllMain rename
- MCO install: `C:\Program Files (x86)\EA Games\Motor City Online\dx8z.dll`
- Original EA preserved: `~/Backups/Motor City Online backup 20260630-222817/dx8z.preserved.original.dll`

## Recovery

If MCO is broken, restore the original:
```
copy /Y "C:\Users\Dadud\Backups\Motor City Online backup 20260630-222817\dx8z.preserved.original.dll" "C:\Program Files (x86)\EA Games\Motor City Online\dx8z.dll"
```
