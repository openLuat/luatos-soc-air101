-- LuaTools需要PROJECT和VERSION这两个信息
PROJECT = "uart_irq"
VERSION = "1.0.0"

-- 引入必要的库文件(lua编写), 内部库不需要require
local sys = require "sys"

-- log.info("main", "uart demo")

-- i2c.setup(0)
-- -- u8g2.begin({mode="i2c_sw", pin0=1, pin1=4})
-- u8g2.begin({mode="i2c_hw",i2c_id=0})
-- --u8g2.begin({mode="spi_hw_4pin",spi_id=1,OLED_SPI_PIN_RES=20,OLED_SPI_PIN_DC=28,OLED_SPI_PIN_CS=29})
-- u8g2.SetFontMode(1)
-- u8g2.ClearBuffer()
-- u8g2.SetFont(u8g2.font_ncenB08_tr)
-- u8g2.DrawUTF8("U8g2+LuatOS", 32, 22)
-- u8g2.SendBuffer()

-- local uartid = 1

-- --初始化
-- local result = uart.setup(
--     uartid,--串口id
--     115200,--波特率
--     8,--数据位
--     1--停止位
-- )


--循环发数据
-- sys.timerLoopStart(uart.write,1000,uartid,"test")
-- uart.on(uartid, "receive", function(id, len)
--     log.info("uart", "receive", id, len, uart.read(uartid, len))
-- end)
-- uart.on(uartid, "sent", function(id)
--     log.info("uart", "sent", id)
-- end)

--轮询例子
-- sys.taskInit(function ()
--     while true do
--         local data = uart.read(uartid,maxBuffer)
--         if data:len() > 0 then
--             print(data)
--             uart.write(uartid,"receive:"..data)
--         end
--         sys.wait(100)--不阻塞的延时函数
--     end
-- end)
--[[

    uart.None,--校验位
    uart.LSB,--高低位顺序
    maxBuffer,--缓冲区大小
    function ()--接收回调
        local str = uart.read(uartid,maxBuffer)
        print("uart","receive:"..str)
    end,
    function ()--发送完成回调
        print("uart","send ok")
    end
]]
-- timer.mdelay(5000)
-- 打印根分区的信息
-- log.info("fsstat", fs.fsstat("/"))


sys.taskInit(function()
    sdio.init(0)
    sdio.mount(0,0)
    local f = io.open("/sd/boot_time", "rb")
    local c = 0
    if f then
        local data = f:read("*a")
        log.info("fs", "data", data, data:toHex())
        c = tonumber(data)
        f:close()
    end
    log.info("fs", "boot count", c)
    c = c + 1
    f = io.open("/sd/boot_time", "wb")
    --if f ~= nil then
    log.info("fs", "write c to file", c, tostring(c))
    f:write(tostring(c))
    f:close()


    log.info("os.date()", os.date())
    local t = rtc.get()
    log.info("rtc", json.encode(t))
    sys.wait(2000)
    rtc.set({year=2021,mon=8,day=31,hour=17,min=8,sec=43})
    log.info("os.date()", os.date())
    -- rtc.timerStart(0, {year=2021,mon=9,day=1,hour=17,min=8,sec=43})
    -- rtc.timerStop(0)
    while 1 do
        -- log.info("os.date()", os.date())
        -- local t = rtc.get()
        -- log.info("rtc", json.encode(t))
        sys.wait(500)
    end
end)

-- 用户代码已结束---------------------------------------------
-- 结尾总是这一句
sys.run()
-- sys.run()之后后面不要加任何语句!!!!!
