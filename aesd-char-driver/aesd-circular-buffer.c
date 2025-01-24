/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
    struct aesd_buffer_entry *buffer_entry = &buffer->entry[buffer->out_offs];
    int next;
    int increment = 0;
    size_t total_offset = 0;

    for(int i = 0; total_offset <= char_offset && i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;i++){
	buffer_entry = &buffer->entry[next];
	next = buffer->out_offs + increment;
	if(next >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
	    increment = 0;
	    next = 0;
	}
        buffer_entry = &buffer->entry[next];
	total_offset = total_offset + buffer_entry->size;
	increment++;
    }
    
    size_t temp = total_offset - char_offset;

    if(char_offset >= total_offset){
	return NULL;
    }

    if((temp - buffer_entry->size) != 0){
	*entry_offset_byte_rtn = buffer_entry->size - temp;
    }

    return buffer_entry;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
    int next;

    next = buffer->in_offs + 1;
    if(next >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED){
	buffer->full = true;
        next = 0;
    }

    if(buffer->in_offs == buffer->out_offs && buffer->full == true){
        buffer->out_offs = buffer->out_offs + 1;
    }

    buffer->entry[buffer->in_offs] = *add_entry;
    buffer->in_offs = next;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
