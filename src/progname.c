#include "progname.h"

/*
 * String containing name the program is called with.
 * To be initialized by main().
 */
const char *program_name;

/*
 * Set program_name, based on argv[0].
 */
void
set_program_name(const char *argv0)
{
	program_name = argv0;
}
