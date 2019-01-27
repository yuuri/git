#ifndef METACOMMIT_PARSER_H
#define METACOMMIT_PARSER_H

// Indicates a normal commit (non-metacommit)
#define METACOMMIT_TYPE_NONE 0
// Indicates a metacommit with normal content (non-abandoned)
#define METACOMMIT_TYPE_NORMAL 1
// Indicates a metacommit with abandoned content
#define METACOMMIT_TYPE_ABANDONED 2

struct commit;

extern int get_metacommit_content(
	struct commit *commit, struct object_id *content);

#endif
