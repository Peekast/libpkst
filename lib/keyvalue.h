#ifndef _KEYVALUE_H
#define _KEYVALUE_H 1


typedef struct KeyValue {
    char *key;
    char *value;
} KeyValue;

typedef struct KeyValueList {
    KeyValue *items;
    int count;
} KeyValueList;

extern KeyValueList* parse_kv_list(const char *kv_str, char pair_delim, char kv_delim);
extern void free_kv_list(KeyValueList* kv_list);
extern void dump_kv_list(const KeyValueList *kv_list);
extern char* serialize_kv_list(const KeyValueList* kv_list, char pair_delim, char kv_delim);
extern int add_to_kv_list(KeyValueList **kv_list, const char *key, const char *value);
#endif