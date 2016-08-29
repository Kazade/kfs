

#include <functional>
#include <memory>
#include "kaztest/kaztest.h"

#include "/home/kazade/Development/kglt/deps/kfs/tests/test_kfs.h"

int main(int argc, char* argv[]) {
    std::shared_ptr<TestRunner> runner(new TestRunner());

    std::string test_case;
    if(argc > 1) {
        test_case = argv[1];
    }

    
    runner->register_case<KFSTests>(
        std::vector<void (KFSTests::*)()>({&KFSTests::test_list_dir}),
        {"KFSTests::test_list_dir"}
    );

    return runner->run(test_case);
}


