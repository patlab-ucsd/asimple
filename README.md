# Example Meson RedBoard project

This is an example project on how to build an application for a RedBoard
Artemis ATP using the meson-ified AmbiqSDK
(https://github.com/gemarcano/AmbiqSuiteSDK).

If you are developing on Linux, I very strongly recommend you do the following:

 1. Copy the `redboard_artemis` file from the AmbiqSuiteSDK folder to
    `~/.local/share/meson/cross/` (create the directory if it doesn't exist
    `mkdir -p ~/.local/share/meson/cross/`
    - This will make it so that you can access that cross-file just by calling
      its name, e.g. `--cross-file redboard_artemis` instead of by its full
      path.

In order for the SDK library to be found, pkgconf must know where it is. The
special meson cross-file property `sys_root` is used for this, and the
`readboard_artemis` cross-file already has a shortcut for it-- it just needs a
variable to be overriden. To override a cross-file constant, you only need to
provide a second cross-file with that variable overriden. For example:

Contents of `my_cross`:
```
[constants]
prefix = '/home/gabriel/.local/redboard'
```

# Building and Flashing RedBoard
```
mkdir build
cd build
meson --prefix [prefix-where-sdk-installed] --cross-file redboard_artemis --cross-file ../my_cross --buildtype release ../
meson compile
meson flash
```

If your RedBoard Artemis is assigned a different tty device than
`/dev/ttyUSB0`, you will need to adjust the device by using `meson configure`.

# License

See the license file for details. In summary, this project is licensed
Apache-2.0, except for the bits copied from the Ambiq SDK.
