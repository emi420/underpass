## Development and debugging

### Build flags

The following flags are suggested when running the configuration for building:

`../configure CXX="ccache g++" CXXFLAGS="-std=c++17 -g -O0" CPPFLAGS="-DTIMING_DEBUG"`

### Debugging with GDB

```
sudo apt-get install gdb libtool-bin
```

Setup an alias for `gdb` in your bash profile,
adding the following line add the bottom of `~/.bashrc`:

`alias lg='libtool --mode=execute gdb'`

And you'll be able to run the debugger, for example:

`lg src/testsuite/libunderpass.all/yaml-test`

`lg --args ./underpass --bootstrap`

## Debugging in MacOS

### With GDB

It's also possible to debug Underpass on MacOS. You should use `glibtool` instead of `libtool`.

`brew install gdb`

Note that gdb might requires special privileges to access Mac ports.
You will need to codesign the binary. For instructions, see: https://sourceware.org/gdb/wiki/PermissionsDarwin

Add the alias in your `~/.bashrc` file:

`alias lg='glibtool --mode=execute gdb'`

And debug:

`lg --args ./underpass --bootstrap`

### With LLDB

When running Underpass on a MacOS system with an Arm64 architecture, `gdb` might be not available.
You can use `lldb` instead.

`lldb -- src/testsuite/libunderpass.all/yaml-test`
`lldb -- .libs/underpass --bootstrap`



