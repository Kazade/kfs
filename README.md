# KFS

KFS is a C++ implementation of a Python's os and os.path modules. There are slight naming differences
for consistency.

The library is a single header and single source file. Everything is under kfs:: or kfs::path namespaces.

Currently only posix is supported, but I hope to add windows support in future.

## Examples

```
    #include <kfs/kfs.h>

    kfs::make_dir("/this/is/some/folder")

    for(auto& file: kfs::list_dir("/this/is/some/folder") {
        std::cout << file << std::endl;
    }

    kfs::touch("/this/is/some/file");

    auto path = kfs::join("/some/path", "some_file");
```
