
# Air101/Air103 固件源码库

## 编译说明
自行下载平头哥[编译器](https://occ.t-head.cn/community/download?id=3885366095506644992)解压命名为csky-elfabiv2-tools-mingw-minilibc
windows放在E盘根目录下并配置好环境变量
linux放在opt根目录下并配置好环境变量

详细安装见[ReleaseNotes.pdf](https://occ-oss-prod.oss-cn-hangzhou.aliyuncs.com/resource/null/1614234913073/ReleaseNotes.pdf)

release模式会编译出.soc格式，需安装[7z](https://www.7-zip.org/)

## windows环境下

下载xmake[安装包](https://github.com/xmake-io/xmake/releases)安装,在命令行下cd到代码所在目录,执行xmake即可

## lib编译

在lib目录下执行xmake -j1 -P即可



