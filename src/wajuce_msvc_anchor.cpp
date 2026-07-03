// Anchor translation unit for the Windows (MSVC) build of the wajuce shared
// library.
//
// The FFI entry points (wajuce_*) are defined in the WAIPlugEngine static
// library and marked __declspec(dllexport); they are pulled into — and
// re-exported from — this DLL via /WHOLEARCHIVE (see CMakeLists.txt).
//
// MSVC will not emit a DLL for a target that has no compiled source of its own.
// On Apple/Linux the target is header-only and the dylib/.so is produced by
// -force_load / --whole-archive, but link.exe needs at least one object file to
// anchor the DLL. This otherwise-empty unit exists solely to provide it.
