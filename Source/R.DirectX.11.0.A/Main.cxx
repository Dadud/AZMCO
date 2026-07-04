/*
Copyright (c) 2024 Americus Maximus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY PARTY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "DirectX11.hxx"

// CRITICAL: The entry point MUST be named DllMain (not Main). Windows
// specifically looks for this name when loading a DLL. If named anything
// else, no DLL_PROCESS_ATTACH notifications are sent, runtime init is
// skipped, and tools that hook DllMain (e.g. some EA anti-cheat, TLS init)
// may crash on first call. The original AZMCO source has this same
// bug (function named Main) which is why the upstream build has a
// #pragma comment to set the entry point name.
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
