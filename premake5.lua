workspace("Backtrace")
	location("build/")
	common:addConfigs()
	common:addBuildDefines()

	cppdialect("C++20")
	rtti("Off")
	exceptionhandling("On")
	flags("MultiProcessorCompile")

	project("Backtrace")
		location("%{wks.location}/")
		warnings("Extra")

		kind("StaticLib")
		targetdir("%{wks.location}/Backtrace/%{cfg.buildcfg}")
		objdir("%{wks.location}/Backtrace/%{cfg.buildcfg}")

		includedirs({ "Inc/" })
		files({
			"Inc/**",
			"Src/**"
		})
		removefiles({ "*.DS_Store" })

		filter("system:windows")
			links({ "Dbghelp.lib" })
		filter({})

		pkgdeps({ "commonbuild" })

		common:addActions()