function(force_redefine_file_macro_for_sources targetname)
    get_target_property(source_files "${targetname}" SOURCES) #获取目标文件${targetname}的所有源文件列表
    foreach(sourcefile ${source_files})
        # Get source file's current list of compile definitions.

        #对于每个源文件，函数使用 get_property 命令获取当前已有的编译定义
        #使用 get_filename_component 命令获取源文件在项目目录下的相对路径，并将 __FILE__ 宏定义添加到编译定义中，以相对路径的形式表示。

        get_property(defs SOURCE "${sourcefile}"
                PROPERTY COMPILE_DEFINITIONS)
        # Get the relative path of the source file in project directory
        get_filename_component(filepath "${sourcefile}" ABSOLUTE)
        string(REPLACE .. "" relpath ${filepath})
        list(APPEND defs "__FILE__=\"${relpath}\"")
        # Set the updated compile definitions on the source file.
        # 然后，函数使用 set_property 命令将更新后的编译定义设置回源文件中
        set_property(
                SOURCE "${sourcefile}"
                PROPERTY COMPILE_DEFINITIONS ${defs}
        )
    endforeach()
endfunction()