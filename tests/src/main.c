#include "expect.h"
#include "test_manager.h"
#include "memory/linear_allocator_test.h"
int main() {
    // Always initalize the test manager first.
    test_manager_init();

    // TODO: add test registrations here.
    linear_allocator_register_tests();


    KDEBUG("Starting tests...");

    // Execute tests
    test_manager_run_tests();

    return 0;
} 