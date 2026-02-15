# Codrut-Subtitle-Editor
 A simple, fast and efficient command-line program written in C to edit SRT files

Supports copying, removing, offsetting, and progressive (linear) time
shifting of subtitles with flexible time parsing.


---

## Features

-   Read and write standard `.srt` files
-   Multiple time input formats:
    -   `hh:mm`
    -   `hh:mm:ss`
    -   `hh:mm:ss,ms`
    -   `12.5s`, `3m`, `1.2h`
-   Offset subtitles by fixed time
-   Linear progressive offset
-   Remove subtitle ranges
-   Optional index rebuild
-   Optional chronological sorting (`--fix-order`)
-   Blazing fast with no dependencies (pure C, standard library)

---

## Compilation / Installation

Linux / macOS:
``` bash
gcc -O2 -Wall -o srt-edit main.c
sudo mv srt-edit /usr/bin 
```

---

## Usage

``` bash
srt-edit [options]
```

## Command Reference
| Category        | Name                     | Option / Mode                  | Description                                            |
|:----------------|:-------------------------|:-------------------------------|:-------------------------------------------------------|
| Information     | Help                     | --help, -h                     | Display help information and commands                  |
| Information     | Usage                    | --usage                        | Display commands and examples                          |
| Information     | Version                  | --version, -v                  | Show version information                               |
| Information     | Verbose                  | --verbose                      | Show verbose info                                      |
| Input / Output  | Input File               | -i, --in <file path>           | File to read from (Required)                           |
| Input / Output  | Output File              | -o, --out <file path>          | File to write to (Defaults to input file)              |
| Input / Output  | Yes                      | -y, --yes                      | Do not ask for confirmations                           |
| Operation Mode  | Copy (Default)           | -oc, --operation-copy          | Copy subtitle to destination                           |
| Operation Mode  | Remove Range             | -or, --operation-remove        | Remove subtitle range                                  |
| Operation Mode  | Fixed Offset             | -oo, --operation-offset        | Offset subtitles in range                              |
| Operation Mode  | Linear Offset            | -olo, --operation-linear-offset| Offset range linearly based on time value              |
| Data Parameters | Fixed Offset Value       | --offset <time>                | Time value for fixed offset mode                       |
| Data Parameters | Linear Offset Start      | --offset-from <time>           | Starting offset for linear mode                        |
| Data Parameters | Linear Offset End        | --offset-to <time>             | Ending offset for linear mode                          |
| Range Options   | From Time                | --from <time>                  | Start range from specific time                         |
| Range Options   | To Time                  | --to <time>                    | End range at specific time                             |
| Range Options   | From Index               | --from-index <integer>         | Start range from subtitle index                        |
| Range Options   | To Index                 | --to-index <integer>           | End range at subtitle index                            |
| Range Options   | Full Range               | --range-full                   | Apply operation to entire file                         |
| Output Options  | No Rebuild Index         | -nix, --no-rebuild-index       | Do not re-build subtitle indices after modification    |
| Output Options  | Fix Order                | --fix-order                    | Sort data chronologically to conform to SRT standard   |

## Examples
`--verbose` is not required, I added it in the examples so you can see the inner-workings of the program in more detail.

### Basic Copy

``` bash
srt-edit -i movie.srt -o out.srt
```

### Remove First 20 Subtitles

``` bash
srt-edit --verbose -i movie.srt -o out.srt -or --from-index 1 --to-index 20
```

### Shift Entire File by 3.5 Seconds

``` bash
srt-edit --verbose -i movie.srt -o out.srt -oo --offset 3.5s --range-full
```

### Shift Everythinf after 15m  by 3.5 Seconds

``` bash
srt-edit --verbose -i movie.srt -o out.srt -oo --offset 3.5s --from 15m
```
When the end is not provided, the program will ask you if you'd like to select the destination as the end of the file. To skip the confirmation and select the end without a prompt, provide `--yes`.

#### Output
<img width="822" height="455" alt="image" src="https://github.com/user-attachments/assets/bf324fce-4334-4d8e-9a9d-ba93ef7a4ba9" />

### Progressive Sync Adjustment

``` bash
srt-edit --verbose -i movie.srt -o out.srt -olo --offset-from -2s   --offset-to 4s   --from-index 1   --to-index 500
```


## Time Format Interpreter

Accepted formats:

    hh:mm
    hh:mm:ss
    hh:mm:ss,ms
    X.Xs
    X.Xm
    X.Xh

Examples:

    12.5s
    1.2m
    00:10:35,500
