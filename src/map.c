/*
 * The MIT License
 *
 * Copyright 2016-2018 Kirill Scherba <kirill@scherba.ru>.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * \file   map.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * Created on June 6, 2016, 12:26 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "map.h"
#include "hash.h"
//#include "packet.h"

// Local functions
static void *_trudpMapGet(trudpMapData *map, void *key, size_t key_length,
        uint32_t hash, size_t *data_length);
static trudpMapElementData *_trudpMapGetValueData(void *tqd_data,
        uint32_t key_length);
static uint32_t _trudpMapHash(void *key, size_t key_length);
static trudpMapData *_trudpMapResize(trudpMapData *map, size_t size);
static teoQueueData *_trudpMapValueDataToQueueData(trudpMapElementData *mvd);

/**
 * Create new map
 * 
 * @param size Size of hash map (hash map resized automatically) 
 * @param auto_resize_f Auto resize hash map
 * @return Pointer to trudpMapData
 */
trudpMapData *trudpMapNew(size_t size, int auto_resize_f) {

    int i;
    trudpMapData *map = (trudpMapData *)malloc(sizeof(trudpMapData));

    // Fill parameters
    map->q = (teoQueue **)malloc(size * sizeof(teoQueue*));
    map->auto_resize_f = auto_resize_f;
    map->hash_map_size = size;
    map->collisions = 0;
    map->length = 0;

    // Create Hash table
    for(i = 0; i < size; i++) map->q[i] = teoQueueNew();

    return map;
}

/**
 * Resize hash table
 * 
 * @param map Pointer to trudpMapData
 * @param size New hash map size
 * @return Pointer to the same trudpMapData
 */
static trudpMapData *_trudpMapResize(trudpMapData *map, size_t size) {

    // Show mime of  resize for testing
    // #define _SHOW_FUNCTION_MSG_ 1
    #if _SHOW_FUNCTION_MSG_
    printf("resize map from %d to %d, time: ", (int)map->hash_map_size, (int)size);
    uint64_t t_stop, t_beg = trudpGetTimestampFull();
    #endif

    int i = 0;
    trudpMapData *map_new = trudpMapNew(size, map->auto_resize_f); 

    // Loop through existing map and add it elements to new map
    trudpMapIterator *it;
    if((it = trudpMapIteratorNew(map))) {
        while(trudpMapIteratorNext(it)) {
            trudpMapElementData *el = trudpMapIteratorElement(it);

            #define _SHOW_RESIZED_MSG_ 0
            #if _SHOW_RESIZED_MSG_
            printf("\n #%d hash: %010u, key: %s, value: %s ", 
                   i, el->hash, (char*)el->data, (char*)el->data + el->key_length);
            #endif


            #define _USE_PUT_ 1
            #if _USE_PUT_
            int idx = el->hash % map_new->hash_map_size;
            teoQueueData *qd_new, *qd = _trudpMapValueDataToQueueData(el);
            qd_new = teoQueueNewData(qd->data, qd->data_length);
            qd_new->prev = qd->prev;
            qd_new->next = qd->next;
            teoQueuePut(map_new->q[idx], qd_new);
            map_new->length++;
            #else
            size_t key_length;
            void *key = trudpMapIteratorElementKey(el, &key_length);
            size_t data_length;
            void *data = trudpMapIteratorElementData(el, &data_length);
            trudpMapAdd(map_new, key, key_length, data, data_length);
            #endif

            i++;
        }
        // Destroy map iterator
        trudpMapIteratorDestroy(it);
    }

    // Free existing queues and move queues pointer of new map to existing
    for(i = 0; i < map->hash_map_size; i++) {
        teoQueueFree(map->q[i]);
        free(map->q[i]);
    }
    free(map->q);
    map->q = map_new->q;
    map->hash_map_size = size;
    map->length = map_new->length;
    map->collisions = 0;
    free(map_new);

    #if _SHOW_FUNCTION_MSG_
    t_stop = (trudpGetTimestampFull() - t_beg);
    printf("%.3f ms\n", t_stop / 1000.0);
    #endif

    return map;
}

/**
 * Destroy map
 * 
 * @param map Pointer to trudpMapData
 */
void trudpMapDestroy(trudpMapData *map) {

    if(map) {

        int i;
        for(i = 0; i < map->hash_map_size; i++) {
            teoQueueDestroy(map->q[i]);
        }
        free(map->q);
        free(map);
    }
}

