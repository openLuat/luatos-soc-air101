set_project("AIR101")
set_xmakever("2.5.8")

-- set_version("0.0.2", {build = "%Y%m%d%H%M"})
add_rules("mode.debug", "mode.release")

local AIR10X_VERSION
local luatos = "../LuatOS/"
local TARGET_NAME 

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

set_warnings("all")
set_optimize("smallest")
-- set language: c99
set_languages("c99", "cxx11")
add_asflags("-DTLS_CONFIG_CPU_XT804=1 -DGCC_COMPILE=1 -mcpu=ck804ef -std=gnu99 -c -mhard-float -Wa,--gdwarf2 -fdata-sections -ffunction-sections")
add_cflags("-DTLS_CONFIG_CPU_XT804=1 -DGCC_COMPILE=1 -mcpu=ck804ef -std=gnu99 -c -mhard-float -Wall -fdata-sections -ffunction-sections")
add_cxflags("-DTLS_CONFIG_CPU_XT804=1 -DGCC_COMPILE=1 -mcpu=ck804ef -std=gnu99 -c -mhard-float -Wall -fdata-sections -ffunction-sections")

set_dependir("$(buildir)/.deps")
set_objectdir("$(buildir)/.objs")

target("lvgl")
    set_kind("static")
    set_plat("cross")
    set_arch("c-sky")
    
    on_load(function (target)
        local conf_data = io.readfile("$(projectdir)/app/port/luat_conf_bsp.h")
        local LVGL_CONF = conf_data:find("// #define LUAT_USE_LVGL\n")
        if LVGL_CONF == nil then target:set("default", true) else target:set("default", false) end
    end)

    add_files(luatos.."components/lvgl/**.c")

    add_includedirs("app/port",{public = true})
    add_includedirs("include",{public = true})
    add_includedirs(luatos.."lua/include",{public = true})
    add_includedirs(luatos.."luat/include",{public = true})
    add_includedirs(luatos.."components/lcd",{public = true})
    add_includedirs(luatos.."components/lvgl",{public = true})
    add_includedirs(luatos.."components/lvgl/binding",{public = true})
    add_includedirs(luatos.."components/lvgl/gen",{public = true})
    add_includedirs(luatos.."components/lvgl/src",{public = true})
    add_includedirs(luatos.."components/lvgl/font",{public = true})
    add_includedirs(luatos.."luat/packages/u8g2")
    
    set_targetdir("$(buildir)/lib")
target_end()

