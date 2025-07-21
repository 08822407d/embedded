/* 放在 IRAM_D, 跟常规 rodata 一样 */
.shell_cmd :
{
	_shell_command_start = ABSOLUTE(.);
	KEEP(*(.shell_cmd*))
	_shell_command_end   = ABSOLUTE(.);
} > dram0_0_seg