#!/usr/bin/python3
# -*- coding: utf-8 -*-

import os, subprocess, datetime, re, glob, json

def main():
    data_json = {
        "version": 1,
        "files": []
    }
    t = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")

    # 从 luat_conf_bsp.h 中读取固件版本号
    fw_ver = None
    with open("app/port/luat_conf_bsp.h", "r", encoding="utf-8") as f:
        for line in f:
            m = re.match(r'\s*#define\s+LUAT_BSP_VERSION\s+"V(\d+)"', line)
            if m:
                fw_ver = m.group(1)
                break

    if not fw_ver:
        print("未找到 LUAT_BSP_VERSION, 退出")
        return

    dst_dir = "air101"
    version_dir = "V" + fw_ver

    # 查找 build/out/ 下所有 .soc 文件
    soc_files = glob.glob("build/out/LuatOS-SoC_V" + fw_ver + "_*.soc")
    if not soc_files:
        print("未找到任何 .soc 文件, 路径: build/out/LuatOS-SoC_V" + fw_ver + "_*.soc")
        return

    for soc_file in soc_files:
        file = os.path.basename(soc_file)
        # 文件名如 LuatOS-SoC_V2001_AIR101.soc -> target_name = AIR101
        target_name = file[:-4].split('_')[-1]
        fw_dir = version_dir + "_" + target_name
        dst = file[:-4] + "_" + t + ".soc"
        remote_rel_dir = dst_dir + "/" + version_dir + "/" + fw_dir

        print("deploy", file, "->", remote_rel_dir + "/" + dst)

        subprocess.check_call([
            "/usr/bin/ssh", "-i", "deploy/id_ed25519", "-o", "StrictHostKeyChecking=no",
            "air602@sh02.air32.cn", "mkdir", "-p", "/opt/wendal/air32/" + remote_rel_dir
        ])
        subprocess.check_call([
            "/usr/bin/scp", "-i", "deploy/id_ed25519", "-o", "StrictHostKeyChecking=no",
            soc_file, "air602@sh02.air32.cn:/opt/wendal/air32/" + remote_rel_dir + "/" + dst
        ])

        data_json["files"].append({
            "file_path": dst,
            "model": dst_dir,
            "number": target_name,
            "url": "http://sh02.air32.cn:43001/" + remote_rel_dir + "/" + dst,
            "urls": [
                "http://sh02.air32.cn:43001/" + remote_rel_dir + "/" + dst
            ],
            "priority": 9
        })

    print("deploy done, 总共上传了", len(data_json["files"]), "个文件")
    with open("ci_build_result.json", "w", encoding="utf-8") as f:
        json.dump(data_json, f, indent=4, ensure_ascii=False)

if __name__ == "__main__":
    main()
