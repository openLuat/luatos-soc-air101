
local sdkdir = "../"

set_targetdir("$(projectdir)/lib")

target("app")
    set_kind("static")
    set_plat("cross")
    set_arch("c-sky")
    
    add_files(sdkdir.."src/app/**.c")
    del_files(sdkdir.."src/app/btapp/**.c")

    add_includedirs(os.dirs(path.join(os.scriptdir(),sdkdir.."src/app/**")))
    add_includedirs(os.dirs(path.join(os.scriptdir(),sdkdir.."src/bt/blehost/**")))

    add_includedirs(sdkdir.."include",{public = true})
    add_includedirs(sdkdir.."include/app",{public = true})
    add_includedirs(sdkdir.."include/driver",{public = true})
    add_includedirs(sdkdir.."include/os",{public = true})
    add_includedirs(sdkdir.."include/bt",{public = true})
    add_includedirs(sdkdir.."include/platform",{public = true})
    add_includedirs(sdkdir.."platform/common/params",{public = true})
    add_includedirs(sdkdir.."include/wifi",{public = true})
    add_includedirs(sdkdir.."include/arch/xt804",{public = true})
    add_includedirs(sdkdir.."include/arch/xt804/csi_core",{public = true})
    add_includedirs(sdkdir.."include/net",{public = true})
    add_includedirs(sdkdir.."demo",{public = true})
    add_includedirs(sdkdir.."platform/inc",{public = true})

target_end()

target("wmarch")
    set_kind("static")
    set_plat("cross")
    set_arch("c-sky")
    
    add_files(sdkdir.."platform/arch/**.c")
    add_files(sdkdir.."platform/arch/**.S")
    add_includedirs(sdkdir.."include",{public = true})
    add_includedirs(sdkdir.."include/driver",{public = true})
    add_includedirs(sdkdir.."include/os",{public = true})
    add_includedirs(sdkdir.."include/arch/xt804",{public = true})
    add_includedirs(sdkdir.."include/arch/xt804/csi_core",{public = true})

    after_load(function (target)
        for _, sourcebatch in pairs(target:sourcebatches()) do
            if sourcebatch.sourcekind == "as" then -- only asm files
                for idx, objectfile in ipairs(sourcebatch.objectfiles) do
                    sourcebatch.objectfiles[idx] = objectfile:gsub("%.S%.o", ".o")
                end
            end
        end
    end)

target_end()
