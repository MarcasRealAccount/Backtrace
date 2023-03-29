local pkg       = premake.extensions.pkg
local scriptDir = common:scriptDir()

local buildTool = pkg:setupPremake("Backtrace", "amd64", { "Debug", "Release", "Dist" }, scriptDir, "build/", "%{targetpath}/%{config}")
buildTool:mapConfigs({
	Debug = {
		config  = "Debug",
		targets = { Backtrace = { path = "Backtrace/", outputFiles = common:libName("Backtrace", true) } }
	},
	Release = {
		config  = "Release",
		targets = { Backtrace = { path = "Backtrace/", outputFiles = common:libName("Backtrace", true) } }
	},
	Dist = {
		config  = "Dist",
		targets = { Backtrace = { path = "Backtrace/", outputFiles = common:libName("Backtrace", false) } }
	}
})
buildTool:build()
buildTool:cleanTemp()