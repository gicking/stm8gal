/**
  \file memory_image.h

  \author G. Icking-Konert

  \brief declaration of memory image and functions to manipulate it

  declaration of memory image and functions to manipulate it
*/

// for including file only once
#ifndef _IMAGE_H_
#define _IMAGE_H_

/**********************
 INCLUDES
**********************/
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


/**********************
 GLOBAL DEFINES / MACROS
**********************/

/// uncomment for optional debug output
//#define MEMIMAGE_DEBUG

/// memory image address datatype / width
#define MEMIMAGE_ADDR_T         uint64_t

/// grow/shrink factor for memory image buffer. Must be >1.0!
#define MEMIMAGE_BUFFER_MARGIN  1.2

/// max. size for memory image buffer [B]
#define MEMIMAGE_BUFFER_MAX     50L*1024L*1024L


/**********************
 GLOBAL STRUCTS
**********************/

/// memory entry consisting of address and data 
typedef struct {
    MEMIMAGE_ADDR_T     address;        //< address 
    uint8_t             data;           //< data
} MemoryEntry_s;


/// memory image container  
typedef struct {
    MemoryEntry_s*      memoryEntries;  //< memory entries 
    size_t              numEntries;     //< number of used entries 
    size_t              capacity;       //< reserved capacity 
#if defined(MEMIMAGE_DEBUG)
    uint8_t             debug;          //< debug output level (0..2)
#endif
} MemoryImage_s;


/**********************
 GLOBAL FUNCTIONS
**********************/

/// @brief initialize empty memory image 
/// @param image          pointer to memory image
void MemoryImage_init(MemoryImage_s* image);

/// @brief release memory image buffer 
/// @param image          pointer to memory image
void MemoryImage_free(MemoryImage_s* image);

/// @brief print memory image to stdout
/// @param[in]  image     pointer to memory image
/// @param[in]  fp        stream to print to, e.g. stdout or file
void MemoryImage_print(const MemoryImage_s* image, FILE* fp);

#if defined(MEMIMAGE_DEBUG)
    /// @brief set debug output level 
    /// @param[in]  image     pointer to memory image
    /// @param[in]  debug     new debug level (0..2)
    void MemoryImage_setDebug(MemoryImage_s* image, const uint8_t debug);
#endif // MEMIMAGE_DEBUG

/// @brief search address in memory image
/// @param[in]  image   pointer to memory image
/// @param[in]  b       second entry to compare
/// @param[in]  address address to find
/// @param[out] index   index if address if found, else index of upper neighbour
/// @return address found
bool MemoryImage_findAddress(const MemoryImage_s* image, const MEMIMAGE_ADDR_T address, size_t *index);

/// @brief add byte at specified address in memory image. If address already exists, overwrite content
/// @param      image     pointer to memory image
/// @param[in]  address   address to add entry at
/// @param[in]  data      data to add
/// @return operation successful
bool MemoryImage_addData(MemoryImage_s* image, const MEMIMAGE_ADDR_T address, const uint8_t data);

/// @brief remove byte from specified address in memory image
/// @param      image     pointer to memory image
/// @param[in]  address   address to remove entry from
/// @return operation successful
bool MemoryImage_deleteData(MemoryImage_s* image, const MEMIMAGE_ADDR_T address);

/// @brief get byte from specified address in memory image
/// @param[in]  image     pointer to memory image
/// @param[in]  address   address read from
/// @param[out] data      read data
/// @return operation successful
bool MemoryImage_getData(const MemoryImage_s* image, const MEMIMAGE_ADDR_T address, uint8_t *data);

/// @brief fill address range [addrStart;addrEnd] with fixed value 
/// @param      image     pointer to memory image
/// @param[in]  addrStart start address (inclusive)
/// @param[in]  addrEnd   end address (inclusive)
/// @param[in]  value     value to fill with
/// @return operation successful
bool MemoryImage_fillValue(MemoryImage_s* image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrEnd, const uint8_t value);

/// @brief fill address range [addrStart;addrEnd] with random values 
/// @param      image     pointer to memory image
/// @param[in]  addrStart start address (inclusive)
/// @param[in]  addrEnd   end address (inclusive)
/// @return operation successful
bool MemoryImage_fillRandom(MemoryImage_s* image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrEnd);

/// @brief remove data outside address range [addrStart;addrEnd]
/// @param      image     pointer to memory image
/// @param[in]  addrStart start address (inclusive)
/// @param[in]  addrEnd   end address (inclusive)
/// @return operation successful
bool MemoryImage_clip(MemoryImage_s* image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrEnd);

/// @brief remove data inside address range [addrStart;addrEnd]
/// @param      image     pointer to memory image
/// @param[in]  addrStart start address (inclusive)
/// @param[in]  addrEnd   end address (inclusive)
/// @return operation successful
bool MemoryImage_cut(MemoryImage_s* image, const MEMIMAGE_ADDR_T addrStart, const MEMIMAGE_ADDR_T addrEnd);

/// @brief clone a memory image. If present, data in destImage will be erased
/// @param[in]  srcImage  source memory image
/// @param      destImage destination memory image. Must be initialized!
/// @return operation successful
bool MemoryImage_clone(const MemoryImage_s* srcImage, MemoryImage_s* destImage);

/// @brief merge two memory images into one. Data in destImage may be overwritten by srcImage
/// @param[in]  srcImage  source memory image
/// @param      destImage destination memory image
/// @return operation successful
bool MemoryImage_merge(const MemoryImage_s* srcImage, MemoryImage_s* destImage);

/// @brief copy address range [addrFromStart;addrFromEnd] to new addresses starting at addrToStart. Existing data is overwritten, empty data is ignored
/// @param      image     pointer to memory image
/// @param[in]  addrFromStart source start address (inclusive)
/// @param[in]  addrFromEnd   source end address (inclusive)
/// @param[in]  addrToStart   target start address
/// @return operation successful
bool MemoryImage_copyRange(MemoryImage_s* image, const MEMIMAGE_ADDR_T addrFromStart, const MEMIMAGE_ADDR_T addrFromEnd, const MEMIMAGE_ADDR_T addrToStart);

/// @brief move address range [addrFromStart;addrFromEnd] to new addresses starting at addrToStart. Existing data is overwritten, empty data is ignored
/// @param      image     pointer to memory image
/// @param[in]  addrFromStart source start address (inclusive)
/// @param[in]  addrFromEnd   source end address (inclusive)
/// @param[in]  addrToStart   target start address
/// @return operation successful
bool MemoryImage_moveRange(MemoryImage_s* image, const MEMIMAGE_ADDR_T addrFromStart, const MEMIMAGE_ADDR_T addrFromEnd, const MEMIMAGE_ADDR_T addrToStart);

#endif // _IMAGE_H_

// end of file
