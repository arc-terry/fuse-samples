#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *trigger_path = "/trigger.txt";

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
    return (int)total;
}

static int demo_getattr(const char *path, struct stat *stbuf,
                        struct fuse_file_info *fi)
{
    (void)fi;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    if (strcmp(path, trigger_path) == 0) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;

        /* fake size; actual read result may be smaller */
        stbuf->st_size = 128;
        return 0;
    }

    return -ENOENT;
}

static int demo_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi,
                        enum fuse_readdir_flags flags)
{
    (void)offset;
    (void)fi;
    (void)flags;

    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    filler(buf, "trigger.txt", NULL, 0, 0);

    return 0;
}

static int demo_open(const char *path, struct fuse_file_info *fi)
{
    if (strcmp(path, trigger_path) != 0) {
        return -ENOENT;
    }

    if ((fi->flags & O_ACCMODE) != O_RDONLY) {
        return -EACCES;
    }

    return 0;
}

static int demo_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi)
{
    char output[256];
    int len;

    (void)fi;

    if (strcmp(path, trigger_path) != 0) {
        return -ENOENT;
    }

    /*
     * Demo behavior:
     * reading /trigger.txt executes "whoami"
     */
    len = run_command_capture("whoami", output, sizeof(output));
    if (len < 0) {
        return -EIO;
    }

    /* Optional: make output clearer in demo */
    {
        char formatted[300];
        int formatted_len = snprintf(formatted, sizeof(formatted),
                                     "Command: whoami\nOutput: %s", output);

        if (formatted_len < 0) {
            return -EIO;
        }

        if ((size_t)formatted_len >= sizeof(formatted)) {
            formatted_len = (int)sizeof(formatted) - 1;
            formatted[formatted_len] = '\0';
        }

        if ((size_t)offset >= (size_t)formatted_len) {
            return 0;
        }

        if (offset + size > (size_t)formatted_len) {
            size = (size_t)formatted_len - (size_t)offset;
        }

        memcpy(buf, formatted + offset, size);
        return (int)size;
    }
}

static const struct fuse_operations demo_ops = {
    .getattr = demo_getattr,
    .readdir = demo_readdir,
    .open    = demo_open,
    .read    = demo_read,
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &demo_ops, NULL);
}
