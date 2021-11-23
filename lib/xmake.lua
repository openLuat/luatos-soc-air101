set_project("AIR101")
set_xmakever("2.5.8")

set_version("0.0.2", {build = "%Y%m%d%H%M"})
add_rules("mode.debug", "mode.release")

local sdkdir = "../"

local sdk_dir = "D:\\csky-elfabiv2-tools-mingw-minilibc\\"
if is_plat("linux") then
    sdk_dir = "/opt/csky-elfabiv2-tools/"
elseif is_plat("windows") then
    sdk_dir = "E:\\csky-elfabiv2-tools-mingw-minilibc\\"
end

toolchain("csky_toolchain")
    set_kind("standalone")
    set_sdkdir(sdk_dir)
toolchain_end()

set_toolchains("csky_toolchain")
--add macro defination
add_defines("GCC_COMPILE=1","TLS_CONFIG_CPU_XT804=1","NIMBLE_FTR=1","USE_LUATOS")
-- set warning all as error
set_warnings("all")
set_optimize("smallest")
-- set language: c99
set_languages("c99", "cxx11")
add_asflags("-DTLS_CONFIG_CPU_XT804=1 -DGCC_COMPILE=1 -mcpu=ck804ef -std=gnu99 -c -mhard-float -Wa,--gdwarf2 -fdata-sections -ffunction-sections")
add_cflags("-DTLS_CONFIG_CPU_XT804=1 -DGCC_COMPILE=1 -mcpu=ck804ef -std=gnu99 -c -mhard-float -Wall -fdata-sections -ffunction-sections")
add_cxflags("-DTLS_CONFIG_CPU_XT804=1 -DGCC_COMPILE=1 -mcpu=ck804ef -std=gnu99 -c -mhard-float -Wall -fdata-sections -ffunction-sections")

set_dependir("$(buildir)/.deps")
set_objectdir("$(buildir)/.objs")
set_targetdir("$(projectdir)")

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

target("wmcommon")
    set_kind("static")
    set_plat("cross")
    set_arch("c-sky")
    
    add_files(sdkdir.."platform/common/**.c")
    add_includedirs(sdkdir.."platform/common/params",{public = true})
    add_includedirs(sdkdir.."platform/inc",{public = true})
    add_includedirs(sdkdir.."src/app/mbedtls/include",{public = true})
    add_includedirs(sdkdir.."include",{public = true})
    add_includedirs(sdkdir.."include/platform",{public = true})
    add_includedirs(sdkdir.."include/driver",{public = true})
    add_includedirs(sdkdir.."include/os",{public = true})
    add_includedirs(sdkdir.."include/arch/xt804",{public = true})
    add_includedirs(sdkdir.."include/arch/xt804/csi_core",{public = true})
    add_includedirs(sdkdir.."src/os/rtos/include",{public = true})
    add_includedirs(sdkdir.."include/wifi",{public = true})

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

target("blehost")
    set_kind("static")
    set_plat("cross")
    set_arch("c-sky")
    
    add_files(sdkdir.."src/bt/blehost/**.c")
    add_includedirs(os.dirs(path.join(os.scriptdir(),sdkdir.."src/bt/blehost/**")))
    add_includedirs(sdkdir.."src/app/bleapp",{public = true})
    add_includedirs(sdkdir.."src/os/rtos/include",{public = true})
    add_includedirs(sdkdir.."include",{public = true})
    add_includedirs(sdkdir.."include/bt",{public = true})
    add_includedirs(sdkdir.."include/platform",{public = true})
    add_includedirs(sdkdir.."include/os",{public = true})
    add_includedirs(sdkdir.."include/arch/xt804",{public = true})
    add_includedirs(sdkdir.."include/arch/xt804/csi_core",{public = true})

target_end()

target("os")
    set_kind("static")
    set_plat("cross")
    set_arch("c-sky")
    
    add_files(sdkdir.."src/os/**.c")
    add_files(sdkdir.."src/os/**.S")
    add_includedirs(sdkdir.."src/os/rtos/include",{public = true})
    add_includedirs(sdkdir.."include",{public = true})
    add_includedirs(sdkdir.."include/platform",{public = true})
    add_includedirs(sdkdir.."include/os",{public = true})
    add_includedirs(sdkdir.."include/arch/xt804",{public = true})
    add_includedirs(sdkdir.."include/arch/xt804/csi_core",{public = true})

target_end()

target("drivers")
    set_kind("static")
    set_plat("cross")
    set_arch("c-sky")
    
    add_files(sdkdir.."platform/drivers/**.c")
    add_includedirs(sdkdir.."platform/inc",{public = true})
    add_includedirs(sdkdir.."include",{public = true})
    add_includedirs(sdkdir.."include/app",{public = true})
    add_includedirs(sdkdir.."include/platform",{public = true})
    add_includedirs(sdkdir.."include/driver",{public = true})
    add_includedirs(sdkdir.."include/os",{public = true})
    add_includedirs(sdkdir.."include/arch/xt804",{public = true})
    add_includedirs(sdkdir.."include/arch/xt804/csi_core",{public = true})
    add_includedirs(sdkdir.."include/net",{public = true})
    add_includedirs(sdkdir.."include/wifi",{public = true})
    add_includedirs(sdkdir.."demo",{public = true})

target_end()

target("wmsys")
    set_kind("static")
    set_plat("cross")
    set_arch("c-sky")
    
    add_files(sdkdir.."platform/sys/*.c")

    add_includedirs(sdkdir.."platform/arch",{public = true})
    add_includedirs(sdkdir.."include",{public = true})
    add_includedirs(sdkdir.."include/driver",{public = true})
    add_includedirs(sdkdir.."include/os",{public = true})
    add_includedirs(sdkdir.."include/platform",{public = true})
    add_includedirs(sdkdir.."include/wifi",{public = true})
    add_includedirs(sdkdir.."include/arch/xt804",{public = true})
    add_includedirs(sdkdir.."include/arch/xt804/csi_core",{public = true})
    add_includedirs(sdkdir.."include/app",{public = true})
    add_includedirs(sdkdir.."include/net",{public = true})
    add_includedirs(sdkdir.."demo",{public = true})
    add_includedirs(sdkdir.."platform/inc",{public = true})

target_end()
