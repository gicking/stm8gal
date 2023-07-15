/**
  \file memory_image.c

  \author G. Icking-Konert

  \brief implementation of memory image and functions to manipulate it

  implementation of memory image and functions to manipulate it
*/

/**********************
 INCLUDES
**********************/
#include <time.h>
#include <string.h>
#include <math.h>
#include "memory_image.h"


/**********************
 LOCAL MACROS
**********************/
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))


/**********************
 GLOBAL FUNCTIONS
**********************/

void MemoryImage_init(MemoryImage_s* image) {
    
    // initialize struct variables
    image->memoryEntries = NULL;
    image->numEntries = 0;
    image->capacity = 0;
    #if defined(MEMIMAGE_DEBUG)
        image->debug = 0;
    #endif

} // MemoryImage_init()


void MemoryImage_free(MemoryImage_s* image) {
    
    // release memory buffer 
    free(image->memoryEntries);

    // reset struct variables
    image->memoryEntries = NULL;
    image->numEntries = 0;
    image->capacity = 0;
    #if defined(MEMIMAGE_DEBUG)
        image->debug = 0;
    #endif

} // MemoryImage_free()


void MemoryImage_print(const MemoryImage_s* image, FILE* fp) {
    
    // optional debug output
    #if defined(MEMIMAGE_DEBUG)
        if (image->debug >= 1) {
            fprintf(fp, "MemoryImage_print(): numEntries=%ld, capacity=%ld\n", (long) image->numEntries, (long) image->capacity);
            fprintf(fp, "\n");
            fprintf(fp, "address\tdata\n");
        }
    #endif // MEMIMAGE_DEBUG

    // loop over image and output address, data in hex format
    for (size_t i = 0; i < image->numEntries; i++) {
        fprintf(fp, "0x%04X\t0x%02X\n", (int) image->memoryEntries[i].address, (int) image->memoryEntries[i].data);
    }
    fflush(fp);

} // MemoryImage_print()


#if defined(MEMIMAGE_DEBUG)
    void MemoryImage_setDebug(MemoryImage_s* image, const uint8_t debug) {
        
        // optional debug output
        if (image->debug == 2) {
            fprintf(stderr, "MemoryImage_setDebug(): debug=%d\n", (int) debug);
        }

        // just set new debug level
        image->debug = debug;

    } // MemoryImage_setDebug()
#endif // MEMIMAGE_DEBUG


