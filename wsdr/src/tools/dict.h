#ifndef DICT_H
#define DICT_H

int dict_value(dict_t dict, const char *key);

void dict_add(dict_t dict, const char *key, int value);

int dict_count();

dict_t dict_new(int count);

void dict_free(dict_t dict);

#endif