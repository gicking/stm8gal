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
#include <inttypes.h>
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


bool MemoryImage_isEmpty(const MemoryImage_s* image) {
    
    // check if memory image is empty
    if ((image->memoryEntries == NULL) || (image->numEntries == 0) || (image->capacity < image->numEntries))
        return true;

    // memory image contains data
    return false;

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
        fprintf(fp, "0x%04" PRIX64 "\t0x%02" PRIX8 "\n", (uint64_t) image->memoryEntries[i].address, (uint8_t) image->memoryEntries[i].data);
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


bool MemoryImage_addData(MemoryImage_s* image, const MEMIMAGE_ADDR_T address, const uint8_t data) {
    
    // if address already exists, replace content and return
    size_t idx;
    if (MemoryImage_getIndex(image, address, &idx)) {
        image->memoryEntries[idx].data = data;
        #if defined(MEMIMAGE_DEBUG)
            if (image->debug >= 1) {
                fprintf(stderr, "MemoryImage_addData(): 0x%04" PRIX64 " 0x%02" PRIX8 " -> overwrite %d\n", (uint64_t) address, (uint8_t) data, (int) idx);
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
            fprintf(stderr, "Error in MemoryImage_addData(): failed to reallocate %ldB\n", newCapacity * (long) sizeof(MemoryEntry_s));
            return false;
        }
        image->capacity = newCapacity;

    }

    // shift higher addresses by +1 to free space for new entry
    if (idx < image->numEntries) {
        memmove(&(image->memoryEntries[idx+1L]), &(image->memoryEntries[idx]), (image->numEntries - idx) * (size_t) (sizeof(MemoryEntry_s)));
    }
    
    // add new entry at correct location
    MemoryEntry_s* entry = &(image->memoryEntries[idx]);
    entry->address = address;
    entry->data = data;
    image->numEntries++;

    // optional debug output
    #if defined(MEMIMAGE_DEBUG)
        if (image->debug >= 1) {
            fprintf(stderr, "MemoryImage_addData(): 0x%04" PRIX64 " 0x%02" PRIX8 " -> insert %d\n", (uint64_t) address, (uint8_t) data, (int) idx);
        }
    #endif // MEMIMAGE_DEBUG

    // return success
    return true;

} // MemoryImage_addData()


bool MemoryImage_deleteData(MemoryImage_s* image, const MEMIMAGE_ADDR_T address) {

    // search for address in memory image
    size_t idx;
    if (MemoryImage_getIndex(image, address, &idx)) {

        // optional debug output
        #if defined(MEMIMAGE_DEBUG)
            if (image->debug >= 1) {
                fprintf(stderr, "MemoryImage_deleteData(): 0x%04" PRIX64 " -> delete %d\n", (uint64_t) address, (int) idx);
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
                fprintf(stderr, "Error in MemoryImage_deleteData(): failed to reallocate %ldB\n", newCapacity * (long) sizeof(MemoryEntry_s));
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
            fprintf(stderr, "MemoryImage_deleteData(): 0x%04" PRIX64 " -> unknown\n", (uint64_t) address);
        }
    #endif // MEMIMAGE_DEBUG

    // address not found -> error
    return false;

} // MemoryImage_deleteData()


bool MemoryImage_getData(const MemoryImage_s* image, const MEMIMAGE_ADDR_T address, uint8_t *data) {
    
    // search for address. If exists, return data
    size_t idx;
    if (MemoryImage_getIndex(image, address, &idx)) {
        *data = image->memoryEntries[idx].data;
        #if defined(MEMIMAGE_DEBUG)
            if (image->debug >= 1) {
                fprintf(stderr, "MemoryImage_getData(): 0x%04" PRIX64 " -> index %d, value 0x%02" PRIX8 "\n", (uint64_t) address, (int) idx, (uint8_t) *data);
            }
        #endif // MEMIMAGE_DEBUG
        return true;
    }

    // address not found -> error
    *data = 0x00;
    #if defined(MEMIMAGE_DEBUG)
        if (image->debug >= 1) {
            fprintf(stderr, "MemoryImage_getData(): 0x%04" PRIX64 " -> unknown\n", (uint64_t) address);
        }
    #endif // MEMIMAGE_DEBUG
    return false;

} // MemoryImage_getData()


bool MemoryImage_getIndex(const MemoryImage_s* image, const MEMIMAGE_ADDR_T address, size_t *index) {

    // handle empty image separately
    if (MemoryImage_isEmpty(image)) {
        *index = 0;
        #if defined(MEMIMAGE_DEBUG)
            if (image->debug >= 2) {
                fprintf(stderr, "MemoryImage_getIndex(): 0x%04" PRIX64 " -> empty: %d\n", (uint64_t) address, (int) *index);
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
                    fprintf(stderr, "MemoryImage_getIndex(): 0x%04" PRIX64 " -> found: %d\n", (uint64_t) address, (int) *index);
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
            fprintf(stderr, "MemoryImage_getIndex(): 0x%04" PRIX64 " -> unknown: %d\n", (uint64_t) address, (int) *index);
        }
    #endif // MEMIMAGE_DEBUG
    return false;

} // MemoryImage_getIndex()


bool MemoryImage_getMemoryBlock(const MemoryImage_s* image, const MEMIMAGE_ADDR_T addrStart, size_t *idxStart, size_t *idxEnd) {

    // handle empty image separately
    if (MemoryImage_isEmpty(image)) {
        #if defined(MEMIMAGE_DEBUG)
            if (image->debug >= 2) {
                fprintf(stderr, "MemoryImage_getMemoryBlock(): empty image\n");
            }
        #endif // MEMIMAGE_DEBUG
        *idxStart = 0x00;
        *idxEnd   = 0x00;
        return false;
    }

    // find start index of next memory block, abort on error (e.g. end of image reached)
    MemoryImage_getIndex(image, addrStart, idxStart);
    if (*idxStart == image->numEntries) {
        #if defined(MEMIMAGE_DEBUG)
            if (image->debug >= 2) {
                fprintf(stderr, "MemoryImage_getMemoryBlock(): end reached at address 0x%04" PRIX64 "\n", (uint64_t) addrStart);
            }
        #endif // MEMIMAGE_DEBUG
    }

    // find last index of next memory block
    MEMIMAGE_ADDR_T addrLast = image->memoryEntries[*idxStart].address;
    size_t idx =*idxStart+1;
    while ((idx < image->numEntries) && (image->memoryEntries[idx].address == addrLast+1))
    {
        idx++;
        addrLast++;
    }
    *idxEnd = idx-1; 

    // check if block found
    if (*idxEnd == image->numEntries) {
        #if defined(MEMIMAGE_DEBUG)
            if (image->debug >= 2) {
                fprintf(stderr, "MemoryImage_getMemoryBlock(): end reached at index %d\n", (int) *idxEnd);
            }
        #endif // MEMIMAGE_DEBUG
        return false;
    }

    // valid memory block was found
    return true;

} // MemoryImage_getMemoryBlock()


uint32_t MemoryImage_checksum_crc32(const MemoryImage_s* image, const size_t idxStart, const size_t idxEnd) {

    // initialize CRC32 checksum
    uint32_t crc = 0xFFFFFFFF;

    // loop over specified memory range 
    for (size_t i = idxStart; i <= idxEnd; i++) {

        // optionally update CRC32 with address
        #if defined(MEMIMAGE_CHK_INCLUDE_ADDRESS)
            
            // add address bytes in order depending on endianness
            for (int j = 0; j < sizeof(MEMIMAGE_ADDR_T); j++) {
                
                uint8_t     byte  = 0;
                uint16_t    val16 = 1;  // to check machine endianness

                // get next byte (little endian)
                if (*((uint8_t*) &val16) == 1)
                    byte = (image->memoryEntries[i].address >> (j * 8)) & 0xFF;
                
                // get next byte (big endian)
                else
                    byte = (image->memoryEntries[i].address >> ((sizeof(MEMIMAGE_ADDR_T) - 1 - j) * 8)) & 0xFF;
                
                // Update CRC32 with address byte
                crc ^= byte;
                for (int k = 0; k < 8; k++) {
                    if (crc & 1)
                        crc = (crc >> 1) ^ CRC32_IEEE_POLYNOM;
                    else
                        crc >>= 1;
                }

            } // loop over address bytes

        #endif // MEMIMAGE_CHK_INCLUDE_ADDRESS

        // update CRC32 with data. Only 1B -> no need to check endianness
        crc ^= image->memoryEntries[i].data;
        for (int j = 0; j < 8*sizeof(uint8_t); j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ CRC32_IEEE_POLYNOM;
            else
                crc >>= 1;
        }

    } // loop over memory range

    // finalize CRC32 checksum
    crc ^= 0xffffffff;

    // return checksum
    return(crc);

} // MemoryImage_checksum_crc32()


bool MemoryImage_fillValue(MemoryImage_s* image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrEnd, const uint8_t value) {

    bool result = true;

    #if defined(MEMIMAGE_DEBUG)
        if (image->debug >= 1) {
            fprintf(stderr, "MemoryImage_fillValue(): 0x%04" PRIX64 " 0x%04" PRIX64 " 0x%02" PRIX8 "\n", (uint64_t) addrStart, (uint64_t) addrEnd, (uint8_t) value);
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
            fprintf(stderr, "MemoryImage_fillRandom(): 0x%04" PRIX64 " 0x%04" PRIX64 "\n", (uint64_t) addrStart, (uint64_t) addrEnd);
        }
    #endif // MEMIMAGE_DEBUG

    // seed rand() only once
    if (flagOnce) {
        flagOnce = false;
        time_t seed = time(NULL);
        #if defined(MEMIMAGE_DEBUG)
            if (image->debug >= 2) {
                fprintf(stderr, "MemoryImage_fillRandom(): rand() seed %ld\n", (long) seed);
            }
        #endif // MEMIMAGE_DEBUG
        flagOnce = false;
        srand(seed);
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
            fprintf(stderr, "MemoryImage_clip(): 0x%04" PRIX64 " 0x%04" PRIX64 "\n", (uint64_t) addrStart, (uint64_t) addrEnd);
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
            fprintf(stderr, "MemoryImage_cut(): 0x%04" PRIX64 " 0x%04" PRIX64 "\n", (uint64_t) addrStart, (uint64_t) addrEnd);
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
        fprintf(stderr, "Error in cloneMemoryImage(): failed to allocate %ldB\n", (long) size);
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
            fprintf(stderr, "MemoryImage_copyRange(): 0x%04" PRIX64 " 0x%04" PRIX64 " 0x%04" PRIX64 "\n", (uint64_t) addrFromStart, (uint64_t) addrFromEnd, (uint64_t) addrToStart);
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
            fprintf(stderr, "MemoryImage_moveRange(): 0x%04" PRIX64 " 0x%04" PRIX64 " 0x%04" PRIX64 "\n", (uint64_t) addrFromStart, (uint64_t) addrFromEnd, (uint64_t) addrToStart);
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
