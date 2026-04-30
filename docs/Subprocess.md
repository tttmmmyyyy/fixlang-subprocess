# Subprocess

Defined in subprocess@1.1.2

## Values

### namespace Subprocess

#### read_stdout_stderr_concurrent

Type: `Std::IO::IOHandle -> Std::IO::IOHandle -> Std::IO::IOFail (Std::String, Std::String)`

Reads the stdout and stderr of a child process concurrently and returns them as a pair of strings.

Reading them sequentially (drain all of stdout, then all of stderr) can deadlock when the child
writes a large amount to the stream the parent has not started reading yet. This function
drains both streams in parallel and avoids that.

`run_string` already does this internally for the common "feed input, collect both streams"
case. Use this helper directly when you need a `run_with_stream` worker that does something
non-trivial alongside the I/O — for example, post-processing one stream while still keeping
the other from blocking the child:

```
run_with_stream("some-cmd", args, |(stdin, stdout, stderr)| (
    close_file(stdin).lift;;                           // No input.
    let (out, err) = *read_stdout_stderr_concurrent(stdout, stderr);
    // Custom logic on `out` / `err` here, e.g. parsing, validating, decoding, ...
    pure $ classify(out, err)
))
```

##### Parameters

* `stdout` - The stdout `IOHandle` of the child process.
* `stderr` - The stderr `IOHandle` of the child process.

#### run_string

Type: `Std::String -> Std::Array Std::String -> Std::String -> Std::IO::IOFail ((Std::String, Std::String), Subprocess::ExitStatus)`

`run_string(com, args, input)` executes a command specified by `com` with arguments `args`, and writes `input` to the standard input of the running command.

The result is the pair of standard output and standard error, and an `ExitStatus` value.

##### Parameters

* `com` - The path to the program to run.
* `args` - The arguments to be passed to `com`.
* `input` - The string to write to the standard input of the running command.

#### run_with_stream

Type: `Std::String -> Std::Array Std::String -> ((Std::IO::IOHandle, Std::IO::IOHandle, Std::IO::IOHandle) -> Std::IO::IOFail a) -> Std::IO::IOFail (a, Subprocess::ExitStatus)`

`run_with_stream(com, args, worker)` executes a command specified by `com` with arguments `args`.

The function `worker` receives three `IOHandle`s which are piped to the stdin, stdout and stderr of the running command.

The result is the value returned by `worker` paired with an `ExitStatus` value.

##### Pipe-buffer deadlock

If `worker` reads stdout and stderr sequentially (e.g. `read_string(stdout)` followed by
`read_string(stderr)`), the call may deadlock when the child writes a large amount to
the stream that has not been drained yet. To safely consume both streams, either:

* call `read_stdout_stderr_concurrent(stdout, stderr)` to drain both in parallel; or
* read only one of the streams (and let the other one stay small / be redirected by the child).

`run_string` already takes care of this internally.

##### Parameters

* `com` - The path to the program to run.
* `args` - The arguments to be passed to `com`.
* `worker` - Receives three `IOHandle`s which are piped to stdin, stdout and stderr of the running command.

## Types and aliases

### namespace Subprocess

#### ExitStatus

Defined as: `type ExitStatus = box union { ...variants... }`

This type represents the exit status of a subprocess.

This type is the union of following variants:

* `exit : U8` - Means that the subprocess successfully exited (i.e., the main function returned or `exit()` was called) and stores the exit status code.
* `signaled : U8` - Means that the subprocess was terminated by a signal and stores the signal number which caused the termination.
* `wait_failed : ()` - Means that the `run*` function failed to wait the subprocess to exit.

##### variant `exit`

Type: `Std::U8`

##### variant `signaled`

Type: `Std::U8`

##### variant `wait_failed`

Type: `()`

#### WaitResult

Defined as: `type WaitResult = box struct { ...fields... }`

Result of `fixsubprocess_wait_subprocess`.

This struct is used in the internal implementation.

##### field `is_timeout`

Type: `Std::U8`

##### field `wait_failed`

Type: `Std::U8`

##### field `exit_status`

Type: `Std::U8`

##### field `exit_status_available`

Type: `Std::U8`

##### field `stop_signal`

Type: `Std::U8`

##### field `stop_signal_available`

Type: `Std::U8`

## Traits and aliases

## Trait implementations