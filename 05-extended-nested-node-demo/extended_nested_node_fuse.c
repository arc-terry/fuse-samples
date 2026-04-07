#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <fuse3/fuse_opt.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct demo_file {
    const char *path;
    const char *node_name;
};

static const struct demo_file demo_files[] = {
    { "/mcu/version", "mcu/version" },
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
 * Run command via fork/pipe/execvp and capture child stdout into buf.
 * Returns number of bytes written to buf, or -1 on error/non-zero exit.
 */
static int run_command_capture_execvp(const char *cmd, char *const argv[],
                                      char *buf, size_t bufsize)
{
    int pipefd[2];
    pid_t pid;
    size_t total = 0;

    if (bufsize == 0) {
        return -1;
    }

    if (pipe(pipefd) < 0) {
        return -1;
    }

    pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid == 0) {
        close(pipefd[0]);

        if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
            _exit(127);
        }

        close(pipefd[1]);
        execvp(cmd, argv);
        _exit(127);
    }

    close(pipefd[1]);

    while (total < bufsize - 1) {
        ssize_t n = read(pipefd[0], buf + total, bufsize - 1 - total);

        if (n > 0) {
            total += (size_t) n;
            continue;
        }

        if (n == 0) {
            break;
        }

        if (errno == EINTR) {
            continue;
        }

        close(pipefd[0]);
        while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) {
        }
        return -1;
    }

    buf[total] = '\0';
    close(pipefd[0]);

    {
        int status;
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                return -1;
            }
        }

        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            return -1;
        }
    }

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
        filler(buf, "version", NULL, 0, 0);
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
    const char *cmd;
    const char *cmd_label;
    char *const *cmd_argv;
    int len;
    int formatted_len;
    char *const whoami_argv[] = { "whoami", NULL };
    char *const ls_root_argv[] = { "ls", "/root", NULL };

    (void) fi;

    file = find_file(path);
    if (file == NULL) {
        return -ENOENT;
    }

    if (strcmp(file->path, "/mcu/version") == 0) {
        cmd = "ls";
        cmd_argv = ls_root_argv;
        cmd_label = "ls /root";
    } else {
        cmd = "whoami";
        cmd_argv = whoami_argv;
        cmd_label = "whoami";
    }

    len = run_command_capture_execvp(cmd, cmd_argv, output, sizeof(output));
    if (len < 0) {
        return -EIO;
    }

    formatted_len = snprintf(formatted, sizeof(formatted),
                             "Node: %s\n"
                             "Path: %s\n"
                             "Command: %s\n"
                             "Output: %s",
                             file->node_name, file->path, cmd_label, output);
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

static int has_max_threads_option(int argc, char *argv[])
{
    int i;

    for (i = 1; i < argc; ++i) {
        const char *arg = argv[i];

        if (strcmp(arg, "-s") == 0 || strcmp(arg, "--singlethread") == 0) {
            return 1;
        }

        if (strncmp(arg, "-omax_threads=", 14) == 0 ||
            strncmp(arg, "max_threads=", 12) == 0 ||
            strncmp(arg, "--max-threads=", 14) == 0) {
            return 1;
        }

        if (strcmp(arg, "-o") == 0 && i + 1 < argc &&
            strstr(argv[i + 1], "max_threads=") != NULL) {
            return 1;
        }
    }

    return 0;
}

static int has_max_idle_threads_option(int argc, char *argv[])
{
    int i;

    for (i = 1; i < argc; ++i) {
        const char *arg = argv[i];

        if (strncmp(arg, "-omax_idle_threads=", 19) == 0 ||
            strncmp(arg, "max_idle_threads=", 17) == 0) {
            return 1;
        }

        if (strcmp(arg, "-o") == 0 && i + 1 < argc &&
            strstr(argv[i + 1], "max_idle_threads=") != NULL) {
            return 1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int i;
    int ret;
    struct fuse_args args = FUSE_ARGS_INIT(0, NULL);

    for (i = 0; i < argc; ++i) {
        if (fuse_opt_add_arg(&args, argv[i]) != 0) {
            fuse_opt_free_args(&args);
            return -1;
        }
    }

    if (!has_max_threads_option(argc, argv)) {
        if (fuse_opt_add_arg(&args, "-o") != 0 ||
            fuse_opt_add_arg(&args, "max_threads=10") != 0) {
            fuse_opt_free_args(&args);
            return -1;
        }
    }

    if (!has_max_idle_threads_option(argc, argv)) {
        if (fuse_opt_add_arg(&args, "-o") != 0 ||
            fuse_opt_add_arg(&args, "max_idle_threads=10") != 0) {
            fuse_opt_free_args(&args);
            return -1;
        }
    }

    ret = fuse_main(args.argc, args.argv, &nested_ops, NULL);
    fuse_opt_free_args(&args);
    return ret;
}
