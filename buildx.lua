
function chip()
    local conf_data = io.readfile("$(projectdir)/app/port/luat_conf_bsp.h")
    local LUAT_FS_SIZE = tonumber(conf_data:match("#define LUAT_FS_SIZE%s+(%d+)"))
    LUAT_SCRIPT_SIZE = tonumber(conf_data:match("#define LUAT_SCRIPT_SIZE%s+(%d+)"))

    AIR10X_FLASH_FS_REGION_SIZE = LUAT_FS_SIZE + LUAT_SCRIPT_SIZE
    -- print(AIR10X_FLASH_FS_REGION_SIZE,LUAT_FS_SIZE , LUAT_SCRIPT_SIZE)
    AIR10X_VERSION = conf_data:match("#define LUAT_BSP_VERSION \"(%w+)\"")
    local LVGL_CONF = conf_data:find("\r#define LUAT_USE_LVGL") or conf_data:find("\n#define LUAT_USE_LVGL")
    
    VM_64BIT = conf_data:find("\r#define LUAT_CONF_VM_64bit") or conf_data:find("\n#define LUAT_CONF_VM_64bit")
    local custom_data = io.readfile("$(projectdir)/app/port/luat_conf_bsp.h")
    local FDB_CONF = conf_data:find("\r#define LUAT_USE_FDB") or conf_data:find("\n#define LUAT_USE_FDB") or conf_data:find("\r#define LUAT_USE_FSKV") or conf_data:find("\n#define LUAT_USE_FSKV") 
    local FOTA_CONF = conf_data:find("\r#define LUAT_USE_FOTA") or conf_data:find("\n#define LUAT_USE_FOTA")

    -- 根据型号判断flash大小
    local is_air101 = custom_data:find("\r#define AIR101") or custom_data:find("\n#define AIR101")
    local is_air103 = custom_data:find("\r#define AIR103") or custom_data:find("\n#define AIR103")
    local is_air601 = custom_data:find("\r#define AIR601") or custom_data:find("\n#define AIR601")
    local is_air690 = custom_data:find("\r#define AIR690") or custom_data:find("\n#define AIR690")

    local flash_size = 1*1024*1024
    if is_air101 or is_air690 then 
        flash_size = 2*1024*1024
    end

    -- 然后, 根据flash大小, 计算flash的分区
    local flash_fs_size = LUAT_FS_SIZE * 1024                  -- 这个直接取宏定义的值
    local flash_script_size = LUAT_SCRIPT_SIZE * 1024          -- 这个直接取宏定义的值
    local flash_kv_size = FDB_CONF and 64*1024 or 0            -- 如果是FDB, 则需要预留64k
    local flash_fota_size = 4 * 1024
    -- APP区域大小 = 剩余flash大小 - fs分区大小 - script分区大小 - kv分区大小 - secboot区域大小 - 末尾分区64k - OTA区域大小(按比例)
    local flash_app_size = flash_size - (flash_fs_size + flash_script_size + flash_kv_size) - 16*1024 - 64*1024
    if FOTA_CONF then
        flash_fota_size = (flash_app_size + flash_script_size // 4 * 3) // 2 // 5 * 4
        flash_fota_size = flash_fota_size // 4096
        flash_fota_size = flash_fota_size * 4096
    end
    flash_app_size = flash_app_size - flash_fota_size - 1024 -- 末尾还需要加1k的image head
    local flash_app_offset = 64*1024 + flash_fota_size + 0x08000000 + 1024

    local ISRAM = string.format("I-SRAM : ORIGIN = 0x%x , LENGTH = 0x%x", flash_app_offset, flash_app_size)

    print("文件系统大小", flash_fs_size // 1024, "kb")
    print("脚本区大小", flash_script_size // 1024, "kb")
    print("KV区大小", flash_kv_size // 1024, "kb")
    print("底层固件区大小", flash_app_size // 1024, "kb")
    print("FOTA区大小", flash_fota_size // 1024, "kb")
    print("ISRAM", ISRAM)

    if is_air101 then TARGET_NAME = "AIR101"
    elseif is_air103 then TARGET_NAME = "AIR103"
    elseif is_air601 then TARGET_NAME = "AIR601"
    elseif is_air690 then TARGET_NAME = "AIR690"
    else TARGET_NAME = "AIR10X" end

    local result = {}

    result.target_name = TARGET_NAME
    result.flash_size = flash_size
    result.flash_fs_size = flash_fs_size
    result.flash_script_size = flash_script_size
    result.flash_kv_size = flash_kv_size
    result.flash_app_size = flash_app_size
    result.flash_app_offset = flash_app_offset
    result.flash_fota_size = flash_fota_size
    result.flash_app_offset = flash_app_offset

    result.ISRAM = ISRAM
    result.is_air101 = is_air101
    result.is_air103 = is_air103
    result.is_air601 = is_air601
    result.is_air690 = is_air690
    result.use_lvgl = LVGL_CONF
    result.use_fota = FOTA_CONF
    result.use_fdb = FDB_CONF
    result.bsp_version = AIR10X_VERSION
    result.use_64bit = VM_64BIT

    return result
end
