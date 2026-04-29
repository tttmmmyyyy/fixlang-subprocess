#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <errno.h>

// Fork child process and launch process by execvp.
// * `error_buf` - If no error occurrs, error_buf will be set to pointing NULL.
//                 Otherwise, error_buf will be set to pointing to null-terminated error string. In this case, the caller should free the string buffer.
// * `streams` - If succceeds, streams[0], streams[1] and streams[2] are set FILE handles that are piped to stdio, stdout and stderr of child process.
void fixsubprocess_fork_execvp(const char *program_path, char *const argv[], char **out_error, FILE *out_streams[], int64_t *out_pid)
{
    *out_error = NULL;

    int pipes[3][2]; // in, out, err

    for (int i = 0; i < 3; i++)
    {
        if (pipe(pipes[i]))
        {
            // Failed creating pipes.
            for (int j = 0; j < i; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            const char *msg = "Failed to create pipe.";
            *out_error = (char *)malloc(sizeof(char) * (strlen(msg) + 1));
            strcpy(*out_error, msg);

            return;
        }
    }
    pid_t pid = fork();
    if (!pid)
    {
        // In child process,

        dup2(pipes[0][0], 0); // stdin
        dup2(pipes[1][1], 1); // stdout
        dup2(pipes[2][1], 2); // stderr

        for (int i = 0; i < 3; i++)
        {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }

        execvp(program_path, argv);

        // If execvp fails,
        fprintf(stderr, "execvp(%s, ...) failed.\n", program_path);
        exit(1);
    }
    else
    { // In parent process,
        if (pid < 0)
        {
            // Failed creating process.

            const char *msg = "Failed to create child process.";
            *out_error = (char *)malloc(sizeof(char) * (strlen(msg) + 1));
            strcpy(*out_error, msg);

            return;
        }
        close(pipes[0][0]);
        close(pipes[1][1]);
        close(pipes[2][1]);

        out_streams[0] = fdopen(pipes[0][1], "w");
        out_streams[1] = fdopen(pipes[1][0], "r");
        out_streams[2] = fdopen(pipes[2][0], "r");

        *out_pid = (int64_t)pid;

        return;
    }
}

typedef struct
{
    uint8_t is_timeout;
    uint8_t wait_failed;
    uint8_t exit_status;
    uint8_t exit_status_available;
    uint8_t stop_signal;
    uint8_t stop_signal_available;
} WaitResult;

// Wait termination of child process specified.
// * `timeout` - Positive for timeout value (in seconds), negative for no timeout.
// * `out_is_timeout` - Set to 1 when return by timeout, or set to 0 otherwise. Should not be NULL when `timeout` is not NULL.
// * `out_wait_failed` - Set to 1 when waiting child process failed, or set to 0 otherwise.
// * `out_exit_status` - The exit status of child process is stored to the address specified this argument. This value should be used only when `*out_exit_status_available == 1`.
// * `out_exit_status_available` - Set to 1 when exit status is available, or set to 0 otherwise.
// * `out_stop_signal` - The signal number which caused the termination of the child process. This value should be used only when `*out_stop_signal_available == 1`.
// * `out_stop_signal_available` - Set to 1 when the stop signal number is available, or set to 0 otherwise.
void fixsubprocess_wait_subprocess(int64_t pid, double timeout,
                                   WaitResult *out)
{
    int wait_status;
    pid_t wait_return;
    struct timespec start;
    double start_f;
    struct timespec now;
    double now_f;

    out->is_timeout = 0;
    out->exit_status_available = 0;
    out->stop_signal_available = 0;
    out->wait_failed = 0;

    if (timeout < 0.0)
    {
        wait_return = waitpid((pid_t)pid, &wait_status, 0);
    }
    else
    {
        clock_gettime(CLOCK_MONOTONIC, &start);
        start_f = (double)start.tv_sec + (double)start.tv_nsec / 1e9;
        while (1)
        {
            // TODO: fix busy wait (using threads?)
            wait_return = waitpid((pid_t)pid, &wait_status, WNOHANG);
            if (wait_return != 0)
            {
                break;
            }
            clock_gettime(CLOCK_MONOTONIC, &now);
            now_f = (double)now.tv_sec + (double)now.tv_nsec / 1e9;
            if (now_f - start_f >= timeout)
            {
                out->is_timeout = 1;
                break;
            }
        }
    }
    if (wait_return == -1)
    {
        out->wait_failed = 1;
        return;
    }
    if (WIFEXITED(wait_status))
    {
        out->exit_status_available = 1;
        out->exit_status = (uint8_t)WEXITSTATUS(wait_status);
        return;
    }
    if (WIFSIGNALED(wait_status))
    {
        out->stop_signal_available = 1;
        out->stop_signal = (uint8_t)WSTOPSIG(wait_status);
        return;
    }
}

// Reads all bytes from `out_fp` (child stdout) and `err_fp` (child stderr) concurrently using `poll(2)`.
// This avoids the classic pipe-buffer deadlock that occurs when the two streams are drained sequentially:
// if the child writes more than the pipe-buffer capacity (64 KiB on Linux by default) to the stream
// the parent has not started reading yet, the child blocks on `write` and the parent blocks on `read`.
//
// On success returns 0 and writes malloc'd, NUL-terminated buffers to `*out_buf` / `*err_buf`
// (caller frees with `free`).
//
// On failure returns -1 and writes a malloc'd NUL-terminated error message to `*out_error`
// (caller frees). `*out_buf` and `*err_buf` are set to NULL on failure.
int64_t fixsubprocess_read_stdout_stderr(
    FILE *out_fp, FILE *err_fp,
    char **out_buf, char **err_buf,
    char **out_error)
{
    *out_buf = NULL;
    *err_buf = NULL;
    *out_error = NULL;

    int fds[2] = {fileno(out_fp), fileno(err_fp)};
    char *bufs[2] = {NULL, NULL};
    size_t lens[2] = {0, 0};
    size_t caps[2] = {0, 0};
    int done[2] = {0, 0};

    const size_t CHUNK = 4096;

    while (!done[0] || !done[1])
    {
        struct pollfd pfds[2];
        int idx_map[2] = {-1, -1};
        nfds_t nfds = 0;
        for (int i = 0; i < 2; i++)
        {
            if (!done[i])
            {
                pfds[nfds].fd = fds[i];
                pfds[nfds].events = POLLIN;
                pfds[nfds].revents = 0;
                idx_map[nfds] = i;
                nfds++;
            }
        }

        int pret = poll(pfds, nfds, -1);
        if (pret < 0)
        {
            if (errno == EINTR)
                continue;
            const char *msg = "poll() failed while reading subprocess stdout/stderr.";
            *out_error = (char *)malloc(strlen(msg) + 1);
            strcpy(*out_error, msg);
            free(bufs[0]);
            free(bufs[1]);
            return -1;
        }

        for (nfds_t j = 0; j < nfds; j++)
        {
            int i = idx_map[j];
            if (pfds[j].revents & (POLLIN | POLLHUP | POLLERR))
            {
                if (lens[i] + CHUNK + 1 > caps[i])
                {
                    size_t new_cap = caps[i] == 0 ? (CHUNK * 2) : caps[i] * 2;
                    while (new_cap < lens[i] + CHUNK + 1)
                        new_cap *= 2;
                    char *new_buf = (char *)realloc(bufs[i], new_cap);
                    if (new_buf == NULL)
                    {
                        const char *msg = "Out of memory while reading subprocess stdout/stderr.";
                        *out_error = (char *)malloc(strlen(msg) + 1);
                        strcpy(*out_error, msg);
                        free(bufs[0]);
                        free(bufs[1]);
                        return -1;
                    }
                    bufs[i] = new_buf;
                    caps[i] = new_cap;
                }
                ssize_t n = read(fds[i], bufs[i] + lens[i], CHUNK);
                if (n > 0)
                {
                    lens[i] += (size_t)n;
                }
                else if (n == 0)
                {
                    done[i] = 1;
                }
                else
                {
                    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                        continue;
                    const char *msg = "read() failed while reading subprocess stdout/stderr.";
                    *out_error = (char *)malloc(strlen(msg) + 1);
                    strcpy(*out_error, msg);
                    free(bufs[0]);
                    free(bufs[1]);
                    return -1;
                }
            }
        }
    }

    // Ensure each buffer has room for a trailing NUL.
    for (int i = 0; i < 2; i++)
    {
        if (caps[i] == 0)
        {
            bufs[i] = (char *)malloc(1);
            if (bufs[i] == NULL)
            {
                const char *msg = "Out of memory while finalizing subprocess output buffer.";
                *out_error = (char *)malloc(strlen(msg) + 1);
                strcpy(*out_error, msg);
                free(bufs[1 - i]);
                return -1;
            }
            caps[i] = 1;
        }
        else if (lens[i] + 1 > caps[i])
        {
            char *new_buf = (char *)realloc(bufs[i], lens[i] + 1);
            if (new_buf == NULL)
            {
                const char *msg = "Out of memory while finalizing subprocess output buffer.";
                *out_error = (char *)malloc(strlen(msg) + 1);
                strcpy(*out_error, msg);
                free(bufs[0]);
                free(bufs[1]);
                return -1;
            }
            bufs[i] = new_buf;
            caps[i] = lens[i] + 1;
        }
        bufs[i][lens[i]] = '\0';
    }

    *out_buf = bufs[0];
    *err_buf = bufs[1];
    return 0;
}