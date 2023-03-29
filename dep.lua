libdirs({ common:scriptDir() .. string.format("/Bin/%s-%s-", common.host, common.arch) .. "%{cfg.buildcfg}" })
links({ "Backtrace" })
externalincludedirs({ common:scriptDir() .. "/Inc/" })