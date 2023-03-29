workspace("Backtrace")
	location("build/")
	common:setConfigsAndPlatforms()
	common:addCoreDefines()

	cppdialect("C++20")
	rtti("Off")
	exceptionhandling("Off")
	flags("MultiProcessorCompile")

	project("Backtrace")
		location("%{wks.location}/")
		warnings("Extra")

		kind("StaticLib")
		common:outDirs(true)

		includedirs({ "Inc/" })
		files({
			"Inc/**",
			"Src/**"
		})
		removefiles({ "*.DS_Store" })

		pkgdeps({ "commonbuild" })

		common:addActions()