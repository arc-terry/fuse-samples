#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

struct demo_file {
    const char *path;
    const char *node_name;
};

static const struct demo_file demo_files[] = {
    { "/mcu/sensor/current/version", "mcu/sensor/current/version" },
    { "/mcu/sensor/thermal/version", "mcu/sensor/thermal/version" },
    { "/mcu/sensor/position/version", "mcu/sensor/position/version" },
};

static const char *demo_dirs[] = {
    "/",
    "/mcu",
    "/mcu/sensor",
    "/mcu/sensor/current",
    "/mcu/sensor/thermal",
    "/mcu/sensor/position",
};

static const size_t demo_file_count = sizeof(demo_files) / sizeof(demo_files[0]);
static const size_t demo_dir_count = sizeof(demo_dirs) / sizeof(demo_dirs[0]);

/*
 * Run a command and capture stdout into buf.
 * Returns number of bytes written to buf, or -1 on error.
 */
static int run_command_capture(const char *cmd, char *buf, size_t bufsize)
{
    FILE *fp;
    size_t total = 0;

    if (bufsize == 0) {
        return -1;
    }

    fp = popen(cmd, "r");
    if (!fp) {
        return -1;
    }

    while (total < bufsize - 1) {
        size_t n = fread(buf + total, 1, bufsize - 1 - total, fp);
        total += n;
        if (n == 0) {
            break;
        }
    }

    buf[total] = '\0';
    pclose(fp);
    return (int) total;
}

static const struct demo_file *find_file(const char *path)
{
    size_t i;

    for (i = 0; i < demo_file_count; ++i) {
        if (strcmp(path, demo_files[i].path) == 0) {
            return &demo_files[i];
        }
    }

    return NULL;
}

static int is_directory(const char *path)
{
    size_t i;

    for (i = 0; i < demo_dir_count; ++i) {
        if (strcmp(path, demo_dirs[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

static int nested_getattr(const char *path, struct stat *stbuf,
                          struct fuse_file_info *fi)
{
    const struct demo_file *file;

    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));

    if (is_directory(path)) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    file = find_file(path);
    if (file != NULL) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 512;
        return 0;
    }

    return -ENOENT;
}

static int nested_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags)
{
    (void) offset;
    (void) fi;
    (void) flags;

    if (!is_directory(path)) {
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    if (strcmp(path, "/") == 0) {
        filler(buf, "mcu", NULL, 0, 0);
        return 0;
    }

    if (strcmp(path, "/mcu") == 0) {
        filler(buf, "sensor", NULL, 0, 0);
        return 0;
    }

    if (strcmp(path, "/mcu/sensor") == 0) {
        filler(buf, "current", NULL, 0, 0);
        filler(buf, "thermal", NULL, 0, 0);
        filler(buf, "position", NULL, 0, 0);
        return 0;
    }

    if (strcmp(path, "/mcu/sensor/current") == 0 ||
        strcmp(path, "/mcu/sensor/thermal") == 0 ||
        strcmp(path, "/mcu/sensor/position") == 0) {
        filler(buf, "version", NULL, 0, 0);
        return 0;
    }

    return -ENOENT;
}

static int nested_open(const char *path, struct fuse_file_info *fi)
{
    if (find_file(path) == NULL) {
        return -ENOENT;
    }

    if ((fi->flags & O_ACCMODE) != O_RDONLY) {
        return -EACCES;
    }

    return 0;
}

static int nested_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi)
{
    char output[256];
    char formatted[512];
    const struct demo_file *file;
    int len;
    int formatted_len;

    (void) fi;

    file = find_file(path);
    if (file == NULL) {
        return -ENOENT;
    }

    len = run_command_capture("whoami", output, sizeof(output));
    if (len < 0) {
        return -EIO;
    }

    formatted_len = snprintf(formatted, sizeof(formatted),
                             "Node: %s\n"
                             "Path: %s\n"
                             "Command: whoami\n"
                             "Output: %s",
                             file->node_name, file->path, output);
    if (formatted_len < 0) {
        return -EIO;
    }

    if ((size_t) formatted_len >= sizeof(formatted)) {
        formatted_len = (int) sizeof(formatted) - 1;
        formatted[formatted_len] = '\0';
    }

    if ((size_t) offset >= (size_t) formatted_len) {
        return 0;
    }

    if (offset + size > (size_t) formatted_len) {
        size = (size_t) formatted_len - (size_t) offset;
    }

    memcpy(buf, formatted + offset, size);
    return (int) size;
}

static const struct fuse_operations nested_ops = {
    .getattr = nested_getattr,
    .readdir = nested_readdir,
    .open    = nested_open,
    .read    = nested_read,
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &nested_ops, NULL);
}
