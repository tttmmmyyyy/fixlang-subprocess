# Subprocess

## Values

### namespace Subprocess

#### run_string

Type: `Std::String -> Std::Array Std::String -> Std::String -> Std::IO::IOFail ((Std::String, Std::String), Subprocess::ExitStatus)`

`run_string(com, args, input)` executes a command specified by `com` with arguments `args`, and writes `input` to the standard input of the running command.

The result is the pair of standard output and standard error, and an `ExitStatus` value.

#### run_with_stream

Type: `Std::String -> Std::Array Std::String -> ((Std::IO::IOHandle, Std::IO::IOHandle, Std::IO::IOHandle) -> Std::IO::IOFail a) -> Std::IO::IOFail (a, Subprocess::ExitStatus)`

`run_with_stream(com, args, worker)` executes a command specified by `com` with arguments `args`.

The function `worker` receives three `IOHandle`s which are piped to the stdin, stdout and stderr of the running command.

The result is the value returned by `worker` paired with an `ExitStatus` value.
- `com : String`: The path to the program to run.
- `args: Array String`: The arguments to be passed to `com`.
- `worker : (IOHandle, IOHandle, IOHandle) -> IOFail a`: Receives three `IOHandle`s which are piped to stdin, stdout and stderr of the running command.

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