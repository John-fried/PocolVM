/* vm_syscalls.c -- PocolVM System Calls and VFS Implementation */

/* Copyright (C) 2026 Bayu Setiawan and Rasya Andrean */

#include "vm_syscalls.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define mkdir _mkdir
#define getcwd _getcwd
#define chdir _chdir
#else
#include <unistd.h>
#include <dirent.h>
#endif

/* Initialize system call context */
void syscalls_init(SysCallContext *ctx) {
    memset(ctx, 0, sizeof(SysCallContext));
    ctx->console_input = stdin;
    ctx->console_output = stdout;
    vfs_init(&ctx->vfs);
    ctx->start_time = time(NULL);
}

/* Free system call context */
void syscalls_free(SysCallContext *ctx) {
    vfs_free(&ctx->vfs);
}

/* VFS Initialization */
void vfs_init(VFS *vfs) {
    memset(vfs, 0, sizeof(VFS));
    strcpy(vfs->current_path, "/");
    
    /* stdin */
    VFile *stdin_file = calloc(1, sizeof(VFile));
    strcpy(stdin_file->name, "stdin");
    strcpy(stdin_file->path, "/dev/stdin");
    stdin_file->type = FTYPE_DEVICE;
    stdin_file->is_open = true;
    stdin_file->is_console = true;
    stdin_file->host_handle = stdin;
    vfs->files[0] = stdin_file;
    vfs->file_count = 1;
    
    /* stdout */
    VFile *stdout_file = calloc(1, sizeof(VFile));
    strcpy(stdout_file->name, "stdout");
    strcpy(stdout_file->path, "/dev/stdout");
    stdout_file->type = FTYPE_DEVICE;
    stdout_file->is_open = true;
    stdout_file->is_console = true;
    stdout_file->host_handle = stdout;
    vfs->files[1] = stdout_file;
    vfs->file_count = 2;
    
    /* stderr */
    VFile *stderr_file = calloc(1, sizeof(VFile));
    strcpy(stderr_file->name, "stderr");
    strcpy(stderr_file->path, "/dev/stderr");
    stderr_file->type = FTYPE_DEVICE;
    stderr_file->is_open = true;
    stderr_file->is_console = true;
    stderr_file->host_handle = stderr;
    vfs->files[2] = stderr_file;
    vfs->file_count = 3;
}

/* Free VFS */
void vfs_free(VFS *vfs) {
    for (int i = 0; i < vfs->file_count; i++) {
        if (vfs->files[i]) {
            if (vfs->files[i]->host_handle && !vfs->files[i]->is_console) {
                fclose((FILE*)vfs->files[i]->host_handle);
            }
            if (vfs->files[i]->buffer) {
                free(vfs->files[i]->buffer);
            }
            free(vfs->files[i]);
        }
    }
}

/* Find free file slot */
VFile* vfs_find_free_slot(VFS *vfs) {
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (!vfs->files[i]) {
            return vfs->files[i] = calloc(1, sizeof(VFile));
        }
    }
    return NULL;
}

/* Find file by path */
VFile* vfs_find_file(VFS *vfs, const char *path) {
    for (int i = 0; i < vfs->file_count; i++) {
        if (vfs->files[i] && strcmp(vfs->files[i]->path, path) == 0) {
            return vfs->files[i];
        }
    }
    return NULL;
}

/* Open file */
VFile* vfs_open(VFS *vfs, const char *path, int mode) {
    VFile *existing = vfs_find_file(vfs, path);
    if (existing && existing->is_open) {
        return existing;
    }
    
    VFile *file = vfs_find_free_slot(vfs);
    if (!file) return NULL;
    
    strncpy(file->path, path, VFS_MAX_PATH - 1);
    const char *name = strrchr(path, '/');
    if (name) {
        strncpy(file->name, name + 1, VFS_MAX_FILENAME - 1);
    } else {
        strncpy(file->name, path, VFS_MAX_FILENAME - 1);
    }
    
    /* Special files */
    if (strcmp(path, "/dev/stdin") == 0 || strcmp(path, "stdin") == 0) {
        file->type = FTYPE_DEVICE;
        file->is_console = true;
        file->host_handle = stdin;
        file->is_open = true;
        return file;
    }
    if (strcmp(path, "/dev/stdout") == 0 || strcmp(path, "stdout") == 0) {
        file->type = FTYPE_DEVICE;
        file->is_console = true;
        file->host_handle = stdout;
        file->is_open = true;
        return file;
    }
    if (strcmp(path, "/dev/stderr") == 0 || strcmp(path, "stderr") == 0) {
        file->type = FTYPE_DEVICE;
        file->is_console = true;
        file->host_handle = stderr;
        file->is_open = true;
        return file;
    }
    
    /* Open host file */
    const char *host_mode;
    switch (mode & 3) {
        case O_RDONLY: host_mode = "rb"; break;
        case O_WRONLY: host_mode = "wb"; break;
        case O_RDWR:   host_mode = "r+b"; break;
        default:       host_mode = "rb"; break;
    }
    if (mode & O_CREAT) host_mode = "w+b";
    
    FILE *fp = fopen(path, host_mode);
    if (!fp) {
        free(file);
        return NULL;
    }
    
    fseek(fp, 0, SEEK_END);
    file->size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    file->type = FTYPE_FILE;
    file->host_handle = fp;
    file->is_open = true;
    file->position = 0;
    file->mode = mode;
    
    return file;
}

