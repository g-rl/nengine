dependencies = {
	basePath = "./deps"
}

function dependencies.load()
	dir = path.join(dependencies.basePath, "premake/*.lua")
	deps = os.matchfiles(dir)

	for i, dep in pairs(deps) do
		dep = dep:gsub(".lua", "")
		require(dep)
	end
end

function dependencies.imports()
	for i, proj in pairs(dependencies) do
		if type(i) == 'number' then
			proj.import()
		end
	end
end

function dependencies.projects()
	for i, proj in pairs(dependencies) do
		if type(i) == 'number' then
			proj.project()
		end
	end
end

newoption {
	trigger = "copy-to",
	description = "Optional, copy the EXE to a custom folder after build, define the path here if wanted.",
	value = "PATH"
}

newoption {
	trigger = "copy-to-mw2",
	description = "Optional, copy the EXE to the MW2 folder after build, define the path here if wanted.",
	value = "PATH"
}

dependencies.load()

workspace "iw8-mod"
    startproject "iw8-mod"
    location "./build"
    objdir "%{wks.location}/obj/%{cfg.platform}/%{cfg.buildcfg}"
    targetdir "%{wks.location}/bin/%{cfg.platform}/%{cfg.buildcfg}"

    configurations {"Debug", "Release"}

    language "C++"
    cppdialect "C++latest"

    architecture "x64"
    platforms "x64"

    systemversion "latest"
    symbols "On"
    staticruntime "On"
    editandcontinue "Off"
    warnings "Extra"
    characterset "ASCII"

    flags {"NoIncrementalLink", "NoMinimalRebuild", "MultiProcessorCompile", "No64BitChecks"}

    filter "platforms:x64"
	    defines {"_WINDOWS", "WIN32"}
    filter {}

    filter "configurations:Release"
	    optimize "Size"
	    buildoptions {"/GL"}
	    linkoptions {"/IGNORE:4702", "/LTCG"}
	    defines {"NDEBUG"}
	    flags {"FatalCompileWarnings"}
    filter {}

    filter "configurations:Debug"
	    optimize "Debug"
	    defines {"DEBUG", "_DEBUG"}
    filter {}

    project "XInput9_1_0"
        kind "SharedLib"
        language "C++"

        files {"./src/**.h", "./src/**.hpp", "./src/**.cpp"}

        includedirs 
		{
			"%{prj.location}/src",
			"./src",
		}

        resincludedirs {"$(ProjectDir)src"}

        pchheader "std_include.hpp"
        pchsource "src/std_include.cpp"

        dependencies.imports()

        if _OPTIONS["copy-to"] then
            postbuildcommands {"copy /y \"$(TargetPath)\" \"" .. _OPTIONS["copy-to"] .. "\""}
        end

        if _OPTIONS["copy-to-mw2"] then
            postbuildcommands {"copy /y \"$(TargetPath)\" \"" .. _OPTIONS["copy-to-mw2"] .. "\\discord_game_sdk.dll\""}
        end

    group "Dependencies"
    dependencies.projects()
