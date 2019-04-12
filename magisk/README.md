# Foxy Boot

Android boot animation replacement which displays the kernel ring buffer.

![Screenshot](https://user-images.githubusercontent.com/24494863/55270348-a4343380-5295-11e9-8bbe-aa6e65546ea4.png)

## Configuration

Foxy Boot uses black background color and white foreground color by default. The font scale is dependent on display DPI.
These values can be changed using the following system properties:

* `foxy.boot.scale` — scale factor, can take integer values from 1 to 9
* `foxy.boot.background` ­— background color in hex format (e.g. #000000)
* `foxy.boot.foreground` ­— foreground color in hex format (e.g. #ffffff)

These properties can be changed using init shell script. For instance, you can create `foxy-boot.sh` in
`/sbin/.magisk/img/.core/post-fs-data.d` (don't forget to make it executable via `chmod a+x`) with the following
commands:

```sh
resetprop 'foxy.boot.scale' 2
resetprop 'foxy.boot.background' '#ffffff'
resetprop 'foxy.boot.foreground' '#000000'
```

## ABI and Linking Issues

Foxy Boot works with private Android API which is not available in NDK. It's dynamically linked against stub libraries
which should resemble the real libraries in the system, however successful linking and full ABI compatibility on the
real Android system cannot be guaranteed.

The program is also dynamically linked against `libc++.so`, which may be unavailable on some Android devices.

The installer script will perform necessary checks and patch the binaries accordingly. It will also report linker errors
so they could be reported at my [GitHub repo](https://github.com/kitsunyan/foxy-boot).

## License

Foxy Boot is available under the terms of GNU General Public License v3 or later. Copyright (C) 2019 kitsunyan.

The program is partially based on `bootanimation` from the AOSP. Copyright (C) 2007 The Android Open Source Project.
