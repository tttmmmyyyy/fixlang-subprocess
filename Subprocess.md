# `module Subprocess`

# Types and aliases

## `namespace Subprocess`

### `type ExitStatus = box union { ...variants... }`

This type represents the exit status of a subprocess.

This type is the union of following variants:
* `exit : U8` - Means that the subprocess successfully exited (i.e., the main function returned or `exit()` was called) and stores the exit status code.
* `signaled : U8` - Means that the subprocess was terminated by a signal and stores the signal number which caused the termination.
* `wait_failed : ()` - Means that the `run*` function failed to wait the subprocess to exit.

#### variant `exit : Std::U8`

#### variant `signaled : Std::U8`

#### variant `wait_failed : ()`

# Traits and aliases

# Trait implementations

# Values

## `namespace Subprocess`

### `run_string : Std::String -> Std::Array Std::String -> Std::String -> Std::IO::IOFail ((Std::String, Std::String), Subprocess::ExitStatus)`

`run_string(com, args, input)` executes a command specified by `com` with arguments `args`, and writes `input` to the standard input of the running command.

The result is the pair of standard output and standard error, and an `ExitStatus` value.

### `run_with_stream : Std::String -> Std::Array Std::String -> ((Std::IO::IOHandle, Std::IO::IOHandle, Std::IO::IOHandle) -> Std::IO::IOFail a) -> Std::IO::IOFail (a, Subprocess::ExitStatus)`

`run_with_stream(com, args, worker)` executes a command specified by `com` with arguments `args`.

The function `worker` receives three `IOHandle`s which are piped to the stdin, stdout and stderr of the running command.

The result is the value returned by `worker` paired with an `ExitStatus` value.
* `com : String` - The path to the program to run.
* `args: Array String` - The arguments to be passed to `com`.
* `worker : (IOHandle, IOHandle, IOHandle) -> IOFail a` - Receives three `IOHandle`s which are piped to stdin, stdout and stderr of the running command.

## `namespace Subprocess::ExitStatus`

### `as_exit : Subprocess::ExitStatus -> Std::U8`

Unwraps a union value of `ExitStatus` as the variant `exit`.
If the value is not the variant `exit`, this function aborts the program.

### `as_signaled : Subprocess::ExitStatus -> Std::U8`

Unwraps a union value of `ExitStatus` as the variant `signaled`.
If the value is not the variant `signaled`, this function aborts the program.

### `as_wait_failed : Subprocess::ExitStatus -> ()`

Unwraps a union value of `ExitStatus` as the variant `wait_failed`.
If the value is not the variant `wait_failed`, this function aborts the program.

### `exit : Std::U8 -> Subprocess::ExitStatus`

Constructs a value of union `ExitStatus` taking the variant `exit`.

### `is_exit : Subprocess::ExitStatus -> Std::Bool`

Checks if a union value of `ExitStatus` is the variant `exit`.

### `is_signaled : Subprocess::ExitStatus -> Std::Bool`

Checks if a union value of `ExitStatus` is the variant `signaled`.

### `is_wait_failed : Subprocess::ExitStatus -> Std::Bool`

Checks if a union value of `ExitStatus` is the variant `wait_failed`.

### `mod_exit : (Std::U8 -> Std::U8) -> Subprocess::ExitStatus -> Subprocess::ExitStatus`

Updates a value of union `ExitStatus` by applying a function if it is the variant `exit`, or doing nothing otherwise.

### `mod_signaled : (Std::U8 -> Std::U8) -> Subprocess::ExitStatus -> Subprocess::ExitStatus`

Updates a value of union `ExitStatus` by applying a function if it is the variant `signaled`, or doing nothing otherwise.

### `mod_wait_failed : (() -> ()) -> Subprocess::ExitStatus -> Subprocess::ExitStatus`

Updates a value of union `ExitStatus` by applying a function if it is the variant `wait_failed`, or doing nothing otherwise.

### `signaled : Std::U8 -> Subprocess::ExitStatus`

Constructs a value of union `ExitStatus` taking the variant `signaled`.

### `wait_failed : () -> Subprocess::ExitStatus`

Constructs a value of union `ExitStatus` taking the variant `wait_failed`.