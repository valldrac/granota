#ifndef INPUT_H
#define INPUT_H

#include "files.h"

struct input {
	unsigned fileno;
	struct files files;
	int semid;
	size_t critbit;
	double score[FILES_MAX];
	int tries[FILES_MAX];
};

static inline char *
get_input_path(char *buf, struct input *in)
{
	return files_path(&in->files, buf, in->fileno);
}

#endif /* INPUT_H */