/* Close file */
int vfs_close(VFS *vfs, VFile *file) {
    if (!file) return -1;
    if (file->host_handle && !file->is_console) {
        fclose((FILE*)file->host_handle);
    }
    if (file->buffer) {
        free(file->buffer);
    }
    memset(file, 0, sizeof(VFile));
    return 0;
}

/* Read from file */
int64_t vfs_read(VFS *vfs, VFile *file, void *buf, int64_t size) {
    if (!file || !file->is_open || !buf) return -1;
    
    if (file->is_console) {
        if (file->host_handle == stdin) {
            return fread(buf, 1, size, stdin);
        }
        return -1;
    }
    
    if (file->type == FTYPE_FILE && file->host_handle) {
        int64_t bytes = fread(buf, 1, size, (FILE*)file->host_handle);
        file->position += bytes;
        return bytes;
    }
    return -1;
}

/* Write to file */
int64_t vfs_write(VFS *vfs, VFile *file, const void *buf, int64_t size) {
    if (!file || !file->is_open || !buf) return -1;
    
    if (file->is_console) {
        FILE *out = (file->host_handle == stdin) ? stdout : (FILE*)file->host_handle;
        return fwrite(buf, 1, size, out);
    }
    
    if (file->type == FTYPE_FILE && file->host_handle) {
        int64_t bytes = fwrite(buf, 1, size, (FILE*)file->host_handle);
        file->position += bytes;
        if (file->position > file->size) file->size = file->position;
        return bytes;
    }
    return -1;
}

/* Seek in file */
int64_t vfs_seek(VFS *vfs, VFile *file, int64_t offset, int whence) {
    if (!file || !file->is_open) return -1;
    if (file->type == FTYPE_FILE && file->host_handle) {
        if (fseek((FILE*)file->host_handle, offset, whence) != 0) return -1;
        file->position = ftell((FILE*)file->host_handle);
        return file->position;
    }
    return -1;
}

/* Tell file position */
int64_t vfs_tell(VFS *vfs, VFile *file) {
    if (!file || !file->is_open) return -1;
    return file->position;
}

/* Make directory */
int vfs_mkdir(VFS *vfs, const char *path) {
#ifdef _WIN32
    return mkdir(path);
#else
    return mkdir(path, 0755);
#endif
}

/* ========== SYSTEM CALL HANDLERS ========== */

int sys_print(SysCallContext *ctx, PocolVM *vm) {
    uint64_t str_ptr = ctx->arg1;
    uint64_t length = ctx->arg2;
    
    if (str_ptr >= POCOL_MEMORY_SIZE || str_ptr + length >= POCOL_MEMORY_SIZE) {
        ctx->error = ERR_ILLEGAL_INST_ACCESS;
        return -1;
    }
    
    fwrite(&vm->memory[str_ptr], 1, length, stdout);
    fflush(stdout);
    
    ctx->return_value = length;
    return 0;
}

int sys_read(SysCallContext *ctx, PocolVM *vm) {
    uint64_t buf_ptr = ctx->arg1;
    uint64_t max_len = ctx->arg2;
    
    if (buf_ptr >= POCOL_MEMORY_SIZE || buf_ptr + max_len >= POCOL_MEMORY_SIZE) {
        ctx->error = ERR_ILLEGAL_INST_ACCESS;
        return -1;
    }
    
    size_t bytes = fread(&vm->memory[buf_ptr], 1, max_len, stdin);
    ctx->return_value = bytes;
    return 0;
}

int sys_open(SysCallContext *ctx, PocolVM *vm) {
    uint64_t path_ptr = ctx->arg1;
    uint64_t path_len = ctx->arg2;
    int mode = (int)ctx->arg3;
    
    if (path_ptr >= POCOL_MEMORY_SIZE || path_ptr + path_len >= POCOL_MEMORY_SIZE) {
        ctx->error = ERR_ILLEGAL_INST_ACCESS;
        return -1;
    }
    
    char path[VFS_MAX_PATH];
    path_len = (path_len < VFS_MAX_PATH - 1) ? path_len : VFS_MAX_PATH - 1;
    memcpy(path, &vm->memory[path_ptr], path_len);
    path[path_len] = '\0';
    
    VFile *file = vfs_open(&ctx->vfs, path, mode);
    if (!file) {
        ctx->error = errno;
        return -1;
    }
    
    for (int i = 0; i < VFS_MAX_FILES; i++) {
        if (ctx->vfs.files[i] == file) {
            ctx->return_value = i;
            return 0;
        }
    }
    
    ctx->return_value = -1;
    return -1;
}