target("air10x")
    -- set kind
    set_kind("binary")
    set_plat("cross")
    set_arch("c-sky")
    set_targetdir("$(buildir)/out")
    on_load(function (target)
        local conf_data = io.readfile("$(projectdir)/app/port/luat_conf_bsp.h")
        AIR10X_VERSION = conf_data:match("#define LUAT_BSP_VERSION \"(%w+)\"")
        local LVGL_CONF = conf_data:find("// #define LUAT_USE_LVGL\n")
        if LVGL_CONF == nil then target:add("deps", "lvgl") end
        local custom_data = io.readfile("$(projectdir)/app/port/luat_conf_bsp.h")
        local TARGET_CONF = custom_data:find("#define AIR101")
        if TARGET_CONF == nil then TARGET_NAME = "AIR103" else TARGET_NAME = "AIR101" end
        target:add("defines", TARGET_NAME)
        target:set("filename", TARGET_NAME..".elf")
        target:add("ldflags", "-Wl,--gc-sections -Wl,-zmax-page-size=1024 -Wl,--whole-archive ./lib/libwmsys.a ./lib/libdrivers.a ./lib/libos.a ./lib/libblehost.a ./lib/libwmarch.a ./lib/libwmcommon.a ./lib/libapp.a ./lib/libgt.a ./lib/libwlan.a ./lib/libdsp.a ./lib/libbtcontroller.a -Wl,--no-whole-archive -mcpu=ck804ef -nostartfiles -mhard-float -lm -Wl,-T./ld/"..TARGET_NAME..".ld -Wl,-ckmap=./build/out/"..TARGET_NAME..".map ",{force = true})
    end)

    -- add files
    add_files("app/*.c")
    add_files("app/port/*.c")
    add_files("app/custom/*.c")

    add_files(luatos.."lua/src/*.c")
    add_files(luatos.."luat/modules/*.c")
    add_files(luatos.."luat/vfs/*.c")
    del_files(luatos.."luat/vfs/luat_fs_posix.c")

    add_files(luatos.."components/lcd/*.c")
    add_files(luatos.."components/sfd/*.c")
    add_files(luatos.."components/nr_micro_shell/*.c")
    add_files(luatos.."luat/packages/eink/*.c")
    add_files(luatos.."luat/packages/epaper/*.c")
    del_files(luatos.."luat/packages/epaper/GUI_Paint.c")
    add_files(luatos.."luat/packages/iconv/*.c")
    add_files(luatos.."luat/packages/lfs/*.c")
    add_files(luatos.."luat/packages/lua-cjson/*.c")
    add_files(luatos.."luat/packages/minmea/*.c")
    add_files(luatos.."luat/packages/qrcode/*.c")
    add_files(luatos.."luat/packages/u8g2/*.c")
    add_files(luatos.."luat/weak/*.c")

    add_files(luatos.."components/sfud/*.c")
    add_includedirs(luatos.."components/sfud")

    add_files(luatos.."components/statem/*.c")
    add_includedirs(luatos.."components/statem")

    add_includedirs("demo")
    add_includedirs("demo/console")
    add_includedirs("include/app")
    add_includedirs("include/arch/xt804")
    add_includedirs("include/arch/xt804/csi_core")
    add_includedirs("include/arch/xt804/csi_dsp")
    add_includedirs("include/bt")
    add_includedirs("include/driver")
    add_includedirs("include/net")
    add_includedirs("include/os")
    add_includedirs("include/platform")
    add_includedirs("include/wifi")
    add_includedirs("platform/common/params")
    add_includedirs("platform/inc")
    add_includedirs("platform/sys")

    add_includedirs("src/os/rtos/include")
    add_includedirs("src/app/mbedtls/include")
    add_includedirs("src/app/mbedtls/ports")
    add_includedirs("src/app/fatfs")

    add_includedirs("app/port",{public = true})
    add_includedirs("include",{public = true})
    add_includedirs(luatos.."lua/include",{public = true})
    add_includedirs(luatos.."luat/include",{public = true})
    add_includedirs(luatos.."components/lcd",{public = true})
    add_includedirs(luatos.."components/lvgl",{public = true})
    add_includedirs(luatos.."components/lvgl/binding",{public = true})
    add_includedirs(luatos.."components/lvgl/gen",{public = true})
    add_includedirs(luatos.."components/lvgl/src",{public = true})
    add_includedirs(luatos.."components/lvgl/font",{public = true})
    
    add_includedirs(luatos.."luat/packages/eink")
    add_includedirs(luatos.."luat/packages/epaper")
    add_includedirs(luatos.."luat/packages/iconv")
    add_includedirs(luatos.."luat/packages/lfs")
    add_includedirs(luatos.."luat/packages/lua-cjson")
    add_includedirs(luatos.."luat/packages/minmea")
    add_includedirs(luatos.."luat/packages/qrcode")
    add_includedirs(luatos.."luat/packages/u8g2")

    -- gtfont
    add_includedirs(luatos.."components/gtfont",{public = true})
    add_files(luatos.."components/gtfont/*.c")

    -- ble
    add_includedirs("src/app/bleapp", "src/bt/blehost/nimble/include", "src/bt/blehost/nimble/host/include", "include/bt")
    add_includedirs("src/bt/blehost/nimble/transport/uart/include", "src/bt/blehost/porting/xt804/include")
    add_includedirs("src/bt/blehost/ext/tinycrypt/include", "src/bt/blehost/nimble/host/util/include")

    add_includedirs(luatos.."components/nr_micro_shell")

	after_build(function(target)
        os.exec(sdk_dir .. "bin/csky-elfabiv2-objcopy -O binary $(buildir)/out/"..TARGET_NAME..".elf $(buildir)/out/"..TARGET_NAME..".bin")
        io.writefile("$(buildir)/out/"..TARGET_NAME..".asm", os.iorun(sdk_dir .. "bin/csky-elfabiv2-objdump -d $(buildir)/out/"..TARGET_NAME..".elf"))
        io.writefile("$(buildir)/out/"..TARGET_NAME..".list", os.iorun(sdk_dir .. "bin/csky-elfabiv2-objdump -h -S $(buildir)/out/"..TARGET_NAME..".elf"))
        io.writefile("$(buildir)/out/"..TARGET_NAME..".size", os.iorun(sdk_dir .. "bin/csky-elfabiv2-size $(buildir)/out/"..TARGET_NAME..".elf"))
        io.cat("$(buildir)/out/"..TARGET_NAME..".size")
        if TARGET_NAME == "AIR101" then
            os.exec("./tools/xt804/wm_tool"..(is_plat("windows") and ".exe" or "").." -b $(buildir)/out/"..TARGET_NAME..".bin -fc 0 -it 1 -ih 8020000 -ra 8020400 -ua 8010000 -nh 0 -un 0 -vs G01.00.02 -o $(buildir)/out/"..TARGET_NAME.."")
            os.exec("./tools/xt804/wm_tool"..(is_plat("windows") and ".exe" or "").." -b  ./tools/xt804/xt804_secboot.bin -fc 0 -it 0 -ih 8002000 -ra 8002400 -ua 8010000 -nh 8020000 -un 0 -o ./tools/xt804/"..TARGET_NAME.."_secboot")
        elseif TARGET_NAME == "AIR103" then
            os.exec("./tools/xt804/wm_tool"..(is_plat("windows") and ".exe" or "").." -b $(buildir)/out/"..TARGET_NAME..".bin -fc 0 -it 1 -ih 8020000 -ra 8020400 -ua 8010000 -nh 0 -un 0 -vs G01.00.02 -o $(buildir)/out/"..TARGET_NAME.."")
            os.exec("./tools/xt804/wm_tool"..(is_plat("windows") and ".exe" or "").." -b  ./tools/xt804/xt804_secboot.bin -fc 0 -it 0 -ih 8002000 -ra 8002400 -ua 8010000 -nh 8020000 -un 0 -o ./tools/xt804/"..TARGET_NAME.."_secboot")
        end
        os.cp("./tools/xt804/"..TARGET_NAME.."_secboot.img", "$(buildir)/out/"..TARGET_NAME..".fls")
        local img = io.readfile("$(buildir)/out/"..TARGET_NAME..".img", {encoding = "binary"})
        local fls = io.open("$(buildir)/out/"..TARGET_NAME..".fls", "a+")
        if fls then fls:write(img) fls:close() end
        if is_mode("debug") then
            os.cd("./mklfs")
            os.exec("./luac_536_32bits.exe -s -o ./disk/main.luac ../user/demo/gpio/main.lua")
            os.exec("./mklfs.exe")
            os.cd("../")
            os.mv("./mklfs/script.bin", "$(buildir)/out/script.bin")
            os.exec("./tools/xt804/wm_tool"..(is_plat("windows") and ".exe" or "").." -b  $(buildir)/out/script.bin -it 1 -fc 0 -ih 20008000 -ra 81E0000 -ua 0 -nh 0  -un 0 -o $(buildir)/out/script")
            local script = io.readfile("$(buildir)/out/script.img", {encoding = "binary"})
            local fls = io.open("$(buildir)/out/"..TARGET_NAME..".fls", "a+")
            if fls then fls:write(script) fls:close() end
        elseif is_mode("release") then
            import("lib.detect.find_file")
            os.cp("$(buildir)/out/"..TARGET_NAME..".fls", "./soc_tools/"..TARGET_NAME..".fls")
            local path7z
            if is_plat("windows") then
                path7z = find_file("7z.exe", { "C:/Program Files/7-Zip/", "D:/Program Files/7-Zip/", "E:/Program Files/7-Zip/"})
            elseif is_plat("linux") then
                path7z = find_file("7z", { "/usr/bin/"})
                if not path7z then
                    path7z = find_file("7zr", { "/usr/bin/"})
                end
            end
            if path7z then
                os.cp("./soc_tools/"..TARGET_NAME..".json", "./soc_tools/info.json")
                os.exec(path7z.." a LuatOS-SoC_"..AIR10X_VERSION.."_"..TARGET_NAME..".7z -pLuat!1802s# ./soc_tools/air101_flash.exe ./soc_tools/info.json ./soc_tools/"..TARGET_NAME..".fls")
                os.mv("LuatOS-SoC_"..AIR10X_VERSION.."_"..TARGET_NAME..".7z", "$(buildir)/out/LuatOS-SoC_"..AIR10X_VERSION.."_"..TARGET_NAME..".soc")
                os.rm("./soc_tools/info.json")
                os.rm("./soc_tools/"..TARGET_NAME..".fls")
            else
                print("7z not find")
                os.rm("./soc_tools/"..TARGET_NAME..".fls")
                return
            end
        end
        -- os.exec("./tools/xt804/wm_tool"..(is_plat("windows") and ".exe" or "").." -b $(buildir)/out/AIR101.img -fc 1 -it 1 -ih 80D0000 -ra 80D0400 -ua 8010000 -nh 0 -un 0 -vs G01.00.02 -o $(buildir)/out/AIR101")
        -- os.mv("$(buildir)/out/AIR101_gz.img", "$(buildir)/out/AIR101_ota.img")
    end)
target_end()
