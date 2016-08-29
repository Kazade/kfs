#pragma once

#include "kaztest/kaztest.h"
#include "kfs/kfs.h"

class KFSTests : public TestCase {
public:
    void set_up() {
        TestCase::set_up();

        root_ = kfs::path::join(kfs::temp_dir(), "test");
        if(kfs::path::exists(root_)) {
            kfs::remove_dirs(root_);
        }

        auto subfolder = kfs::path::join(root_, "subfolder");
        auto file1 = kfs::path::join(subfolder, "file1");
        auto file2 = kfs::path::join(subfolder, "file2");
        auto file3 = kfs::path::join(subfolder, "file3");

        kfs::make_dirs(subfolder);
        kfs::touch(file1);
        kfs::touch(file2);
        kfs::touch(file3);
    }

    void tear_down() {
        kfs::remove_dirs(root_);
    }

    void test_list_dir() {
        auto ret = kfs::path::list_dir(root_);

        assert_equal(1, ret.size());
        assert_equal(std::string("subfolder"), ret.back());

        ret = kfs::path::list_dir(kfs::path::join(root_, "subfolder"));
        assert_equal(3, ret.size());
    }

private:
    kfs::Path root_;
};
