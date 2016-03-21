#ifndef PROGNAME_H
#define PROGNAME_H

/* String containing name the program is called with. */
extern const char *program_name;

/* Set program_name, based on argv[0]. */
void set_program_name(const char *argv0);

#endif /* PROGNAME_H */
