#include "strhashmap.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define INVALID_INDEX 0x7FFFFFFF

enum strhashmap_entry_state {
    HASHMAP_NULL = '\0',
    HASHMAP_TOMBSTONE = '\1',
    HASHMAP_FULL = '\2',
};

typedef struct bk_strhashmap_entry {
    char eState;
    void* pValue;
    bk_strhashmapkey pKey;
} bk_strhashmap_entry;

typedef struct bk_strhashmap_t {
    size_t cCapacity;
    size_t cSize;
    bk_strhashmap_entry* pEntries;
} bk_strhashmap_t;

static size_t hash_string(const char* str) {
    size_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; 
    }
    return hash;
}

static void probe_linearly_idx(bk_strhashmap map, size_t* index, const char* key, int returnEmptyEntries) {
    assert(map);

    for (size_t i = 0; i < map->cCapacity; ++i) {
        bk_strhashmap_entry* pEntry = map->pEntries + *index;
        switch (pEntry->eState) {
        case HASHMAP_NULL:
            if (!returnEmptyEntries) {
                *index = INVALID_INDEX;
            }
            return;
        case HASHMAP_FULL:
            if (strcmp(key, pEntry->pKey) == 0) {
                return;
            }
            break;
        case HASHMAP_TOMBSTONE:
            if (returnEmptyEntries) {
                return;
            }
            break;
        }
        size_t nextIdx = (*index) + 1;
        if (nextIdx >= map->cCapacity) {
            nextIdx = 0;
        }
        *index = nextIdx;
    }
    *index = INVALID_INDEX;
}

static size_t get_strhashmap_entry_index(bk_strhashmap map, const char* key, int returnEmptyEntries) {
    assert(map);
    size_t hash = hash_string(key);
    size_t index = hash % map->cCapacity;
    probe_linearly_idx(map, &index, key, returnEmptyEntries);
    return index;
}

static void grow_strhashmap(bk_strhashmap map) {
    assert(map);
    size_t oldCapacity = map->cCapacity;
    bk_strhashmap_entry* pOldEntries = map->pEntries;

    map->cCapacity *= 2;
    map->cSize = 0;
    map->pEntries = calloc(map->cCapacity, sizeof(bk_strhashmap_entry));

    for (size_t i = 0; i < oldCapacity; ++i) {
        bk_strhashmap_entry* pEntry = pOldEntries + i;
        if (pEntry->eState == HASHMAP_FULL) {
            size_t newIndex = get_strhashmap_entry_index(map, pEntry->pKey, 1);
            assert(newIndex != INVALID_INDEX && "this should never happen!!");
            map->pEntries[newIndex].eState = HASHMAP_FULL;
            map->pEntries[newIndex].pKey = pEntry->pKey; 
            map->pEntries[newIndex].pValue = pEntry->pValue;
            map->cSize++;
        }
    }
    free(pOldEntries);
}

bk_strhashmap bk_internal_create_strhashmap(void) {
    bk_strhashmap map = (bk_strhashmap)malloc(sizeof(bk_strhashmap_t));
    if (!map) return NULL;

    map->cCapacity = 16;
    map->cSize = 0;
    map->pEntries = calloc(map->cCapacity, sizeof(bk_strhashmap_entry));
    return map;
}

void bk_internal_free_strhashmap(bk_strhashmap map) {
    if (map) {
        for (size_t i = 0; i < map->cCapacity; ++i) {
            if (map->pEntries[i].eState == HASHMAP_FULL) {
                free((void*)map->pEntries[i].pKey);
            }
        }
        free(map->pEntries);
        free(map);
    }
}

void* bk_internal_strhashmap_put(bk_strhashmap map, bk_strhashmapkey key, void* value) {
    assert(map && "don't pass a null map");

    if ((double)map->cSize >= map->cCapacity * 0.75) {
        grow_strhashmap(map);
    }
    size_t index = get_strhashmap_entry_index(map, key, 1);
    assert(index != INVALID_INDEX && "grown maps should always have space!");

    bk_strhashmap_entry* pEntry = map->pEntries + index;
    if (pEntry->eState != HASHMAP_FULL) {
        pEntry->eState = HASHMAP_FULL;
        pEntry->pKey = strdup(key);
        map->cSize++;
    }
    void* old = pEntry->pValue;
    pEntry->pValue = value;
    return old;
}

void bk_internal_strhashmap_remove(bk_strhashmap map, bk_strhashmapkey key) {
    assert(map && "don't pass a null map");

    size_t index = get_strhashmap_entry_index(map, key, 0);
    if (index == INVALID_INDEX) {
        return;
    }

    bk_strhashmap_entry* pEntry = map->pEntries + index;
    free((void*)pEntry->pKey);
    pEntry->pKey = NULL; 
    pEntry->eState = HASHMAP_TOMBSTONE;
    map->cSize--;
}

const void* bk_internal_strhashmap_get_ptr(bk_strhashmap map, bk_strhashmapkey key) {
    assert(map && "don't pass a null map");

    size_t index = get_strhashmap_entry_index(map, key, 0);
    if (index == INVALID_INDEX) {
        return NULL;
    }
    bk_strhashmap_entry* pEntry = map->pEntries + index;
    return pEntry->pValue;
}

struct bk_strhashmap_iter bk_internal_strhashmap_iter_init(void) {
    struct bk_strhashmap_iter iter = { 0 };
    iter._internal_index = 0;
    iter.key = NULL;
    iter.value = NULL;
    return iter;
}

int bk_internal_strhashmap_iter_next(bk_strhashmap map, struct bk_strhashmap_iter* iter) {
    if (!map || !iter) return 0;

    while (iter->_internal_index < map->cCapacity) {
        bk_strhashmap_entry* pEntry = map->pEntries + iter->_internal_index;

        if (pEntry->eState == HASHMAP_FULL) {
            iter->key = pEntry->pKey;
            iter->value = pEntry->pValue;

            iter->_internal_index++;
            return 1; 
        }
        iter->_internal_index++;
    }

    return 0; 
}