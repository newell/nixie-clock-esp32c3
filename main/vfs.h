#ifndef VFS_H
#define VFS_H

#include <esp_vfs_fat.h>

esp_err_t vfs_init(void);
esp_err_t vfs_unregister(void);

#endif /* VFS_H */
