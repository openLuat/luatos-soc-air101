
-- 解析大小字符串, 支持 "64K"/"1536K"/"0x10000"/纯数字(字节)
local function parse_size(s)
    s = s:match("^%s*(.-)%s*$")
    local num, suffix = s:match("^(%d+)([KkMm]?)$")
    if num then
        num = tonumber(num)
        if suffix == "K" or suffix == "k" then
            return num * 1024
        elseif suffix == "M" or suffix == "m" then
            return num * 1024 * 1024
        else
            return num
        end
    end
    local hex = s:match("^0[xX](%x+)$")
    if hex then
        return tonumber(hex, 16)
    end
    raise("无法解析大小: " .. s)
end

-- 解析偏移量, 支持十六进制 "0x010000" 和十进制
local function parse_offset(s)
    s = s:match("^%s*(.-)%s*$")
    local hex = s:match("^0[xX](%x+)$")
    if hex then
        return tonumber(hex, 16)
    end
    return tonumber(s)
end

-- 解析分区表CSV文件
-- 格式: # 注释行, # key = value 为元数据, 数据行: Name, Type, Offset, Size
-- 返回 { meta = {flash_base, flash_size, ram_end}, partitions = {name={...}}, partition_list = {{...}} }
local function parse_csv(filepath)
    local data = io.readfile(filepath)
    if not data then
        raise("无法读取分区表文件: " .. filepath)
    end

    local meta = {}
    local partitions = {}
    local partition_list = {}

    for line in data:gmatch("[^\r\n]+") do
        line = line:match("^%s*(.-)%s*$")
        if line == "" then
            -- 跳过空行
        elseif line:sub(1, 1) == "#" then
            -- 注释行, 检查是否为元数据 (# key = value)
            local key, value = line:match("^#%s*([%w_]+)%s*=%s*(.+)$")
            if key and value then
                value = value:match("^%s*(.-)%s*$")
                if key == "flash_base" or key == "ram_end" then
                    meta[key] = parse_offset(value)
                elseif key == "flash_size" then
                    meta[key] = parse_size(value)
                end
            end
        else
            -- 数据行: Name, Type, Offset, Size
            local name, ptype, offset_s, size_s = line:match("^([^,]+),([^,]+),([^,]+),([^,]+)$")
            if name then
                name = name:match("^%s*(.-)%s*$")
                ptype = ptype:match("^%s*(.-)%s*$")
                local offset = parse_offset(offset_s)
                local size = parse_size(size_s)
                if not offset then raise("分区 " .. name .. " 偏移量解析失败: " .. offset_s) end
                if not size then raise("分区 " .. name .. " 大小解析失败: " .. size_s) end
                local p = {name = name, type = ptype, offset = offset, size = size}
                partitions[name] = p
                table.insert(partition_list, p)
            end
        end
    end

    -- 校验: 必须存在的元数据
    if not meta.flash_base then raise(filepath .. ": 缺少 flash_base 元数据") end
    if not meta.flash_size then raise(filepath .. ": 缺少 flash_size 元数据") end
    if not meta.ram_end then raise(filepath .. ": 缺少 ram_end 元数据") end

    -- 按offset排序
    table.sort(partition_list, function(a, b) return a.offset < b.offset end)

    -- 校验: 分区不重叠
    for i = 2, #partition_list do
        local prev = partition_list[i - 1]
        local curr = partition_list[i]
        local prev_end = prev.offset + prev.size
        if prev_end > curr.offset then
            raise(string.format("分区重叠: %s (0x%06X+0x%X=0x%06X) 与 %s (0x%06X)",
                prev.name, prev.offset, prev.size, prev_end,
                curr.name, curr.offset))
        end
    end

    -- 校验: 不超出flash范围
    if #partition_list > 0 then
        local last = partition_list[#partition_list]
        local last_end = last.offset + last.size
        if last_end > meta.flash_size then
            raise(string.format("分区 %s 超出flash范围: 0x%06X+0x%X=0x%06X > flash_size=0x%X",
                last.name, last.offset, last.size, last_end, meta.flash_size))
        end
    end

    -- 校验: flash 尾部 24K 为系统保留区, 其中最后 8K 必须是 sysparam 分区
    local reserved_tail_size = 24 * 1024
    local sysparam_size = 8 * 1024
    local reserved_start = meta.flash_size - reserved_tail_size
    local sysparam_offset = meta.flash_size - sysparam_size
    local sysparam = partitions.sysparam

    if not sysparam then
        raise(filepath .. ": 缺少 sysparam 分区, flash 最后8K必须保留给 sysparam")
    end

    if sysparam.offset ~= sysparam_offset or sysparam.size ~= sysparam_size then
        raise(string.format("%s: sysparam 分区必须位于 flash 最后8K (offset=0x%06X, size=0x%X), 当前为 offset=0x%06X, size=0x%X",
            filepath, sysparam_offset, sysparam_size, sysparam.offset, sysparam.size))
    end

    for _, p in ipairs(partition_list) do
        local part_end = p.offset + p.size
        if p.name ~= "sysparam" and part_end > reserved_start then
            raise(string.format("%s: 分区 %s 占用了 flash 尾部保留区, 结束地址 0x%06X 超过保留区起始 0x%06X",
                filepath, p.name, part_end, reserved_start))
        end
    end

    return {meta = meta, partitions = partitions, partition_list = partition_list}
end

function chip()
    local conf_data = io.readfile("$(projectdir)/app/port/luat_conf_bsp.h")

    -- 从 luat_conf_bsp.h 读取BSP版本
    AIR10X_VERSION = conf_data:match("#define LUAT_BSP_VERSION \"(%w+)\"")

    -- 读取功能宏开关
    local LVGL_CONF = conf_data:find("\r#define LUAT_USE_LVGL") or conf_data:find("\n#define LUAT_USE_LVGL")
    VM_64BIT = conf_data:find("\r#define LUAT_CONF_VM_64bit") or conf_data:find("\n#define LUAT_CONF_VM_64bit")
    local FDB_CONF = conf_data:find("\r#define LUAT_USE_FDB") or conf_data:find("\n#define LUAT_USE_FDB") or conf_data:find("\r#define LUAT_USE_FSKV") or conf_data:find("\n#define LUAT_USE_FSKV")
    local FOTA_CONF = conf_data:find("\r#define LUAT_USE_FOTA") or conf_data:find("\n#define LUAT_USE_FOTA")
    local WLAN_CONF = conf_data:find("\r#define LUAT_USE_WLAN") or conf_data:find("\n#define LUAT_USE_WLAN")
    local NIMBLE_CONF = conf_data:find("\r#define LUAT_USE_NIMBLE") or conf_data:find("\n#define LUAT_USE_NIMBLE")
    local TLS_CONF = conf_data:find("\r#define LUAT_USE_TLS") or conf_data:find("\n#define LUAT_USE_TLS")

    -- 识别芯片型号
    local is_air101 = conf_data:find("\r#define AIR101") or conf_data:find("\n#define AIR101")
    local is_air103 = conf_data:find("\r#define AIR103") or conf_data:find("\n#define AIR103")
    local is_air601 = conf_data:find("\r#define AIR601") or conf_data:find("\n#define AIR601")
    local is_air690 = conf_data:find("\r#define AIR690") or conf_data:find("\n#define AIR690")
    local is_air6208 = conf_data:find("\r#define AIR6208") or conf_data:find("\n#define AIR6208")

    local TARGET_NAME
    if is_air101 then TARGET_NAME = "AIR101"
    elseif is_air103 then TARGET_NAME = "AIR103"
    elseif is_air601 then TARGET_NAME = "AIR601"
    elseif is_air690 then TARGET_NAME = "AIR690"
    elseif is_air6208 then TARGET_NAME = "AIR6208"
    else raise("未识别的芯片型号, 请在 luat_conf_bsp.h 中定义 AIR101/AIR103/AIR601/AIR690/AIR6208") end

    -- 加载对应型号的分区表CSV
    local csv_path = "$(projectdir)/partition/" .. TARGET_NAME .. ".csv"
    local pt = parse_csv(csv_path)
    local meta = pt.meta
    local partitions = pt.partitions

    -- 从分区表提取各区域参数
    local flash_base = meta.flash_base
    local flash_size = meta.flash_size
    local ram_end = meta.ram_end

    local flash_fs_size = partitions.fs and partitions.fs.size or 0
    local flash_script_size = partitions.script and partitions.script.size or 0
    local flash_kv_size = partitions.kv and partitions.kv.size or 0
    local flash_fota_size = partitions.fota and partitions.fota.size or 0

    -- app分区包含1K image header, 实际代码从 app.offset + 1024 开始
    local app_part = partitions.app
    if not app_part then raise("分区表缺少 app 分区: " .. csv_path) end
    local flash_app_offset = flash_base + app_part.offset + 1024
    local flash_app_size = app_part.size - 1024

    -- [deprecated] 以下全局变量保留供 xmake.lua after_build 使用, 值来自分区表CSV
    LUAT_SCRIPT_SIZE = flash_script_size // 1024
    AIR10X_FLASH_FS_REGION_SIZE = (flash_fs_size + flash_script_size) // 1024

    local ISRAM = string.format("I-SRAM : ORIGIN = 0x%x , LENGTH = 0x%x", flash_app_offset, flash_app_size)

    -- 打印分区表
    print(string.format("分区表: %s (%dK Flash)", TARGET_NAME, flash_size // 1024))
    print(string.format("  %-12s %-8s %-12s %-8s", "名称", "类型", "偏移量", "大小"))
    for _, p in ipairs(pt.partition_list) do
        print(string.format("  %-12s %-8s 0x%06X     %dK", p.name, p.type, p.offset, p.size // 1024))
    end
    print("ISRAM", ISRAM)

    local result = {}

    result.target_name = TARGET_NAME
    result.flash_size = flash_size
    result.flash_fs_size = flash_fs_size
    result.flash_script_size = flash_script_size
    result.flash_kv_size = flash_kv_size
    result.flash_app_size = flash_app_size
    result.flash_app_offset = flash_app_offset
    result.flash_fota_size = flash_fota_size

    result.ISRAM = ISRAM
    result.is_air101 = is_air101
    result.is_air103 = is_air103
    result.is_air601 = is_air601
    result.is_air690 = is_air690
    result.is_air6208 = is_air6208
    result.use_lvgl = LVGL_CONF
    result.use_fota = FOTA_CONF
    result.use_fdb = FDB_CONF
    result.bsp_version = AIR10X_VERSION
    result.use_64bit = VM_64BIT
    result.use_wlan = WLAN_CONF
    result.use_nimble = NIMBLE_CONF
    result.use_tls = TLS_CONF
    result.ram_end = ram_end
    
    -- 添加分区表原始信息供 after_build 使用
    result.flash_base = flash_base
    result.partitions = partitions
    result.script_offset = partitions.script and partitions.script.offset or 0

    -- 生成分区内存头文件
    local header_lines = {
        string.format("// Auto-generated from partition/%s.csv", TARGET_NAME),
        "// DO NOT EDIT MANUALLY",
        "// Generated by buildx.lua",
        "",
        string.format("#ifndef LUAT_PARTITION_MEM_%s_H", TARGET_NAME),
        string.format("#define LUAT_PARTITION_MEM_%s_H", TARGET_NAME),
        "",
        string.format("// %s (%dKB Flash)", TARGET_NAME, flash_size // 1024),
    }

    for _, p in ipairs(pt.partition_list) do
        local partition_macro = p.name:upper():gsub("[^%w]", "_")
        table.insert(header_lines,
            string.format("#define %-36s 0x%08XU      // %s 分区偏移", "LUAT_PARTITION_" .. partition_macro .. "_ADDR", p.offset, p.type))
        table.insert(header_lines,
            string.format("#define %-36s 0x%08XU      // %dKB", "LUAT_PARTITION_" .. partition_macro .. "_SIZE", p.size, p.size // 1024))
    end

    table.insert(header_lines, "")
    table.insert(header_lines, "// 兼容旧代码的 KB 宏")
    table.insert(header_lines, "#define LUAT_FS_SIZE                       (LUAT_PARTITION_FS_SIZE / 1024U)")
    table.insert(header_lines, "#define LUAT_SCRIPT_SIZE                   (LUAT_PARTITION_SCRIPT_SIZE / 1024U)")
    table.insert(header_lines, "")
    table.insert(header_lines, string.format("#endif /* LUAT_PARTITION_MEM_%s_H */", TARGET_NAME))

    local header_content = table.concat(header_lines, "\n") .. "\n"
    local header_path = "$(projectdir)/app/port/partition_mem_" .. TARGET_NAME .. ".h"
    
    existing_content = nil
    if os.exists(header_path) then
        -- 读取文件, 内容不同才写入, 避免不必要的文件修改时间更新
        existing_content = io.readfile(header_path)
    end
    if existing_content == header_content then
        print("分区内存头文件未改变: " .. header_path)
    else
        io.writefile(header_path, header_content)
        print("生成分区内存头文件: " .. header_path)
    end

    return result
end
