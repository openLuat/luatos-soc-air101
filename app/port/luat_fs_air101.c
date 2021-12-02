// #include "luat_conf_bsp.h"
#include "luat_base.h"
#include "luat_fs.h"
#define LUAT_LOG_TAG "luat.fs"
#include "luat_log.h"
#include "lfs_port.h"
#include "wm_include.h"

extern struct lfs_config lfs_cfg;
extern lfs_t lfs;

#ifndef LUAT_USE_FS_VFS

FILE *luat_fs_fopen(const char *filename, const char *mode)
{
    ////LLOGD("fopen %s %s", filename, mode);
    int ret;
    char *t = (char *)mode;
    int flags = 0;
    lfs_file_t *lfsfile = NULL;
    for (int i = 0; i < strlen(mode); i++)
    {

        switch (*(t++))
        {
        case 'w':
            flags |= LFS_O_RDWR;
            break;
        case 'r':
            flags |= LFS_O_RDONLY;
            break;
        case 'a':
            flags |= LFS_O_APPEND;
            break;
        case 'b':
            break;
        case '+':
            break;
        default:

            break;
        }
    }

    lfsfile = tls_mem_alloc(sizeof(lfs_file_t));
    if (!lfsfile)
    {
        return lfsfile;
    }

    ret = lfs_file_open(&lfs, lfsfile, filename, flags);
    if (ret != 0)
    {
        return NULL;
    }

    return lfsfile;

}

int luat_fs_getc(FILE *stream)
{
    return getc(stream);
}

int luat_fs_fseek(FILE *stream, long int offset, int origin)
{
    int ret;
    ret = lfs_file_seek(&lfs, stream, offset, origin);
    if (ret < 0)
    {
        return -1;
    }

    return ret;
}

int luat_fs_ftell(FILE *stream)
{
    return ftell(stream);
}

int luat_fs_fclose(FILE *stream)
{
    int ret;
    ret = lfs_file_close(&lfs, (lfs_dir_t *)stream);
    if (ret != 0)
    {
        return 1;
    }
    tls_mem_free(stream);
    return 0;
}
int luat_fs_feof(FILE *stream)
{
    return feof(stream);
}
int luat_fs_ferror(FILE *stream)
{
    return ferror(stream);
}
size_t luat_fs_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    int ret;
    ret = lfs_file_read(&lfs, stream,ptr, size * nmemb);
    if (ret < 0)
    {
        return 0;
    }
    return  ret;
}
size_t luat_fs_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    int ret;
    ret = lfs_file_write(&lfs,stream, ptr, size * nmemb);
    if (ret < 0)
    {
        return 0;
    }

    return ret;
}
int luat_fs_remove(const char *filename)
{
    int ret;
    ret = lfs_remove(&lfs,filename);
    if(ret != 0)
    {
        return 1;
    }
    return 0;
}
int luat_fs_rename(const char *old_filename, const char *new_filename)
{
    int ret;
    ret = lfs_rename(&lfs, old_filename,new_filename);
    if (ret != 0)
    {
       return 1;
    }
    return 0;
}
int luat_fs_fexist(const char *filename)
{
    FILE *fd = luat_fs_fopen(filename, "rb");
    if (fd)
    {
        luat_fs_fclose(fd);
        return 1;
    }
    return 0;
}

size_t luat_fs_fsize(const char *filename)
{
    FILE *fd;
    size_t size = 0;
    fd = luat_fs_fopen(filename, "rb");
    if (fd)
    {
        luat_fs_fseek(fd, 0, SEEK_END);
        size = luat_fs_ftell(fd);
        luat_fs_fclose(fd);
    }
    return size;
}

int luat_fs_init(void)
{
    LFS_Init();
}

#else

extern const struct luat_vfs_filesystem vfs_fs_lfs2;
#ifdef LUAT_USE_VFS_INLINE_LIB
extern const char luadb_inline_sys[];
extern const char luadb_inline[];
extern const struct luat_vfs_filesystem vfs_fs_luadb;
#endif

#ifdef LUAT_USE_LVGL
#include "lvgl.h"
void luat_lv_fs_init(void);
void lv_bmp_init(void);
void lv_png_init(void);
void lv_split_jpeg_init(void);
#endif

int luat_fs_init(void) {
    LFS_Init();
    luat_vfs_reg(&vfs_fs_lfs2);
	luat_fs_conf_t conf = {
		.busname = &lfs,
		.type = "lfs2",
		.filesystem = "lfs2",
		.mount_point = "/"
	};
	luat_fs_mount(&conf);
	#ifdef LUAT_USE_VFS_INLINE_LIB
	luat_vfs_reg(&vfs_fs_luadb);
	luat_fs_conf_t conf2 = {
		.busname = (char*)luadb_inline_sys,
		.type = "luadb",
		.filesystem = "luadb",
		.mount_point = "/luadb/",
	};
	luat_fs_mount(&conf2);
	#endif

	#ifdef LUAT_USE_LVGL
	luat_lv_fs_init();
	// lv_bmp_init();
	// lv_png_init();
	lv_split_jpeg_init();
	#endif

	return 0;
}

#endif