/**
 * Calculate hash for selected key
 *
 * @param key Pointer to key
 * @param key_length Key length
 * @return
 */
static inline uint32_t _trudpMapHash(void *key, size_t key_length) {

    // Select one of several hash functions
    #define _USE_HASH_ 0
    #if _USE_HASH_ == 0
    uint32_t hash = trudpHashFast(key, key_length, HASH_TABLE_INITVAL);
    #elif _USE_HASH_ == 1
    uint32_t hash = trudpHashSuperFast(key, key_length);
    #endif

    return hash;
}

/**
 * Get key data from hash table
 *
 * @param map Pointer to trudpHashTdata
 * @param key Key
 * @param key_length Key length
 * @param hash Hash of key
 * @param data_length [out] Pointer to returned data length (may be NULL)
 *
 * @return Pointer to Data of selected key or NULL if not found
 */
static void *_trudpMapGet(trudpMapData *map, void *key, size_t key_length,
        uint32_t hash, size_t *data_length) {

    void *data = NULL; //(void*)-1;
    if(data_length) *data_length = 0;

    int idx = hash % map->hash_map_size;
    trudpMapElementData *htd;
    teoQueueData *tqd;
    teoQueueIterator *it = teoQueueIteratorNew(map->q[idx]);
    if(it != NULL) {
      while((tqd = teoQueueIteratorNext(it))) {

        htd = (trudpMapElementData *)tqd->data;
        if(htd->hash == hash) {

            if(key_length == htd->key_length &&
               !memcmp(htd->data, key, key_length)) {

                if(data_length) *data_length = htd->data_length;
                data = htd->data + htd->key_length;
                break;
            }
            else map->collisions++;
        }
      }
      teoQueueIteratorFree(it);
    }

    return data;
}

/**
 * Get first available element from hash table
 * 
 * @param map Pointer to trudpHashTdata
 * @param data_length [out] Pointer to returned data length (may be NULL)
 * 
 * @return Pointer to Data of first available element or (void*)-1 if not found
 */
void *trudpMapGetFirst(trudpMapData *map, size_t *data_length) {
    
    void *data = (void*)-1;
    if(data_length) *data_length = 0;
    
    trudpMapIterator *it = trudpMapIteratorNew(map);
    if(it != NULL) {
        trudpMapElementData *el;
        if((el = trudpMapIteratorNext(it))) {
            data = trudpMapIteratorElementData(el, data_length);
        }
        trudpMapIteratorDestroy(it);
    }
    
    return data;
}

/**
 * Get pointer to trudpMapValueData from data pointer returned by trudpMapGet
 * 
 * @param tqd_data Pointer to map data returned by trudpMapGet function
 * @return Pointer to trudpMapValueData
 */
static inline trudpMapElementData *_trudpMapGetValueData(void *tqd_data,
        uint32_t key_length) {

    return tqd_data - key_length - sizeof(trudpMapElementData);
}

/**
 * Get pointer to maps queue data from pointer to trudpMapValueData
 * 
 * @param mvd Pointer to trudpMapValueData
 * @return Pointer to maps queue data
 */
static inline teoQueueData *_trudpMapValueDataToQueueData(trudpMapElementData *mvd) {
    return mvd ? (teoQueueData *)((void*)mvd - sizeof(teoQueueData)) : NULL;
}

/**
 * Add (or update) key data to the map
 *
 * @param map Pointer to trudpMapData
 * @param key Pointer to key
 * @param key_length Key length
 * @param data Pointer to data
 * @param data_length Data length
 * @return Data of added key or (void*)-1 at error
 */
