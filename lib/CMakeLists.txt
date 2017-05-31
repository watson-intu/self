include_directories(".")

add_subdirectory(cpp-sdk)

qi_create_lib(sqlite STATIC
	sqlite/shell.c
	sqlite/sqlite3.c)
qi_stage_lib(sqlite)