int sys_close(SysCallContext *ctx, PocolVM *vm) {
    int fd = (int)ctx->arg1;
    
    if (fd < 0 || fd >= VFS_MAX_FILES || !ctx->vfs.files[fd]) {
        ctx->error = EBADF;
        return -1;
    }
    
    int result = vfs_close(&ctx->vfs, ctx->vfs.files[fd]);
    ctx->vfs.files[fd] = NULL;
    ctx->return_value = result;
    return result;
}

int sys_write(SysCallContext *ctx, PocolVM *vm) {
    int fd = (int)ctx->arg1;
    uint64_t buf_ptr = ctx->arg2;
    uint64_t size = ctx->arg3;
    
    if (fd < 0 || fd >= VFS_MAX_FILES || !ctx->vfs.files[fd]) {
        ctx->error = EBADF;
        return -1;
    }
    if (buf_ptr >= POCOL_MEMORY_SIZE || buf_ptr + size >= POCOL_MEMORY_SIZE) {
        ctx->error = ERR_ILLEGAL_INST_ACCESS;
        return -1;
    }
    
    int64_t written = vfs_write(&ctx->vfs, ctx->vfs.files[fd], &vm->memory[buf_ptr], size);
    ctx->return_value = written;
    return (written < 0) ? -1 : 0;
}

int sys_read_file(SysCallContext *ctx, PocolVM *vm) {
    int fd = (int)ctx->arg1;
    uint64_t buf_ptr = ctx->arg2;
    uint64_t size = ctx->arg3;
    
    if (fd < 0 || fd >= VFS_MAX_FILES || !ctx->vfs.files[fd]) {
        ctx->error = EBADF;
        return -1;
    }
    if (buf_ptr >= POCOL_MEMORY_SIZE || buf_ptr + size >= POCOL_MEMORY_SIZE) {
        ctx->error = ERR_ILLEGAL_INST_ACCESS;
        return -1;
    }
    
    int64_t bytes = vfs_read(&ctx->vfs, ctx->vfs.files[fd], &vm->memory[buf_ptr], size);
    ctx->return_value = bytes;
    return (bytes < 0) ? -1 : 0;
}

int sys_seek(SysCallContext *ctx, PocolVM *vm) {
    int fd = (int)ctx->arg1;
    int64_t offset = ctx->arg2;
    int whence = (int)ctx->arg3;
    
    if (fd < 0 || fd >= VFS_MAX_FILES || !ctx->vfs.files[fd]) {
        ctx->error = EBADF;
        return -1;
    }
    
    int64_t result = vfs_seek(&ctx->vfs, ctx->vfs.files[fd], offset, whence);
    ctx->return_value = result;
    return (result < 0) ? -1 : 0;
}

int sys_tell(SysCallContext *ctx, PocolVM *vm) {
    int fd = (int)ctx->arg1;
    
    if (fd < 0 || fd >= VFS_MAX_FILES || !ctx->vfs.files[fd]) {
        ctx->error = EBADF;
        return -1;
    }
    
    int64_t pos = vfs_tell(&ctx->vfs, ctx->vfs.files[fd]);
    ctx->return_value = pos;
    return (pos < 0) ? -1 : 0;
}

int sys_time(SysCallContext *ctx, PocolVM *vm) {
    time_t now = time(NULL);
    ctx->return_value = (int64_t)now;
    return 0;
}

int sys_sleep(SysCallContext *ctx, PocolVM *vm) {
    uint64_t ms = ctx->arg1;
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
    ctx->return_value = 0;
    return 0;
}

int sys_exit(SysCallContext *ctx, PocolVM *vm) {
    vm->halt = 1;
    ctx->return_value = ctx->arg1;
    return 0;
}

int sys_chdir(SysCallContext *ctx, PocolVM *vm) {
    uint64_t path_ptr = ctx->arg1;
    uint64_t path_len = ctx->arg2;
    
    if (path_ptr >= POCOL_MEMORY_SIZE) {
        ctx->error = ERR_ILLEGAL_INST_ACCESS;
        return -1;
    }
    
    char path[VFS_MAX_PATH];
    path_len = (path_len < VFS_MAX_PATH - 1) ? path_len : VFS_MAX_PATH - 1;
    memcpy(path, &vm->memory[path_ptr], path_len);
    path[path_len] = '\0';
    
    int result = chdir(path);
    if (result == 0) {
        strncpy(ctx->vfs.current_path, path, VFS_MAX_PATH - 1);
    }
    ctx->return_value = result;
    return result;
}

