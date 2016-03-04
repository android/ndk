APP_ABI := all

# This is a different with test-googletest-stlport:
# target API level 9 since it allows the use of clone() and regex()
# in the GoogleTest implementation.
#
APP_PLATFORM := android-9
APP_STL := gnustl_shared
APP_MODULES := \
    gtest-death-test_test \
    gtest_environment_test \
    gtest-filepath_test \
    gtest-linked_ptr_test \
    gtest-listener_test \
    gtest_main_unittest \
    gtest-message_test \
    gtest_no_test_unittest \
    gtest-options_test \
    gtest-param-test_test \
    gtest-port_test \
    gtest_pred_impl_unittest \
    gtest_premature_exit_test \
    gtest-printers_test \
    gtest_prod_test \
    gtest_repeat_test \
    gtest_sole_header_test \
    gtest_stress_test \
    gtest-test-part_test \
    gtest-typed-test_test \
    gtest_unittest \
    gtest-unittest-api_test \
