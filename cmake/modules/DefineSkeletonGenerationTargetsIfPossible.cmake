# Copyright 2011 Alex Turbov <i.zaufi@gmail.com>

# check if autogen is even installed
find_program(AUTOGEN_EXECUTABLE autogen)
if (AUTOGEN_EXECUTABLE)

    # prepare autogen wrapper for new class skeleton creation
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/support/new_class_wrapper.sh.in
        ${CMAKE_BINARY_DIR}/cmake/support/new_class_wrapper.sh
      )
    # prepare autogen wrapper for unit tests
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/support/new_class_tester_wrapper.sh.in
        ${CMAKE_BINARY_DIR}/cmake/support/new_class_tester_wrapper.sh
      )

    # add new-class as target
    add_custom_target(new-class /bin/sh ${CMAKE_BINARY_DIR}/cmake/support/new_class_wrapper.sh)
    add_dependencies(new-class ${CMAKE_BINARY_DIR}/cmake/support/new_class_wrapper.sh)
    add_dependencies(new-class ${CMAKE_SOURCE_DIR}/cmake/support/class.tpl)

    # add new-class-tester as target
    add_custom_target(new-class-tester /bin/sh ${CMAKE_BINARY_DIR}/cmake/support/new_class_tester_wrapper.sh)
    add_dependencies(new-class-tester ${CMAKE_BINARY_DIR}/cmake/support/new_class_tester_wrapper.sh)
    add_dependencies(new-class-tester ${CMAKE_SOURCE_DIR}/cmake/support/class_tester.tpl)

endif (AUTOGEN_EXECUTABLE)
