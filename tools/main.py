
import os
import subprocess
import sys
import serial
import serial.tools.list_ports
import time
import traceback
import _thread

SPOOL = {}


def burn_dev(device):
    ser = None
    linelen = len("I/main auth ok 680845463968338A185E air101")+1
    try:
        ser = serial.Serial(device, 115200, timeout=1)
        ser.rts = False
        data = ser.read(linelen)
        ser.close()
        #print(data)
        if data and  b'CCC' in data :
            # {} -ds {} -ws 115200 -c {} -rs rts -dl {}
            print("***********刷机开始**************", device)
            subprocess.check_call(["air101_flash.exe", "-ds", "2M", "-c", str(device), "-ws", "115200", "-rs", "rts", "-dl", "air10x.fls"])
            print("***********刷机完成**************", device)

        # TODO 读取授权信息,联网授权
        ser = serial.Serial(device, 921600, timeout=1)
        ser.rts = False
        data = ser.read(linelen)
        #ser.close()
        
        if data and b'auth ok' in data:
            print("============设备已刷机且已激活===", device)
            time.sleep(3)
        else :
            ser.close()
            ser = None
            print("===非刷机模式的COM===========", device)
            time.sleep(3)
        SPOOL.pop(str(device))
    except Exception:
        traceback.print_exc()
        if ser :
            ser.close()
        time.sleep(1)

def serial_lpop():
    for item in serial.tools.list_ports.comports():
        if "CH340" in str(item.description) :
            if str(item.device) in SPOOL :
                continue
            SPOOL[str(item.device)] = True
            _thread.start_new_thread(burn_dev, (item.device,))
    print("===等待新的设备==========================")

def main():
    while 1:
        try :
            serial_lpop()
        except Exception:
            traceback.print_exc()
        time.sleep(1)

if __name__ == "__main__" :
    main()