void *trudpMapAdd(trudpMapData *map, void *key, size_t key_length, void *data,
        size_t data_length) {

    void *r_data = (void*)-1;

    if(!data) data_length = 0;

    // Create and fill Data structure
    size_t htd_length = sizeof(trudpMapElementData) + key_length + data_length;
    trudpMapElementData *htd = (trudpMapElementData *) malloc(htd_length);
    htd->hash = _trudpMapHash(key, key_length);
    htd->key_length = key_length;
    htd->data_length = data_length;
    memcpy(htd->data, key, key_length);
    if(data_length) memcpy(htd->data + htd->key_length, data, data_length);

    // Check that key exist and add data to map if not exists
    void *tqd_data = NULL;
    size_t d_length;
    teoQueueData *tqd;
    // Add data to map
    if(!(tqd_data = _trudpMapGet(map, key, key_length, htd->hash, &d_length))) {
        int idx = htd->hash % map->hash_map_size;
        tqd = teoQueueAdd(map->q[idx], (void*)htd, htd_length);
        if(tqd) {
            map->length++;

            // Resize if needed
            if(map->auto_resize_f && map->length > map->hash_map_size * 3)
                _trudpMapResize(map, map->hash_map_size * 10);
        }
    }
    // Update existing key data
    else {
        trudpMapElementData *htd_existing = _trudpMapGetValueData(tqd_data, key_length);
        tqd = _trudpMapValueDataToQueueData(htd_existing);
        int idx = htd->hash % map->hash_map_size;
        tqd = teoQueueUpdate(map->q[idx], (void*)htd, htd_length, tqd);
    }

    // Free allocated data
    free(htd);

    // Set pointers to trudpMapValueData and returned data
    if(tqd) {
        htd = (trudpMapElementData *)tqd->data;
        r_data = htd->data_length ? htd->data + htd->key_length : NULL;
    }

    return r_data;
}

/**
 * Get key data from hash table
 *
 * @param map Pointer to trudpMapData
 * @param key Pointer to key
 * @param key_length Key length
 * @param data_length [out] Pointer to data length
 *
 * @return Data of selected key (may be NULL) or (void*)-1 if not found
 */
void *trudpMapGet(trudpMapData *map, void *key, size_t key_length,
        size_t *data_length) {

    uint32_t hash = _trudpMapHash(key, key_length);
    void *data = _trudpMapGet(map, key, key_length, hash, data_length);

    return !data ? (void*)-1 : 
           _trudpMapGetValueData(data, key_length)->data_length ? data : NULL;
}

/**
 * Delete keys element from map
 *
 * @param map Pointer to trudpMapData
 * @param key Pointer to key
 * @param key_length Key length
 * @return Zero at success, or errors: -1 - keys element not found
 */
int trudpMapDelete(trudpMapData *map, void *key, size_t key_length) {

    int rv = -1;

    size_t data_length;
    uint32_t hash = _trudpMapHash(key, key_length);
    void *data = _trudpMapGet(map, key, key_length, hash, &data_length);
    if(data) {
        trudpMapElementData *mvd = _trudpMapGetValueData(data, key_length);
        teoQueueData *tqd = _trudpMapValueDataToQueueData(mvd);
        int idx = mvd->hash % map->hash_map_size;
        rv = teoQueueDelete(map->q[idx], tqd);
        if(!rv) {
            map->length--;

            // Resize if needed
            if(map->auto_resize_f && 
               map->hash_map_size > 10 && map->length < (map->hash_map_size / 10) * 3)
                _trudpMapResize(map, map->hash_map_size / 10);
        }
    }

    return rv;
}

/**
 * Create new map iterator
 * 
 * @param map Pointer to trudpMapData
 * @return Pointer to trudpMapIterator or NULL at memory allocate error
 */
trudpMapIterator *trudpMapIteratorNew(trudpMapData *map) {

    trudpMapIterator *map_it = (trudpMapIterator*)malloc(sizeof(trudpMapIterator));
    if(map_it) {
        map_it->it = teoQueueIteratorNew(map->q[0]);
        map_it->idx = 0;
        map_it->map = map;
        map_it->tmv = NULL;
    }

    return map_it;
}

/**
 * Destroy map iterator
 * 
 * @param map_it Pointer to trudpMapIterator
 * @return Zero at success
 */
int trudpMapIteratorDestroy(trudpMapIterator *map_it) {

    if(map_it) {
        teoQueueIteratorFree(map_it->it);
        free(map_it);
    }

    return 0;
}

/**
 * Get next maps element
 * 
 * @param map_it Pointer to trudpMapIterator
 * @return Pointer to map element data trudpMapValueData
 */
trudpMapElementData *trudpMapIteratorNext(trudpMapIterator *map_it) {

    if(!map_it) return NULL;

    teoQueueData *tqd;
    trudpMapElementData *tmv = NULL;

    while(!(tqd = teoQueueIteratorNext(map_it->it))) {
        if(++map_it->idx < map_it->map->hash_map_size) {
            teoQueueIteratorReset(map_it->it, map_it->map->q[map_it->idx]);
        }
        else break;
    }

    if(tqd) {
        tmv = (trudpMapElementData *)tqd->data;
        map_it->tmv = tmv;
    }

    return tmv;
}
