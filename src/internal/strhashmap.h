#ifndef _BLINK_INTERNAL_STRHASHMAP_H_
#define _BLINK_INTERNAL_STRHASHMAP_H_
#include <stddef.h>

typedef struct bk_strhashmap_t* bk_strhashmap;

typedef const char* bk_strhashmapkey;

bk_strhashmap bk_internal_create_strhashmap(void);
void bk_internal_free_strhashmap(bk_strhashmap map);

// returns the old value if it exists
void* bk_internal_strhashmap_put(bk_strhashmap map, bk_strhashmapkey key, void* value);
void bk_internal_strhashmap_remove(bk_strhashmap map, bk_strhashmapkey key);
const void* bk_internal_strhashmap_get_ptr(bk_strhashmap map, bk_strhashmapkey key);

#define bk_internal_strhashmap_get(pType, map, key) (pType)bk_internal_strhashmap_get_ptr(map, key)

struct bk_strhashmap_iter {
    size_t _internal_index;
    bk_strhashmapkey key;
    void* value;
};

struct bk_strhashmap_iter bk_internal_strhashmap_iter_init(void);
int bk_internal_strhashmap_iter_next(bk_strhashmap map, struct bk_strhashmap_iter* iter);

#define bk_internal_strhashmap_for_each(map, iter_name) \
    for (struct bk_strhashmap_iter iter_name = bk_internal_strhashmap_iter_init(); \
         bk_internal_strhashmap_iter_next((map), &(iter_name)); )

#endif // _BLINK_INTERNAL_STRHASHMAP_H_