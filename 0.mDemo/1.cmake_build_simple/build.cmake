# func:
#     1. Generate kbuild file
#     2. Create target and add to all, to execute make command
FUNCTION(ADD_KBUILD MODULE_NAME)
    SET(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

    SET(MODULE_KO "${MODULE_NAME}.ko")
    SET(MODULE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    SET(MODULE_CFLAG "")
    SET(MODULE_OBJS "")
    SET(MODULE_BYPRODUCTS "")
    SET(MODULE_EXTRA_SYM "")

    FILE(MAKE_DIRECTORY ${MODULE_DIR})


    # KERNEL_DIR
    EXECUTE_PROCESS(COMMAND uname -r
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE  KERNEL_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    SET(KERNEL_DIR "/lib/modules/${KERNEL_DIR}/build")
    MESSAGE(STATUS "KERNEL_DIR: ${KERNEL_DIR}")


    # proc CFLAG
    GET_DIRECTORY_PROPERTY(DIR_DEFS COMPILE_DEFINITIONS)
    MESSAGE(STATUS "CMP DIR_DEFS: ${DIR_DEFS}")
    FOREACH(d ${DIR_DEFS})
        SET(MODULE_CFLAG "${MODULE_CFLAG} -D${d}")
    ENDFOREACH()
    MESSAGE(STATUS "MODULE_CFLAG: ${MODULE_CFLAG}")

    GET_DIRECTORY_PROPERTY(DIR_DEFS INCLUDE_DIRECTORIES )
    MESSAGE(STATUS "INC DIR_DEFS: ${DIR_DEFS}")
    FOREACH(d ${DIR_DEFS})
        SET(MODULE_CFLAG "${MODULE_CFLAG} -I${d}")
    ENDFOREACH()
    MESSAGE(STATUS "MODULE_CFLAG: ${MODULE_CFLAG}")


    # collect source file
    FILE(GLOB MODULE_SOURCE_FILES "${SOURCE_DIR}/*.c")
    MESSAGE(STATUS "SOURCE_FILES: ${MODULE_SOURCE_FILES}")


    # proc MODULE_OBJS
    FOREACH(src ${MODULE_SOURCE_FILES})
        IF(NOT IS_ABSOLUTE ${src})
            SET(src "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
        ENDIF()

        GET_FILENAME_COMPONENT(name ${src} NAME)
        SET(dst "${MODULE_DIR}/${name}")

        EXECUTE_PROCESS(COMMAND bash -c "ln -sf ${src} ${dst}")

        # NAME_WE 中 WE 代表 “Without Extension”，即“无扩展名”
        GET_FILENAME_COMPONENT(name_we ${src} NAME_WE)
        SET(obj "${name_we}.o")

        STRING(APPEND MODULE_OBJS "${obj} ")

        LIST(APPEND MODULE_BYPRODUCTS
            "${MODULE_DIR}/${obj}"
            "${MODULE_DIR}/${obj}.cmd"
        )
    ENDFOREACH()
    MESSAGE(STATUS "MODULE_OBJS: ${MODULE_OBJS}")

    # MODULE_EXTRA_SYM
    # ARGV 是 CMake 中的一个特殊变量，它包含了传递给当前函数或宏的所有参数。
    # ARGV 是一个列表，其中包含了按顺序传递给函数或宏的每个参数。
    SET(module_deps ${ARGV})
    LIST(REMOVE_AT module_deps 0)

    FOREACH(d ${module_deps})
        GET_PROPERTY(dep_dir TARGET ${d} PROPERTY LOCATION)
        STRING(APPEND MODULE_EXTRA_SYM "${dep_dir}/Module.symvers ")
    ENDFOREACH()

    # MODULE_BYPRODUCTS
    LIST(APPEND MODULE_BYPRODUCTS
        "${MODULE_DIR}/.${MODULE_KO}.cmd"
        "${MODULE_DIR}/.${MODULE_NAME}.mod.o.cmd"
        "${MODULE_DIR}/.${MODULE_NAME}.o.cmd"
        "${MODULE_DIR}/.built-in.o.cmd"
        "${MODULE_DIR}/${MODULE_NAME}.mod.c"
        "${MODULE_DIR}/${MODULE_NAME}.mod.o"
        "${MODULE_DIR}/${MODULE_NAME}.o"
        "${MODULE_DIR}/built-in.o"
        "${MODULE_DIR}/Module.symvers"
        "${MODULE_DIR}/modules.order"
        )

    MESSAGE(STATUS "MODULE_BYPRODUCTS: ${MODULE_BYPRODUCTS}")
    MESSAGE(STATUS "KERNEL_MAKE_OPT: ${KERNEL_MAKE_OPT}")

    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/Kbuild.in ${MODULE_DIR}/Kbuild @ONLY)

    SET(KBUILD_CMD
        PATH=$ENV{PATH}
        make # ${CMAKE_MAKE_PROGRAM}
        -C ${KERNEL_DIR}
        ${KERNEL_MAKE_OPT}
        M=${MODULE_DIR}
        src=${MODULE_DIR}
        modules
        )
    MESSAGE(STATUS "KBUILD_CMD: ${KBUILD_CMD}")

    # BYPRODUCTS 参数告诉 CMake，自定义命令会生成哪些额外的文件，这样 CMake 可以
    # 追踪这些文件的状态，确保它们是最新的。这对于构建系统的依赖关系管理非常重要，
    # 因为 CMake 需要知道何时需要重新执行自定义命令
    ADD_CUSTOM_COMMAND(
        COMMAND ${KBUILD_CMD}
        COMMENT "Building ${MODULE_KO}"
        OUTPUT ${MODULE_DIR}/${MODULE_KO}
        DEPENDS ${MODULE_SOURCE_FILES}
        WORKING_DIRECTORY ${MODULE_DIR}
        BYPRODUCTS ${MODULE_BYPRODUCTS}
        VERBATIM
        )

    ADD_CUSTOM_TARGET(
        ${MODULE_NAME} ALL
        DEPENDS ${MODULE_DIR}/${MODULE_KO}
        )

    # clean for others module
    SET_TARGET_PROPERTIES(
        ${MODULE_NAME}
        PROPERTIES
        LOCATION            ${MODULE_DIR}
        OUTPUT_NAME         ${MODULE_NAME}
        MODULE_KO           ""
        MODULE_DIR          ""
        MODULE_CFLAG        ""
        MODULE_OBJS         ""
        MODULE_BYPRODUCTS   ""
        MODULE_EXTRA_SYM    ""
        )
ENDFUNCTION()