bool MemoryImage_findAddress(const MemoryImage_s* image, const MEMIMAGE_ADDR_T address, size_t *index) {

    // handle empty image separately
    if (image->numEntries == 0) {
        *index = 0;
        #if defined(MEMIMAGE_DEBUG)
            if (image->debug >= 2) {
                fprintf(stderr, "MemoryImage_findAddress(): 0x%04x -> empty: %d\n", (int) address, (int) *index);
            }
        #endif // MEMIMAGE_DEBUG
        return false;
    }
    
    // search for address using binary search. If exists, return index
    int64_t low = 0;
    int64_t high = image->numEntries - 1;
    int64_t mid;
    while (low <= high) {
        mid = low + (high - low) / 2;
        if (image->memoryEntries[mid].address == address) {
            *index = mid;
            #if defined(MEMIMAGE_DEBUG)
                if (image->debug >= 2) {
                    fprintf(stderr, "MemoryImage_findAddress(): 0x%04x -> found: %d\n", (int) address, (int) *index);
                }
            #endif // MEMIMAGE_DEBUG
            return true;
        }
        if (image->memoryEntries[mid].address < address) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    
    // address not found -> return index of upper neighbour
    *index = low;
    #if defined(MEMIMAGE_DEBUG)
        if (image->debug >= 2) {
            fprintf(stderr, "MemoryImage_findAddress(): 0x%04x -> unknown: %d\n", (int) address, (int) *index);
        }
    #endif // MEMIMAGE_DEBUG
    return false;

} // MemoryImage_findAddress()


bool MemoryImage_addData(MemoryImage_s* image, const MEMIMAGE_ADDR_T address, const uint8_t data) {
    
    // if address already exists, replace content and return
    size_t idx;
    if (MemoryImage_findAddress(image, address, &idx)) {
        image->memoryEntries[idx].data = data;
        #if defined(MEMIMAGE_DEBUG)
            if (image->debug >= 1) {
                fprintf(stderr, "MemoryImage_addData(): 0x%04x 0x%02X -> overwrite %d\n", (int) address, (int) data, (int) idx);
            }
        #endif // MEMIMAGE_DEBUG
        return true;
    }

    // assert buffer size limit
    if ((image->numEntries+1) * sizeof(MemoryEntry_s) > MEMIMAGE_BUFFER_MAX) {
        fprintf(stderr, "Error in MemoryImage_addData(): buffer size limit of %gMB reached\n", (float) MEMIMAGE_BUFFER_MAX/(1024.0*1024.0));
        return false;
    }

    // expand memory buffer, if required
    if (image->numEntries+1 >= image->capacity) {
        size_t newCapacity = MAX(image->numEntries+1, MIN(ceil((float) image->capacity * (float) MEMIMAGE_BUFFER_MARGIN), MEMIMAGE_BUFFER_MAX));

        // optional debug output
        #if defined(MEMIMAGE_DEBUG)
            if (image->debug >= 2) {
                fprintf(stderr, "MemoryImage_addData(): resize %d to %d\n", (int) image->capacity, (int) newCapacity);
            }
        #endif // MEMIMAGE_DEBUG

        // re-allocate memory buffer. Return on fail
        image->memoryEntries = (MemoryEntry_s*)realloc(image->memoryEntries, newCapacity * sizeof(MemoryEntry_s));
        if (image->memoryEntries == NULL) {
            fprintf(stderr, "Error in MemoryImage_addData(): failed to reallocate %ldB\n", newCapacity * (long int) sizeof(MemoryEntry_s));
            return false;
        }
        image->capacity = newCapacity;

    }

    // shift higher addresses by +1 to free space for new entry
    if (idx < image->numEntries) {
        memcpy(&(image->memoryEntries[idx+1L]), &(image->memoryEntries[idx]), (image->numEntries - idx) * (size_t) (sizeof(MemoryEntry_s)));
    }
    
    // add new entry at correct location
    MemoryEntry_s* entry = &(image->memoryEntries[idx]);
    entry->address = address;
    entry->data = data;
    image->numEntries++;

    // optional debug output
    #if defined(MEMIMAGE_DEBUG)
        if (image->debug >= 1) {
            fprintf(stderr, "MemoryImage_addData(): 0x%04x 0x%02X -> insert %d\n", (int) address, (int) data, (int) idx);
        }
    #endif // MEMIMAGE_DEBUG

    // return success
    return true;

} // MemoryImage_addData()


bool MemoryImage_deleteData(MemoryImage_s* image, const MEMIMAGE_ADDR_T address) {

    // search for address in memory image
    size_t idx;
    if (MemoryImage_findAddress(image, address, &idx)) {

        // optional debug output
        #if defined(MEMIMAGE_DEBUG)
            if (image->debug >= 1) {
                fprintf(stderr, "MemoryImage_deleteData(): 0x%04x -> delete %d\n", (int) address, (int) idx);
            }
        #endif // MEMIMAGE_DEBUG

        // shift all following elements left to remove original entry
        for (size_t j = idx; j < image->numEntries - 1; j++) {
            image->memoryEntries[j] = image->memoryEntries[j + 1];
        }
        image->numEntries--;

        // shrink memory buffer if possible
        if ((image->capacity > 1) && (floor(image->numEntries * (float) MEMIMAGE_BUFFER_MARGIN) <= (float) image->capacity)) {
            size_t newCapacity = MAX(1, image->numEntries);

            // optional debug output
            #if defined(MEMIMAGE_DEBUG)
                if (image->debug >= 2) {
                    fprintf(stderr, "MemoryImage_deleteData(): resize %d to %d\n", (int) image->capacity, (int) newCapacity);
                }
            #endif // MEMIMAGE_DEBUG

            // re-allocate memory buffer. Return on fail
            image->memoryEntries = (MemoryEntry_s*)realloc(image->memoryEntries, newCapacity * sizeof(MemoryEntry_s));
            if (image->memoryEntries == NULL) {
                fprintf(stderr, "Error in MemoryImage_deleteData(): failed to reallocate %ldB\n", newCapacity * (long int) sizeof(MemoryEntry_s));
                return false;
            }
            image->capacity = newCapacity;

        }

        // deletion was successful
        return true;
    }

    // optional debug output
    #if defined(MEMIMAGE_DEBUG)
        if (image->debug >= 1) {
            fprintf(stderr, "MemoryImage_deleteData(): 0x%04x -> unknown\n", (int) address);
        }
    #endif // MEMIMAGE_DEBUG

    // address not found -> error
    return false;

} // MemoryImage_deleteData()


bool MemoryImage_getData(const MemoryImage_s* image, const MEMIMAGE_ADDR_T address, uint8_t *data) {
    
    // search for address. If exists, return data
    size_t idx;
    if (MemoryImage_findAddress(image, address, &idx)) {
        *data = image->memoryEntries[idx].data;
        #if defined(MEMIMAGE_DEBUG)
            if (image->debug >= 1) {
                fprintf(stderr, "MemoryImage_getData(): 0x%04x -> index %d, value 0x%02X\n", (int) address, (int) idx, (int) *data);
            }
        #endif // MEMIMAGE_DEBUG
        return true;
    }

    // address not found -> error
    *data = 0x00;
    #if defined(MEMIMAGE_DEBUG)
        if (image->debug >= 1) {
            fprintf(stderr, "MemoryImage_getData(): 0x%04x -> unknown\n", (int) address);
        }
    #endif // MEMIMAGE_DEBUG
    return false;

} // MemoryImage_getData()


bool MemoryImage_fillValue(MemoryImage_s* image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrEnd, const uint8_t value) {

    bool result = true;

    #if defined(MEMIMAGE_DEBUG)
        if (image->debug >= 1) {
            fprintf(stderr, "MemoryImage_fillValue(): 0x%04x 0x%04x 0x%02X\n", (int) addrStart, (int) addrEnd, (int) value);
        }
    #endif // MEMIMAGE_DEBUG

    // loop over address range and add/replace fixed data
    for (MEMIMAGE_ADDR_T address = addrStart; address <= addrEnd; address++) {
        result &= MemoryImage_addData(image, address, value);
    }

    // return cumulated result
    return result;

} // MemoryImage_fillValue()


bool MemoryImage_fillRandom(MemoryImage_s* image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrEnd) {

    bool result = true;
    static bool flagOnce = true;
    
    // optional debug output
    #if defined(MEMIMAGE_DEBUG)
        if (image->debug >= 1) {
            fprintf(stderr, "MemoryImage_fillRandom(): 0x%04x 0x%04x\n", (int) addrStart, (int) addrEnd);
        }
    #endif // MEMIMAGE_DEBUG

    // seed rand() only once
    if (flagOnce) {
        #if defined(MEMIMAGE_DEBUG)
            if (image->debug >= 2) {
                fprintf(stderr, "MemoryImage_fillRandom(): seed rand()\n");
            }
        #endif // MEMIMAGE_DEBUG
        flagOnce = false;
        srand(time(NULL));
    }

    // loop over address range and add/replace random data
    for (MEMIMAGE_ADDR_T address = addrStart; address <= addrEnd; address++) {
        result &= MemoryImage_addData(image, address, rand() % 0xFF);
    }

    // return cumulated result
    return result;

} // MemoryImage_fillRandom()


bool MemoryImage_clip(MemoryImage_s* image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrEnd) {

    bool result = true;

    // optional debug output
    #if defined(MEMIMAGE_DEBUG)
        if (image->debug >= 1) {
            fprintf(stderr, "MemoryImage_clip(): 0x%04x 0x%04x\n", (int) addrStart, (int) addrEnd);
        }
    #endif // MEMIMAGE_DEBUG

    // loop over image and remove data outside [addrStart;addrEnd]
    size_t i = 0;
    while (i != image->numEntries) {
        MEMIMAGE_ADDR_T address = image->memoryEntries[i].address;
        if ((address < addrStart) || (address > addrEnd)) {
            result &= MemoryImage_deleteData(image, address);
        } else {
            i++;
        }
    }

    // return cumulated result
    return result;

} // MemoryImage_clip()


bool MemoryImage_cut(MemoryImage_s* image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrEnd) {

    bool result = true;

    // optional debug output
    #if defined(MEMIMAGE_DEBUG)
        if (image->debug >= 1) {
            fprintf(stderr, "MemoryImage_cut(): 0x%04x 0x%04x\n", (int) addrStart, (int) addrEnd);
        }
    #endif // MEMIMAGE_DEBUG

    // loop over image and remove data inside [addrStart;addrEnd]
    size_t i = 0;
    while (i != image->numEntries) {
        MEMIMAGE_ADDR_T address = image->memoryEntries[i].address;
        if ((address >= addrStart) && (address <= addrEnd)) {
            result &= MemoryImage_deleteData(image, address);
        } else {
            i++;
        }
    }

    // return cumulated result
    return result;

} // MemoryImage_cut()


bool MemoryImage_clone(const MemoryImage_s* srcImage, MemoryImage_s* destImage) {

    // optional debug output
    #if defined(MEMIMAGE_DEBUG)
        if ((srcImage->debug >= 1) || (destImage->debug >= 1)) {
            fprintf(stderr, "MemoryImage_clone()\n");
        }
    #endif // MEMIMAGE_DEBUG

    // assert empty destination
    if (destImage->memoryEntries != NULL) {
        MemoryImage_free(destImage);
    } else {
        MemoryImage_init(destImage);
    }
    
    // allocate memory and copy srcImage data to destImage. Copy only used buffer
    size_t size = srcImage->numEntries * sizeof(MemoryEntry_s);
    destImage->memoryEntries = (MemoryEntry_s*) malloc(size);
    if (destImage->memoryEntries == NULL) {
        fprintf(stderr, "Error in cloneMemoryImage(): failed to allocate %ldB\n", (long int) size);
        return false;
    }
    memcpy((void*) destImage->memoryEntries, (void*) srcImage->memoryEntries, size);
    destImage->numEntries = srcImage->numEntries;
    destImage->capacity = srcImage->numEntries;
    #if defined(MEMIMAGE_DEBUG)
        destImage->debug = srcImage->debug;
    #endif // MEMIMAGE_DEBUG

    // return success
    return true;

} // MemoryImage_clone()


bool MemoryImage_merge(const MemoryImage_s* srcImage, MemoryImage_s* destImage) {

    bool result = true;

    // optional debug output
    #if defined(MEMIMAGE_DEBUG)
        if ((srcImage->debug >= 1) || (destImage->debug >= 1)) {
            fprintf(stderr, "MemoryImage_merge()\n");
        }
    #endif // MEMIMAGE_DEBUG

    // loop over srcImage and add/replace data to/in destImage
    for (size_t i=0; i < srcImage->numEntries; i++) {
        result &= MemoryImage_addData(destImage, srcImage->memoryEntries[i].address, srcImage->memoryEntries[i].data);
    }

    // return cumulated result
    return result;

} // MemoryImage_merge()


bool MemoryImage_copyRange(MemoryImage_s* image, const MEMIMAGE_ADDR_T addrFromStart, const MEMIMAGE_ADDR_T addrFromEnd, const MEMIMAGE_ADDR_T addrToStart) {

    bool result = true;

    // optional debug output
    #if defined(MEMIMAGE_DEBUG)
        if (image->debug >= 1) {
            fprintf(stderr, "MemoryImage_copyRange(): 0x%04X 0x%04X 0x%04X\n", (int) addrFromStart, (int) addrFromEnd, (int) addrToStart);
        }
    #endif // MEMIMAGE_DEBUG

    // copy image to temporary working image
    MemoryImage_s tmpImage;
    MemoryImage_init(&tmpImage);
    result &= MemoryImage_clone(image, &tmpImage);

    // loop over image and copy specified range to temporary image
    for (size_t i=0; i < image->numEntries; i++) {
        MEMIMAGE_ADDR_T address = image->memoryEntries[i].address;
        if ((address >= addrFromStart) && (address <= addrFromEnd)) {
            result &= MemoryImage_addData(&tmpImage, address-addrFromStart+addrToStart, image->memoryEntries[i].data);
        }
    }

    // copy result to image. Release original buffer
    free(image->memoryEntries);
    image->memoryEntries = tmpImage.memoryEntries;
    image->numEntries = tmpImage.numEntries;
    image->capacity = tmpImage.capacity;

    // return cumulated result
    return result;

} // MemoryImage_copyRange()


bool MemoryImage_moveRange(MemoryImage_s* image, const MEMIMAGE_ADDR_T addrFromStart, const MEMIMAGE_ADDR_T addrFromEnd, const MEMIMAGE_ADDR_T addrToStart) {

    bool result = true;

    // optional debug output
    #if defined(MEMIMAGE_DEBUG)
        if (image->debug >= 1) {
            fprintf(stderr, "MemoryImage_moveRange(): 0x%04X 0x%04X 0x%04X\n", (int) addrFromStart, (int) addrFromEnd, (int) addrToStart);
        }
    #endif // MEMIMAGE_DEBUG

    // copy image to temporary working image
    MemoryImage_s tmpImage;
    MemoryImage_init(&tmpImage);
    result &= MemoryImage_clone(image, &tmpImage);

    // loop over image and remove specified range from temporary image
    for (size_t i=0; i < image->numEntries; i++) {
        MEMIMAGE_ADDR_T address = image->memoryEntries[i].address;
        if ((address >= addrFromStart) && (address <= addrFromEnd)) {
            result &= MemoryImage_deleteData(&tmpImage, address);
        }
    }
    
    // loop over image and add specified range to temporary image
    for (size_t i=0; i < image->numEntries; i++) {
        MEMIMAGE_ADDR_T address = image->memoryEntries[i].address;
        if ((address >= addrFromStart) && (address <= addrFromEnd)) {
            result &= MemoryImage_addData(&tmpImage, address-addrFromStart+addrToStart, image->memoryEntries[i].data);
        }
    }

    // copy result to image. Release original buffer
    free(image->memoryEntries);
    image->memoryEntries = tmpImage.memoryEntries;
    image->numEntries = tmpImage.numEntries;
    image->capacity = tmpImage.capacity;

    // return cumulated result
    return result;

} // MemoryImage_moveRange()

// end of file
