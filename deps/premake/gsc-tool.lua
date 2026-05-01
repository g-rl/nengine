gsc_tool = {
    source = path.join(dependencies.basePath, "gsc-tool")
}

function gsc_tool.import()
    links {"xsk-gsc-iw8", "xsk-gsc-s4", "xsk-gsc-iw9", "xsk-gsc-utils"}
    gsc_tool.includes()
end

function gsc_tool.includes()
    includedirs {
        path.join(gsc_tool.source, "include")
    }
end

function gsc_tool.project()
    project "xsk-gsc-utils"
    kind "StaticLib"
    language "C++"
    warnings "Off"

    files {
        path.join(gsc_tool.source, "include/xsk/utils/*.hpp"), 
        path.join(gsc_tool.source, "src/utils/*.cpp")
    }

    includedirs {
        path.join(gsc_tool.source, "include")
    }

    zlib.includes()

    project "xsk-gsc-iw8"
        kind "StaticLib"
        language "C++"
        warnings "Off"

        filter "action:vs*"
            buildoptions "/Zc:__cplusplus"
        filter {}

        files {
            path.join(gsc_tool.source, "include/xsk/stdinc.hpp"),
    
            path.join(gsc_tool.source, "include/xsk/gsc/engine/iw8.hpp"),
            path.join(gsc_tool.source, "src/gsc/engine/iw8.cpp"),

            path.join(gsc_tool.source, "src/gsc/engine/iw8_code.cpp"),
            path.join(gsc_tool.source, "src/gsc/engine/iw8_func.cpp"),
            path.join(gsc_tool.source, "src/gsc/engine/iw8_meth.cpp"),
            path.join(gsc_tool.source, "src/gsc/engine/iw8_token.cpp"),
            path.join(gsc_tool.source, "src/gsc/*.cpp"),

            path.join(gsc_tool.source, "src/gsc/common/*.cpp"),
            path.join(gsc_tool.source, "include/xsk/gsc/common/*.hpp")
        }

        includedirs {
            path.join(gsc_tool.source, "include")
        }

     project "xsk-gsc-s4"
        kind "StaticLib"
        language "C++"
        warnings "Off"

        filter "action:vs*"
            buildoptions "/Zc:__cplusplus"
        filter {}

        files {
            path.join(gsc_tool.source, "include/xsk/stdinc.hpp"),
    
            path.join(gsc_tool.source, "include/xsk/gsc/engine/s4.hpp"),
            path.join(gsc_tool.source, "src/gsc/engine/s4.cpp"),

            path.join(gsc_tool.source, "src/gsc/engine/s4_code.cpp"),
            path.join(gsc_tool.source, "src/gsc/engine/s4_func.cpp"),
            path.join(gsc_tool.source, "src/gsc/engine/s4_meth.cpp"),
            path.join(gsc_tool.source, "src/gsc/engine/s4_token.cpp"),
            path.join(gsc_tool.source, "src/gsc/*.cpp"),

            path.join(gsc_tool.source, "src/gsc/common/*.cpp"),
            path.join(gsc_tool.source, "include/xsk/gsc/common/*.hpp")
        }

        includedirs {
            path.join(gsc_tool.source, "include")
        }

    project "xsk-gsc-iw9"
        kind "StaticLib"
        language "C++"
        warnings "Off"

        filter "action:vs*"
            buildoptions "/Zc:__cplusplus"
        filter {}

        files {
            path.join(gsc_tool.source, "include/xsk/stdinc.hpp"),
 
            path.join(gsc_tool.source, "include/xsk/gsc/engine/iw9.hpp"),
            path.join(gsc_tool.source, "src/gsc/engine/iw9.cpp"),

            path.join(gsc_tool.source, "src/gsc/engine/iw9_code.cpp"),
            path.join(gsc_tool.source, "src/gsc/engine/iw9_func.cpp"),
            path.join(gsc_tool.source, "src/gsc/engine/iw9_hash.cpp"),
            path.join(gsc_tool.source, "src/gsc/engine/iw9_meth.cpp"),
            path.join(gsc_tool.source, "src/gsc/engine/iw9_path.cpp"),
            path.join(gsc_tool.source, "src/gsc/*.cpp"),

            path.join(gsc_tool.source, "src/gsc/common/*.cpp"),
            path.join(gsc_tool.source, "include/xsk/gsc/common/*.hpp")
        }

        includedirs {
            path.join(gsc_tool.source, "include")
        }
end

table.insert(dependencies, gsc_tool)
