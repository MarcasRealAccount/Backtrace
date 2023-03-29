workspace("Backtrace")
	location("build/")
	common:setConfigsAndPlatforms()
	common:addCoreDefines()

	cppdialect("C++20")
	rtti("Off")
	exceptionhandling("On")
	flags("MultiProcessorCompile")

	project("Backtrace")
		location("%{wks.location}/")
		warnings("Extra")

		kind("StaticLib")
		common:outDirs()

		includedirs({ "Inc/" })
		files({
			"Inc/**",
			"Src/**"
		})
		removefiles({ "*.DS_Store" })

		pkgdeps({ "commonbuild" })

		common:addActions()