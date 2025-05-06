# true-concurrency

## Overview

true-concurrency is a C-based image processing suite that demonstrates both sequential and concurrent (multi-threaded) approaches to image manipulation. It provides utilities for blurring, flipping, rotating, inverting, and converting images to grayscale, with support for JPEG and PNG formats via the included `sod` image library. The project is designed for educational purposes, showcasing concurrency concepts and efficient image processing techniques.

## Features

- **Image Processing Operations:** Blur, flip (horizontal/vertical), rotate (90/180/270 degrees), invert colors, grayscale conversion.
- **Concurrent Processing:** Utilizes thread pools to process images in parallel for improved performance.
- **Modular Design:** Core logic is separated into reusable modules (`PicProcess`, `PicStore`, `Picture`, `Utils`, etc.).
- **Command-Line Tools:** Includes main programs for both sequential (`SeqMain.c`) and concurrent (`ConcMain.c`) execution.
- **Testing:** Ruby-based test scripts and a variety of test input/output files.
- **Image Format Support:** Supports JPEG and PNG images via the included `sod` image library.

## Directory Structure

- `*.c`, `*.h` — Core C source and header files for image processing and concurrency.
- `images/` — Example images for demonstration and testing.
- `test_images/` — Additional images for comprehensive testing.
- `test_files/` — Text files specifying test cases and expected results.
- `sod_118/` — Lightweight image library for reading/writing JPEG and PNG files.
- `extension_pic_proc_tests.rb`, `pic_proc_tests.rb` — Ruby scripts for automated testing.
- `build/` — CMake build output directory (auto-generated).

## Building

You can build the project using either `make` or CMake.

### Using Make

```sh
make
```

### Using CMake

```sh
mkdir build
cd build
cmake ..
make
```

## Command-Line Input Format

The picture processing library is invoked from the command line using the sequential main executable (`SeqMain`). The format is:

```
./SeqMain <input_path> <output_path> <process> [process_args]
```

- `<input_path>`: Path to the input image file (e.g., `images/ducks1.jpg`)
- `<output_path>`: Path to save the processed image (e.g., `images/ducks1_inverted.jpg`)
- `<process>`: The image operation to perform (see below)
- `[process_args]`: Optional arguments for certain operations

### Supported Processes

- `invert`
- `grayscale`
- `rotate 90` | `rotate 180` | `rotate 270`
- `flip H` | `flip V`
- `blur`
- `parallel-blur`

### Examples

```
./SeqMain images/ducks1.jpg images/ducks1_inverted.jpg invert
./SeqMain images/ducks1.jpg images/ducks1_gray.jpg grayscale
./SeqMain images/ducks1.jpg images/ducks1_rotated.jpg rotate 90
./SeqMain images/ducks1.jpg images/ducks1_flipped.jpg flip H
./SeqMain images/ducks1.jpg images/ducks1_blur.jpg blur
./SeqMain images/ducks1.jpg images/ducks1_pblur.jpg parallel-blur
```

This format applies only to the sequential executable. The process argument determines which image operation is performed, and some processes require an additional argument (e.g., rotation angle or flip direction).

## Input File Format

The input file specifies a sequence of image operations. See `example_input.txt` or files in `test_files/` for supported commands and syntax. Typical commands include loading, saving, blurring, flipping, rotating, inverting, and converting images to grayscale.

## Testing

Run the Ruby test scripts to validate functionality:

```sh
ruby pic_proc_tests.rb
ruby extension_pic_proc_tests.rb
```

## Example Images

- `images/` contains sample images for demonstration and quick testing.
- `test_images/` contains a comprehensive set of images for automated and manual tests.

## Dependencies

- Standard C libraries (POSIX threads for concurrency)
- No external dependencies required for building; image I/O handled by included `sod` library.
- Ruby (for running test scripts)

## License

See `sod_118/license.txt` for the image library license. The rest of the project is provided for educational use.

---

For more details, refer to the source code and comments within each module.
