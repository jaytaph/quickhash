#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "quickhash.h"

/**
 * Hashes the key
 *
 * The algorithm is from: http://burtleburtle.net/bob/hash/integer.html
 *
 * Parameters:
 * - key, the key to be hashsed
 *
 * Returns:
 * - the hash key
 */
static uint32_t jenkins(uint32_t key)
{
    key = (key ^ 61) ^ (key >> 16);
    key = key + (key << 3);
    key = key ^ (key >> 4);
    key = key * 0x27d4eb2d;
    key = key ^ (key >> 15);
    return key;
}

/**
 * Defines the number of buckets to pre-allocate
 */
#define QHB_BUFFER_PREALLOC_INC 1024

/**
 * Allocates a hash bucket.
 *
 * The algorithm allocates memory for QHB_BUFFER_PREALLOC_INC buckets at the
 * same time to avoid having too many allocs and frees.
 *
 * Returns:
 * - A newly allocated hash bucket or NULL upon allocation failure
 */
inline qhb *qhb_create(qhi *hash)
{
	qhb *tmp = NULL;

	if (hash->bucket_buffer_pos % QHB_BUFFER_PREALLOC_INC == 0) {
		hash->bucket_buffer_nr++;
		hash->bucket_buffer = realloc(hash->bucket_buffer, sizeof(qhb*) * (hash->bucket_buffer_nr + 1));
		hash->bucket_buffer[hash->bucket_buffer_nr] = malloc(sizeof(qhb) * QHB_BUFFER_PREALLOC_INC);
		hash->bucket_buffer_pos = 0;
	}
	tmp = &(hash->bucket_buffer[hash->bucket_buffer_nr][hash->bucket_buffer_pos]);
	hash->bucket_buffer_pos++;

	return tmp;
}

/**
 * Creates a new integer quick hash
 *
 * Parameters:
 * - size: the number of buckets to use. A typical value to pick here is to
 *         pick the expected amount of set elements.
 *
 * Returns:
 * - a pointer to the hash, or NULL if:
 *   - upon memory allocation failures.
 *   - size < 4
 */
qhi *qhi_create(uint32_t size)
{
	qhi *tmp;

	if (size < 4) {
		return NULL;
	}
	if (size > 1048576) {
		size = 1048576;
	}

	tmp = malloc(sizeof(qhi));
	if (!tmp) {
		return NULL;
	}

	tmp->hasher = jenkins;
	tmp->bucket_count = size;
	tmp->bucket_list = calloc(sizeof(qhl) * size, 1);

	tmp->bucket_buffer_nr   = -1;
	tmp->bucket_buffer_pos  = 0;
	tmp->bucket_buffer      = NULL;

	if (!tmp->bucket_list) {
		free(tmp);
		return NULL;
	}

	return tmp;
}

/**
 * Frees a quickhash
 *
 * Parameters:
 * - hash: A valid quickhash
 */
void qhi_free(qhi *hash)
{
	uint32_t idx;

	for (idx = 0; idx <= hash->bucket_buffer_nr; idx++) {
		free(hash->bucket_buffer[idx]);
	}

	free(hash->bucket_buffer);
	free(hash->bucket_list);
	free(hash);
}

/**
 * Obtains the hash value for the specified key
 *
 * Parameters:
 * - hash: A valid quickhash
 * - key: The key
 *
 * Returns:
 * - The hashed key
 */
inline uint32_t qhi_set_hash(qhi *hash, uint32_t key)
{
	uint32_t idx = hash->hasher(key);
	return idx % hash->bucket_count;
}

/**
 * Adds a new element to the hash
 *
 * Parameters:
 * - hash: A valid quickhash
 * - key: The key
 *
 * Returns:
 * - 1 if the element was added or 0 if the element couldn't be added
 */
int qhi_set_add(qhi *hash, int32_t key)
{
	uint32_t idx;
	qhl     *list;
	qhb     *bucket;

	// obtain the hashed key, and the bucket list for the hashed key
	idx = qhi_set_hash(hash, key);
	list = &(hash->bucket_list[idx]);

	// create new bucket
	bucket = qhb_create(hash);
	if (!bucket) {
		return 0;
	}
	bucket->key = key;
	bucket->next = NULL;

	// add bucket to list
	if (list->head == NULL) {
		// first bucket in list
		list->head = bucket;
		list->tail = bucket;
	} else {
		// following bucked in a list
		list->tail->next = bucket;
		list->tail = bucket;
	}
	return 1;
}

/**
 * Tests whether the key exists in the set
 *
 * Parameters:
 * - hash: A valid quickhash
 * - key: The key
 *
 * Returns:
 * - 1 if the element is part of the set or 0 if the element is not part of the
 *   set
 */
int qhi_set_exists(qhi *hash, int32_t key)
{
	uint32_t idx;
	qhl     *list;

	// obtain the hashed key, and the bucket list for the hashed key
	idx = qhi_set_hash(hash, key);
	list = &(hash->bucket_list[idx]);

	if (!list->head) {
		// there is no bucket list for this hashed key
		return 0;
	} else {
		qhb *p = list->head;

		// loop over the elements in this bucket list to see if the key exists
		do {
			if (p->key == key) {
				return 1;
			}
			p = p->next;
		} while(p);
	}
	return 0;
}

/**
 * Loads a set from a file pointed to by the file descriptor
 *
 * Parameters:
 * - fd: a file descriptor that is suitable for reading from
 *
 * Returns:
 * - A new hash, or NULL upon failure
 */
qhi *qhi_set_load_from_file(int fd)
{
	struct stat finfo;
	uint32_t    nr_of_elements, elements_read = 0;
	uint32_t    i, bytes_read;
	int32_t     key_buffer[1024];
	qhi        *tmp;

	if (fstat(fd, &finfo) != 0) {
		return NULL;
	}

	// if the filesize is not an increment of 4, abort
	if (finfo.st_size % 4 != 0) {
		return NULL;
	}

	// create the set
	nr_of_elements = finfo.st_size / 4;
	tmp = qhi_create(nr_of_elements);
	if (!tmp) {
		return NULL;
	}

	// read the elements and add them to the set
	do {
		bytes_read = read(fd, &key_buffer, sizeof(key_buffer));
		for (i = 0; i < bytes_read / 4; i++) {
			qhi_set_add(tmp, key_buffer[i]);
		}
		elements_read += (bytes_read / 4);
	} while (elements_read < nr_of_elements);
	return tmp;
}
