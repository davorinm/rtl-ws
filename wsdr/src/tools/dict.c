#include "dict.h"
// https://stackoverflow.com/questions/4384359/quick-way-to-implement-dictionary-in-c

typedef struct dict_entry_s
{
    const char *key;
    int value;
} dict_entry_s;

typedef struct dict_s
{
    int len;
    int cap;
    dict_entry_s *entry;
} dict_s, *dict_t;

static int dict_find_index(dict_t dict, const char *key)
{
    for (int i = 0; i < dict->len; i++)
    {
        if (!strcmp(dict->entry[i], key))
        {
            return i;
        }
    }
    return -1;
}

int dict_value(dict_t dict, const char *key)
{
    int idx = dict_find_index(dict, key);
    if (idx < 0)
    {
        return -1;
    }
    return dict->entry[idx].value;
}

void dict_add(dict_t dict, const char *key, int value)
{
    int idx = dict_find_index(dict, key);
    if (idx != -1)
    {
        dict->entry[idx].value = value;
        return;
    }
    if (dict->len == dict->cap)
    {
        dict->cap *= 2;
        dict->entry = realloc(dict->entry, dict->cap * sizeof(dict_entry_s));
    }
    dict->entry[dict->len].key = strdup(key);
    dict->entry[dict->len].value = value;
    dict->len++;
}

int dict_count(dict_t dict)
{
    return dict->len;
}

dict_t dict_new(int count)
{
    dict_s proto = {0, count, malloc(count * sizeof(dict_entry_s))};
    dict_t d = malloc(sizeof(dict_s));
    *d = proto;
    return d;
}

void dict_free(dict_t dict)
{
    for (int i = 0; i < dict->len; i++)
    {
        free(dict->entry[i].key);
    }
    free(dict->entry);
    free(dict);
}