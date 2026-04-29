#!/usr/bin/python3
# -*- coding: utf-8 -*-

import os, datetime, re, glob, json, time, requests


def upload_with_retry(upload_url, token, local_path, remote_rel_path, retries=3):
    """Upload a .soc file via HTTP with exponential back-off retry."""
    for attempt in range(1, retries + 1):
        try:
            with open(local_path, "rb") as fh:
                resp = requests.post(
                    f"{upload_url}/api/upload",
                    headers={"Authorization": f"Bearer {token}"},
                    files={"file": fh},
                    data={"path": remote_rel_path},
                    timeout=120,
                )
            resp.raise_for_status()
            return resp.json()
        except Exception as exc:
            print(f"  attempt {attempt}/{retries} failed: {exc}")
            if attempt == retries:
                raise
            time.sleep(2 ** attempt)


def main():
    upload_url = os.environ.get("UPLOAD_URL", "http://sh02.air32.cn:43002").rstrip("/")
    upload_token = os.environ["UPLOAD_TOKEN"]

    data_json = {
        "version": 1,
        "files": []
    }
    t = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")

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

    soc_files = glob.glob("build/out/LuatOS-SoC_V" + fw_ver + "_*.soc")
    if not soc_files:
        print("未找到任何 .soc 文件, 路径: build/out/LuatOS-SoC_V" + fw_ver + "_*.soc")
        return

    for soc_file in soc_files:
        file = os.path.basename(soc_file)
        target_name = file[:-4].split("_")[-1]
        fw_dir = version_dir + "_" + target_name
        dst = file[:-4] + "_" + t + ".soc"
        remote_rel_path = dst_dir + "/" + version_dir + "/" + fw_dir + "/" + dst

        print("deploy", file, "->", remote_rel_path)
        result = upload_with_retry(upload_url, upload_token, soc_file, remote_rel_path)
        url = result["url"]
        print("  uploaded:", url)

        data_json["files"].append({
            "file_path": dst,
            "model": dst_dir,
            "number": target_name,
            "url": url,
            "urls": [url],
            "priority": 9,
        })

    print("deploy done, 总共上传了", len(data_json["files"]), "个文件")
    with open("ci_build_result.json", "w", encoding="utf-8") as f:
        json.dump(data_json, f, indent=4, ensure_ascii=False)


if __name__ == "__main__":
    main()
