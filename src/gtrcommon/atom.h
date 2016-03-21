#ifndef ATOM_H
#define ATOM_H

#define ATOM_MAX	666
#define SET_BUFSZ	8000

struct atom {
	int len;
	unsigned hash;
	char *str;
};

struct set {
	int bufsz;
	struct atom member[ATOM_MAX];
	char buf[SET_BUFSZ];
};

unsigned atom_hash(const char *str, int len);
int atom_new(struct set *set, const char *str, int len, unsigned hash);
struct atom *atom_member(struct set *set, int i);
void atom_empty(struct set *set);

#endif /* ATOM_H */