int sys_getcwd(SysCallContext *ctx, PocolVM *vm) {
    uint64_t buf_ptr = ctx->arg1;
    uint64_t size = ctx->arg2;
    
    if (buf_ptr >= POCOL_MEMORY_SIZE) {
        ctx->error = ERR_ILLEGAL_INST_ACCESS;
        return -1;
    }
    
    char cwd[VFS_MAX_PATH];
    if (!getcwd(cwd, VFS_MAX_PATH)) {
        ctx->return_value = -1;
        return -1;
    }
    
    size_t len = strlen(cwd);
    size_t copy_len = (len < size && len < POCOL_MEMORY_SIZE - buf_ptr) ? len : 0;
    if (copy_len > 0) {
        memcpy(&vm->memory[buf_ptr], cwd, copy_len);
    }
    ctx->return_value = copy_len;
    return 0;
}

int sys_mkdir(SysCallContext *ctx, PocolVM *vm) {
    uint64_t path_ptr = ctx->arg1;
    uint64_t path_len = ctx->arg2;
    
    if (path_ptr >= POCOL_MEMORY_SIZE) {
        ctx->error = ERR_ILLEGAL_INST_ACCESS;
        return -1;
    }
    
    char path[VFS_MAX_PATH];
    path_len = (path_len < VFS_MAX_PATH - 1) ? path_len : VFS_MAX_PATH - 1;
    memcpy(path, &vm->memory[path_ptr], path_len);
    path[path_len] = '\0';
    
    int result = vfs_mkdir(&ctx->vfs, path);
    ctx->return_value = result;
    return result;
}

int sys_system(SysCallContext *ctx, PocolVM *vm) {
    uint64_t cmd_ptr = ctx->arg1;
    uint64_t cmd_len = ctx->arg2;
    
    if (cmd_ptr >= POCOL_MEMORY_SIZE) {
        ctx->error = ERR_ILLEGAL_INST_ACCESS;
        return -1;
    }
    
    char cmd[VFS_MAX_PATH];
    cmd_len = (cmd_len < VFS_MAX_PATH - 1) ? cmd_len : VFS_MAX_PATH - 1;
    memcpy(cmd, &vm->memory[cmd_ptr], cmd_len);
    cmd[cmd_len] = '\0';
    
    int result = system(cmd);
    ctx->return_value = result;
    return result;
}

/* Main system call dispatcher */
int syscalls_exec(SysCallContext *ctx, PocolVM *vm, int syscall_num) {
    ctx->arg1 = vm->registers[1];
    ctx->arg2 = vm->registers[2];
    ctx->arg3 = vm->registers[3];
    ctx->arg4 = vm->registers[4];
    
    ctx->error = 0;
    ctx->return_value = 0;
    
    int result;
    switch (syscall_num) {
        case SYS_PRINT:    result = sys_print(ctx, vm); break;
        case SYS_READ:     result = sys_read(ctx, vm); break;
        case SYS_OPEN:     result = sys_open(ctx, vm); break;
        case SYS_CLOSE:    result = sys_close(ctx, vm); break;
        case SYS_WRITE:    result = sys_write(ctx, vm); break;
        case SYS_READ_FILE:result = sys_read_file(ctx, vm); break;
        case SYS_SEEK:     result = sys_seek(ctx, vm); break;
        case SYS_TELL:     result = sys_tell(ctx, vm); break;
        case SYS_TIME:     result = sys_time(ctx, vm); break;
        case SYS_SLEEP:    result = sys_sleep(ctx, vm); break;
        case SYS_EXIT:     result = sys_exit(ctx, vm); break;
        case SYS_CHDIR:    result = sys_chdir(ctx, vm); break;
        case SYS_GETCWD:   result = sys_getcwd(ctx, vm); break;
        case SYS_MKDIR:    result = sys_mkdir(ctx, vm); break;
        case SYS_SYSTEM:   result = sys_system(ctx, vm); break;
        default:
            ctx->error = ENOSYS;
            result = -1;
    }
    
    vm->registers[0] = ctx->return_value;
    return result;
}

/* Error string */
const char* sys_strerror(int error) {
    switch (error) {
        case 0: return "Success";
        case ERR_ILLEGAL_INST: return "Illegal instruction";
        case ERR_ILLEGAL_INST_ACCESS: return "Illegal memory access";
        case ERR_STACK_OVERFLOW: return "Stack overflow";
        case ERR_STACK_UNDERFLOW: return "Stack underflow";
        case ENOENT: return "No such file or directory";
        case EBADF: return "Bad file descriptor";
        case EACCES: return "Permission denied";
        case ENOMEM: return "Out of memory";
        case EEXIST: return "File exists";
        case ENOSYS: return "Function not implemented";
        default: return strerror(error);
    }
}