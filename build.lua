local pkg       = premake.extensions.pkg
local scriptDir = pkg:scriptDir()
local premake   = pkg.builders.premake

local buildTool = premake:setup("Backtrace", "amd64", { "Debug", "Release", "Dist" }, scriptDir, "build/", "%{targetname}/%{config}")
buildTool:mapConfigs({
	Debug = {
		config  = "Debug",
		targets = { Backtrace = { path = "Backtrace/", outputFiles = pkg:libName("Backtrace", true) } }
	},
	Release = {
		config  = "Release",
		targets = { Backtrace = { path = "Backtrace/", outputFiles = pkg:libName("Backtrace", true) } }
	},
	Dist = {
		config  = "Dist",
		targets = { Backtrace = { path = "Backtrace/", outputFiles = pkg:libName("Backtrace", false) } }
	}
})
buildTool:build()
buildTool:cleanTemp()