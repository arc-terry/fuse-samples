#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

struct demo_node {
    const char *path;
    const char *name;
};

static const struct demo_node demo_nodes[] = {
    { "/node-1.txt", "node-1" },
    { "/node-2.txt", "node-2" },
    { "/node-3.txt", "node-3" },
};

static const size_t demo_node_count = sizeof(demo_nodes) / sizeof(demo_nodes[0]);

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

static const struct demo_node *find_node(const char *path)
{
    size_t i;

    for (i = 0; i < demo_node_count; ++i) {
        if (strcmp(path, demo_nodes[i].path) == 0) {
            return &demo_nodes[i];
        }
    }

    return NULL;
}

static int multi_getattr(const char *path, struct stat *stbuf,
                         struct fuse_file_info *fi)
{
    const struct demo_node *node;

    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    node = find_node(path);
    if (node != NULL) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 512;
        return 0;
    }

    return -ENOENT;
}

static int multi_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi,
                         enum fuse_readdir_flags flags)
{
    size_t i;

    (void) offset;
    (void) fi;
    (void) flags;

    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    for (i = 0; i < demo_node_count; ++i) {
        filler(buf, demo_nodes[i].path + 1, NULL, 0, 0);
    }

    return 0;
}

static int multi_open(const char *path, struct fuse_file_info *fi)
{
    if (find_node(path) == NULL) {
        return -ENOENT;
    }

    if ((fi->flags & O_ACCMODE) != O_RDONLY) {
        return -EACCES;
    }

    return 0;
}

static int multi_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    char output[256];
    char formatted[512];
    const struct demo_node *node;
    int len;
    int formatted_len;

    (void) fi;

    node = find_node(path);
    if (node == NULL) {
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
                             node->name, node->path, output);
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

static const struct fuse_operations multi_ops = {
    .getattr = multi_getattr,
    .readdir = multi_readdir,
    .open    = multi_open,
    .read    = multi_read,
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &multi_ops, NULL);
}
