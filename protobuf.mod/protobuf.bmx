SuperStrict

Rem
bbdoc: Pinned Protocol Buffers runtime used by Nakama.SDK.
End Rem
Module Nakama.Protobuf

ModuleInfo "Version: 3.21.12"
ModuleInfo "License: BSD-3-Clause"
ModuleInfo "Copyright: Google LLC"
ModuleInfo "CPP_OPTS: -std=c++17"
ModuleInfo "CPP_OPTS: -DGOOGLE_PROTOBUF_CMAKE_BUILD"
ModuleInfo "CPP_OPTS: -I%PWD%/vendor/protobuf"

Import Archive.zlib
Import "source.bmx"
