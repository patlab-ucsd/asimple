# asimple: Sparkfun Artemis simple library

This is a collection of wrappers around the AmbiqSuiteSDK to simplify the use
of some common tasks. This relies on a fork of the Sparkfun AmbiqSuiteSDK fork
that uses meson for its build system
(https://github.com/gemarcano/AmbiqSuiteSDK).

If you are developing on Linux, it is very strongly recommended you do the
following:

 1. Copy the `artemis` file from the AmbiqSuiteSDK project to
    `~/.local/share/meson/cross/` (create the directory if it doesn't exist
    `mkdir -p ~/.local/share/meson/cross/`
    - This will make it so that you can access that cross-file just by calling
      its name, e.g. `--cross-file artemis` instead of by its full
      path.

In order for the SDK library to be found, `pkgconf` must know where it is. The
special meson cross-file property `sys_root` is used for this, and the
`artemis` cross-file already has a shortcut for it-- it just needs a
variable to be overriden. To override a cross-file constant, you only need to
provide a second cross-file with that variable overriden. For example:

Contents of `my_cross`:
```
[constants]
prefix = '/home/gabriel/.local/redboard'
```

# Dependencies

 - [AmmbiqSuiteSDK](https://github.com/gemarcano/AmbiqSuiteSDK)
 - [littlefs](https://github.com/littlefs-project/littlefs)

# Compiling and installing
```
mkdir build
cd build
meson setup --prefix [prefix-where-sdk-installed] --cross-file artemis --cross-file ../my_cross --buildtype release
meson install
```

# License

See the license file for details. In summary, this project is licensed
Apache-2.0, except for the bits copied from the Ambiq SDK.
