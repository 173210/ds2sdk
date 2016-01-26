/*
 directory.c
 Reading, writing and manipulation of the directory structure on
 a FAT partition

 Copyright (c) 2006 Michael "Chishm" Chisholm
	
 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
	2006-08-14 - Chishm
		* entryFromPath correctly finds "" and "." now
		
	2006-08-17 - Chishm
		* entryFromPath doesn't look for "" anymore - use "." to refer to the current directory
	
	2006-08-19 - Chishm 
		* Fixed entryFromPath bug when looking for "." in root directory
		
	2006-10-01 - Chishm
		* Now clears the whole new cluster when linking in more clusters for a directory
		
	2006-10-28 - Chishm
		* stat returns the hostType for the st_dev value
*/
//version 1.1
//Note: fix bug in _FAT_directory_isValidAlias()

#include <string.h>
#include <ctype.h>

#include "directory.h"
#include "fs_common.h"
#include "partition.h"
#include "file_allocation_table.h"
#include "bit_ops.h"
#include "filetime.h"
#include "fs_unicode.h"

// Directory entry codes
#define DIR_ENTRY_LAST 0x00
#define DIR_ENTRY_FREE 0xE5


// Long file name directory entry
enum LFN_offset {
	LFN_offset_ordinal = 0x00,	// Position within LFN
	LFN_offset_char0 = 0x01,	
	LFN_offset_char1 = 0x03,
	LFN_offset_char2 = 0x05,
	LFN_offset_char3 = 0x07,
	LFN_offset_char4 = 0x09,
	LFN_offset_flag = 0x0B,	// Should be equal to ATTRIB_LFN
	LFN_offset_reserved1 = 0x0C,	// Always 0x00
	LFN_offset_checkSum = 0x0D,	// Checksum of short file name (alias)
	LFN_offset_char5 = 0x0E,
	LFN_offset_char6 = 0x10,
	LFN_offset_char7 = 0x12,
	LFN_offset_char8 = 0x14,
	LFN_offset_char9 = 0x16,
	LFN_offset_char10 = 0x18,
	LFN_offset_reserved2 = 0x1A,	// Always 0x0000
	LFN_offset_char11 = 0x1C,
	LFN_offset_char12 = 0x1E
};
const int LFN_offset_table[13]={0x01,0x03,0x05,0x07,0x09,0x0E,0x10,0x12,0x14,0x16,0x18,0x1C,0x1E}; 

#define LFN_END 0x40
#define LFN_DEL 0x80

char* strupr(char* a)
{
	char *str;

	str = a;
	while(*str)
	{
		if(*str >= 'a' && *str <= 'z') *str -= 0x20;
		str += 1;
	}

	return a;
}

bool _FAT_directory_isValidLfn (const char* name) {
	u32 i;
	u32 nameLength;
	// Make sure the name is short enough to be valid
	if ( strnlen(name, MAX_FILENAME_LENGTH) >= MAX_FILENAME_LENGTH) {
		return false;
	}
	// Make sure it doesn't contain any invalid characters
	if (strpbrk (name, "\\/:*?\"<>|") != NULL) {
		return false;
	}

	nameLength = strnlen(name, MAX_FILENAME_LENGTH);
	// Make sure the name doesn't contain any control codes
	for (i = 0; i < nameLength; i++) {
		if ((unsigned char)name[i] < 0x20) {
			return false;
		}
	}
	// Otherwise it is valid
	return true;
}
bool _FAT_directory_isValidAlias (const char* name) {
	u32 i;
	u32 nameLength;
	const char* dot;

	// Make sure the name is short enough to be valid
	if ( strnlen(name, MAX_ALIAS_LENGTH) >= MAX_ALIAS_LENGTH) {
		return false;
	}
	// Make sure it doesn't contain any invalid characters
	if (strpbrk (name, "\\/:;*?\"<>|&+,=[]") != NULL) {
		return false;
	}

	//
	if (strpbrk (name, " ") != NULL) {
		return false;
	}

	nameLength = strnlen(name, MAX_ALIAS_LENGTH);
	// Make sure the name doesn't contain any control codes
	for (i = 0; i < nameLength; i++) {
		if (name[i] < 0x20) {
			return false;
		}
	}

	dot = strchr ( name, '.');
	// Make sure there is only one '.'
	if ((dot != NULL) && (strrchr ( name, '.') != dot)) {
		return false;
	}
	// If there is a '.':
	if (dot != NULL) {
		// Make sure the filename portion is 1-8 characters long
		if (((dot - 1 - name) > 8) || ((dot - 1 - name) < 1)) {
			return false;
		}
		// Make sure the extension is 1-3 characters long, if it exists
		if ((strnlen(dot + 1, MAX_ALIAS_LENGTH) > 3) || (strnlen(dot + 1, MAX_ALIAS_LENGTH) < 1)) {
			return false;
		}
	} else {
		// Make sure the entire file name is 1-8 characters long
		if ((nameLength > 8) || (nameLength < 1)) {
			return false;
		}
	}

	// Since we made it through all those tests, it must be valid
	return true;
}

static bool _FAT_directory_entryGetAlias (const u8* entryData, char* destName) {
	int i=0;
	int j=0;

	destName[0] = '\0';
	if (entryData[0] != DIR_ENTRY_FREE) {
		if (entryData[0] == '.') {
			destName[0] = '.';
			if (entryData[1] == '.') {
				destName[1] = '.';
				destName[2] = '\0';
			} else {
				destName[1] = '\0';
			}
		} else {		
			// Copy the filename from the dirEntry to the string
			for (i = 0; (i < 8) && (entryData[DIR_ENTRY_name + i] != ' '); i++) {
				destName[i] = entryData[DIR_ENTRY_name + i];
			}
			// Copy the extension from the dirEntry to the string
			if (entryData[DIR_ENTRY_extension] != ' ') {
				destName[i++] = '.';
				for ( j = 0; (j < 3) && (entryData[DIR_ENTRY_extension + j] != ' '); j++) {
					destName[i++] = entryData[DIR_ENTRY_extension + j];
				}
			}
			destName[i] = '\0';
		}
	}

	return (destName[0] != '\0');
}

u32 _FAT_directory_entryGetCluster (const u8* entryData) {
	return u8array_to_u16(entryData,DIR_ENTRY_cluster) | (u8array_to_u16(entryData, DIR_ENTRY_clusterHigh) << 16);
}

static bool _FAT_directory_incrementDirEntryPosition (PARTITION* partition, DIR_ENTRY_POSITION* entryPosition, bool extendDirectory) {
	DIR_ENTRY_POSITION position;
	position = *entryPosition;
	u32 tempCluster;

	// Increment offset, wrapping at the end of a sector
	++ position.offset;
	if (position.offset == BYTES_PER_READ / DIR_ENTRY_DATA_SIZE) {
		position.offset = 0;
		// Increment sector when wrapping
		++ position.sector;
		// But wrap at the end of a cluster
		if ((position.sector == partition->sectorsPerCluster) && (position.cluster != FAT16_ROOT_DIR_CLUSTER)) {
			position.sector = 0;
			// Move onto the next cluster, making sure there is another cluster to go to
			tempCluster = _FAT_fat_nextCluster(partition, position.cluster);
			if (tempCluster == CLUSTER_EOF) {
				if (extendDirectory) {
					tempCluster = _FAT_fat_linkFreeClusterCleared (partition, position.cluster);
					if (tempCluster == CLUSTER_FREE) {
						return false;	// This will only happen if the disc is full
					}
				} else {
					return false;		// Got to the end of the directory, not extending it
				}
			} 
			position.cluster = tempCluster;
		} else if ((position.cluster == FAT16_ROOT_DIR_CLUSTER) && (position.sector == (partition->dataStart - partition->rootDirStart))) {
			return false;	// Got to end of root directory, can't extend it
		}
	}
	*entryPosition = position;
	return true;
}

bool _FAT_directory_getNextEntry (PARTITION* partition, DIR_ENTRY* entry) {
	DIR_ENTRY_POSITION entryStart;
	DIR_ENTRY_POSITION entryEnd;

	u8 entryData[0x20];

	bool notFound, found;
	u32 maxSectors;
	int lfnPos;
	u8 lfnChkSum, chkSum;
	char* filename;
	u16	unicodeFilename[256];
	bool lfnExists;

	int i;

	lfnChkSum = 0;

	entryStart = entry->dataEnd;

	// Make sure we are using the correct root directory, in case of FAT32
	if (entryStart.cluster == FAT16_ROOT_DIR_CLUSTER) {
		entryStart.cluster = partition->rootDirCluster;
	}

	entryEnd = entryStart;
	filename = entry->d_name;
	//unicodeFilename = entry->unicodeFilename;
	memset( unicodeFilename, 0, 512 );

	// Can only be FAT16_ROOT_DIR_CLUSTER if it is the root directory on a FAT12 or FAT16 partition
	if (entryStart.cluster == FAT16_ROOT_DIR_CLUSTER) {
		maxSectors = partition->dataStart - partition->rootDirStart;
	} else {
		maxSectors = partition->sectorsPerCluster;
	}

	lfnExists = false;

	found = false;
	notFound = false;

	while (!found && !notFound) {
		if (_FAT_directory_incrementDirEntryPosition (partition, &entryEnd, false) == false) {
			notFound = true;
		}

		_FAT_cache_readPartialSector (partition->cache, entryData, _FAT_fat_clusterToSector(partition, entryEnd.cluster) + entryEnd.sector, entryEnd.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE);

		if (entryData[DIR_ENTRY_attributes] == ATTRIB_LFN) {
			// It's an LFN
			if (entryData[LFN_offset_ordinal] & LFN_DEL) {
				lfnExists = false;
			} else if (entryData[LFN_offset_ordinal] & LFN_END) {
				// Last part of LFN, make sure it isn't deleted using previous if(Thanks MoonLight)
				entryStart = entryEnd;	// This is the start of a directory entry
				lfnExists = true;
				//filename[(entryData[LFN_offset_ordinal] & ~LFN_END) * 13] = '\0';	// Set end of lfn to null character
				unicodeFilename[(entryData[LFN_offset_ordinal] & ~LFN_END) * 13] = 0x0000;
				lfnChkSum = entryData[LFN_offset_checkSum];
			} if (lfnChkSum != entryData[LFN_offset_checkSum]) {
				lfnExists = false;
			}
			//unicodeFilename[0] = 0x0000;
			if (lfnExists) {
				lfnPos = ((entryData[LFN_offset_ordinal] & ~LFN_END) - 1) * 13;
				for (i = 0; i < 13; i++) {
					unicodeFilename[lfnPos + i] = u8array_to_u16( entryData, LFN_offset_table[i]);
					//	| entryData[LFN_offset_table[i]+1]<<8; // modify this for unicode support;
				}
			}
		} else if (entryData[DIR_ENTRY_attributes] & ATTRIB_VOL) {
			// This is a volume name, don't bother with it
		} else if (entryData[0] == DIR_ENTRY_LAST) {
			notFound = true;
		} else if ((entryData[0] != DIR_ENTRY_FREE) && (entryData[0] > 0x20) && !(entryData[DIR_ENTRY_attributes] & ATTRIB_VOL)) {
			if (lfnExists) {
				// Calculate file checksum
				chkSum = 0;
				for (i=0; i < 11; i++) {
					// NOTE: The operation is an unsigned char rotate right
					chkSum = ((chkSum & 1) ? 0x80 : 0) + (chkSum >> 1) + entryData[i]; 
				}
				if (chkSum != lfnChkSum) {
					lfnExists = false;
					//filename[0] = '\0';
					//unicodeFilename[0] = 0x0000; // move this line to below(1)
				}
			}
			//short name
			if (!lfnExists) {
				entryStart = entryEnd;
				unicodeFilename[0] = 0x0000; // (1)make sure clear previous search junk
				// get alias anyway
				_FAT_directory_entryGetAlias (entryData, filename);
			}
			//long name
			else
			{
				_FAT_unicode16_to_utf8 (unicodeFilename, filename);
			}

			found = true;
		}
	}

	// If no file is found, return false
	if (notFound) {
		return false;
	} else {
		// Fill in the directory entry struct
		entry->dataStart = entryStart;
		entry->dataEnd = entryEnd;
		memcpy (entry->entryData, entryData, DIR_ENTRY_DATA_SIZE);
		return true;
	}
}

bool _FAT_directory_getFirstEntry (PARTITION* partition, DIR_ENTRY* entry, u32 dirCluster) {
	entry->dataStart.cluster = dirCluster;
	entry->dataStart.sector = 0;
	entry->dataStart.offset = -1; // Start before the beginning of the directory

	entry->dataEnd = entry->dataStart;

	return _FAT_directory_getNextEntry (partition, entry);
}

bool _FAT_directory_getRootEntry (PARTITION* partition, DIR_ENTRY* entry) {
	entry->dataStart.cluster = 0;
	entry->dataStart.sector = 0;
	entry->dataStart.offset = 0;
	
	entry->dataEnd = entry->dataStart;	
	
	memset (entry->d_name, '\0', MAX_FILENAME_LENGTH);
	entry->d_name[0] = '.';
	
	memset (entry->entryData, 0, DIR_ENTRY_DATA_SIZE);
	memset (entry->entryData, ' ', 11);
	entry->entryData[0] = '.';
	
	entry->entryData[DIR_ENTRY_attributes] = ATTRIB_DIR;
	
	u16_to_u8array (entry->entryData, DIR_ENTRY_cluster, partition->rootDirCluster);
	u16_to_u8array (entry->entryData, DIR_ENTRY_clusterHigh, partition->rootDirCluster >> 16);
	
	return true;
}

bool _FAT_directory_entryFromPosition (PARTITION* partition, DIR_ENTRY* entry) {
	DIR_ENTRY_POSITION entryStart;
	DIR_ENTRY_POSITION entryEnd;
	entryStart = entry->dataStart;
	entryEnd = entry->dataEnd;
	bool entryStillValid;
	bool finished;
	
	int i;
	int lfnPos;

	u8 entryData[DIR_ENTRY_DATA_SIZE];
	
	memset (entry->d_name, '\0', MAX_FILENAME_LENGTH);

	// Create an empty directory entry to overwrite the old ones with
	for ( entryStillValid = true, finished = false; 
		entryStillValid && !finished; 
		entryStillValid = _FAT_directory_incrementDirEntryPosition (partition, &entryStart, false))
	{
		_FAT_cache_readPartialSector (partition->cache, entryData, 
			_FAT_fat_clusterToSector(partition, entryStart.cluster) + entryStart.sector,
			entryStart.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE);
			
		if ((entryStart.cluster == entryEnd.cluster)
			&& (entryStart.sector == entryEnd.sector)
			&& (entryStart.offset == entryEnd.offset)) {
			// Copy the entry data and stop, since this is the last section of the directory entry
			memcpy (entry->entryData, entryData, DIR_ENTRY_DATA_SIZE);
			finished = true;
		} else {
			// Copy the long file name data
			lfnPos = ((entryData[LFN_offset_ordinal] & ~LFN_END) - 1) * 13;
			for (i = 0; i < 13; i++) {
				entry->d_name[lfnPos + i] = entryData[LFN_offset_table[i]]; // modify this for unicode support;
			}
		}
	}

	if (!entryStillValid) {
		return false;
	}
	
	if ((entryStart.cluster == entryEnd.cluster)
		&& (entryStart.sector == entryEnd.sector)
		&& (entryStart.offset == entryEnd.offset)) {
		// Since the entry doesn't have a long file name, extract the short filename
		if (!_FAT_directory_entryGetAlias (entry->entryData, entry->d_name)) {
			return false;
		}
	}

	return true;
}

bool _FAT_directory_entryFromPath (PARTITION* partition, DIR_ENTRY* entry, const char* path, const char* pathEnd) {
	size_t dirnameLength;
	const char* pathPosition;
	const char* nextPathPosition;

	//size_t uniDirnameLength;
	//u16 uniPath[MAX_FILENAME_LENGTH];
	//const u16 * uniPathPosition;
	//const u16 * uniNextPathPosition;

	u32 dirCluster;
	bool foundFile;

	//char alias[MAX_ALIAS_LENGTH];

	bool found, notFound;

	//_FAT_utf8_to_unicode16( path, uniPath );
	pathPosition = path;
	//uniPathPosition = uniPath;
	
	found = false;
	notFound = false;

	if (pathEnd == NULL) {
		// Set pathEnd to the end of the path string
		pathEnd = strchr (path, '\0');
	}

	if (pathPosition[0] == DIR_SEPARATOR) {
		// Start at root directory
		dirCluster = partition->rootDirCluster;
		// Consume separator(s)
		while (pathPosition[0] == DIR_SEPARATOR) {
			pathPosition++;
		}

		//while (uniPathPosition[0] == (unsigned short)DIR_SEPARATOR) {
		//	uniPathPosition++;
		//}

		if (pathPosition >= pathEnd)  {
			_FAT_directory_getRootEntry (partition, entry);
			found = true;
		}
	} else {
		// Start in current working directory
		dirCluster = partition->cwdCluster;
	}

	// If the path is only specifying a directory in the form "." 
	// and this is the root directory, return it
	//if ((dirCluster == partition->rootDirCluster) && (strncasecmp(".", pathPosition, 2) == 0)) {
	if ((dirCluster == partition->rootDirCluster) && strlen(pathPosition) == 1 && (strcasecmp(".", pathPosition) == 0)) {
		_FAT_directory_getRootEntry (partition, entry);
		found = true;
	}

	while (!found && !notFound) {
		// Get the name of the next required subdirectory within the path
		nextPathPosition = strchr (pathPosition, DIR_SEPARATOR);
		//uniNextPathPosition = _unistrchr( uniPathPosition, (unsigned short)DIR_SEPARATOR );
		if (nextPathPosition != NULL) {
			dirnameLength = nextPathPosition - pathPosition;
		} else {
			dirnameLength = strlen(pathPosition);
		}
		//if (uniNextPathPosition != 0x0000) {
		//	uniDirnameLength = uniNextPathPosition - uniPathPosition;
		//} else {
		//	uniDirnameLength = _unistrnlen(uniPathPosition, MAX_FILENAME_LENGTH);
		//}

		if (dirnameLength > MAX_FILENAME_LENGTH) {
			// The path is too long to bother with
			return false;
		}
		//if( uniDirnameLength > MAX_FILENAME_LENGTH ) {
		//	return false;
		//}

		// Look for the directory within the path
		foundFile = _FAT_directory_getFirstEntry (partition, entry, dirCluster);

		while (foundFile && !found && !notFound) {			// It hasn't already found the file
			// Check if the filename matches
			//if ((uniDirnameLength == _unistrnlen(entry->unicodeFilename, MAX_FILENAME_LENGTH))
			//	&& (_unistrncmp(entry->unicodeFilename, uniPathPosition, uniDirnameLength) == 0)) {
			//		found = true;
			//}

			//if(!strncasecmp(entry->d_name, pathPosition, dirnameLength))
			if(strlen(entry->d_name) == dirnameLength  && !strncasecmp(entry->d_name, pathPosition, dirnameLength))
				found = true;

			// Check if the alias matches
			//_FAT_directory_entryGetAlias (entry->entryData, alias);
			//if ((dirnameLength == strnlen(alias, MAX_ALIAS_LENGTH))
			//	&& (strncasecmp(alias, pathPosition, dirnameLength) == 0)) {
			//		found = true;
			//}

			if (found && !(entry->entryData[DIR_ENTRY_attributes] & ATTRIB_DIR) && (nextPathPosition != NULL)) {
				// Make sure that we aren't trying to follow a file instead of a directory in the path
				found = false;
			}

			if (!found) {
				foundFile = _FAT_directory_getNextEntry (partition, entry);
			}
		}

		if (!foundFile) {
			// Check that the search didn't get to the end of the directory
			notFound = true;
			found = false;
		} else if ((nextPathPosition == NULL) || (nextPathPosition >= pathEnd)) {
			// Check that we reached the end of the path
			found = true;
		} else if (entry->entryData[DIR_ENTRY_attributes] & ATTRIB_DIR) {
			dirCluster = _FAT_directory_entryGetCluster (entry->entryData);
			pathPosition = nextPathPosition;
			//uniPathPosition = uniNextPathPosition;
			// Consume separator(s)
			while (pathPosition[0] == DIR_SEPARATOR) {
				pathPosition++;
			}
			//while (uniPathPosition[0] == (unsigned short)DIR_SEPARATOR) {
			//	uniPathPosition++;
			//}
			// The requested directory was found
			if (pathPosition >= pathEnd)  {
				found = true;
			} else {
				found = false;
			}
		}
	}

	if (found && !notFound) {
		return true;
	} else {
		return false;
	}
}

bool _FAT_directory_removeEntry (PARTITION* partition, DIR_ENTRY* entry) {
	DIR_ENTRY_POSITION entryStart;
	DIR_ENTRY_POSITION entryEnd;
	entryStart = entry->dataStart;
	entryEnd = entry->dataEnd;
	bool entryStillValid;
	bool finished;

	u8 entryData[DIR_ENTRY_DATA_SIZE];

	// Create an empty directory entry to overwrite the old ones with
	for ( entryStillValid = true, finished = false; 
		entryStillValid && !finished; 
		entryStillValid = _FAT_directory_incrementDirEntryPosition (partition, &entryStart, false))
	{
		_FAT_cache_readPartialSector (partition->cache, entryData, _FAT_fat_clusterToSector(partition, entryStart.cluster) + entryStart.sector, entryStart.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE);
		entryData[0] = DIR_ENTRY_FREE;
		_FAT_cache_writePartialSector (partition->cache, entryData, _FAT_fat_clusterToSector(partition, entryStart.cluster) + entryStart.sector, entryStart.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE);
		if ((entryStart.cluster == entryEnd.cluster) && (entryStart.sector == entryEnd.sector) && (entryStart.offset == entryEnd.offset)) {
			finished = true;
		}
	}

	if (!entryStillValid) {
		return false;
	}

	return true;
}

static bool _FAT_directory_findEntryGap (PARTITION* partition, DIR_ENTRY* entry, u32 dirCluster, u32 size) {
	DIR_ENTRY_POSITION gapStart;
	DIR_ENTRY_POSITION gapEnd;

	u8 entryData[DIR_ENTRY_DATA_SIZE];

	u32 dirEntryRemain;

	bool endOfDirectory, entryStillValid;

	// Scan Dir for free entry
	gapEnd.offset = 0;
	gapEnd.sector = 0;
	gapEnd.cluster = dirCluster;

	gapStart = gapEnd;

	entryStillValid = true;
	dirEntryRemain = size;
	endOfDirectory = false;
	
	while (entryStillValid && !endOfDirectory && (dirEntryRemain > 0)) {
		_FAT_cache_readPartialSector (partition->cache, entryData, _FAT_fat_clusterToSector(partition, gapEnd.cluster) + gapEnd.sector, gapEnd.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE);
		if (entryData[0] == DIR_ENTRY_LAST) {
			gapStart = gapEnd;
			-- dirEntryRemain;
			endOfDirectory = true;
		} else if (entryData[0] == DIR_ENTRY_FREE) {
			if (dirEntryRemain == size) {
				gapStart = gapEnd;
			}
			-- dirEntryRemain;
		} else {
			dirEntryRemain = size;
		}
		
		if (!endOfDirectory && (dirEntryRemain > 0)) {
			entryStillValid = _FAT_directory_incrementDirEntryPosition (partition, &gapEnd, true);
		}
	}

	// Make sure the scanning didn't fail
	if (!entryStillValid) {
		return false;
	}

	// Save the start entry, since we know it is valid
	entry->dataStart = gapStart;

	if (endOfDirectory) {
		memset (entryData, DIR_ENTRY_LAST, DIR_ENTRY_DATA_SIZE);
		dirEntryRemain += 1;	// Increase by one to take account of End Of Directory Marker
		while ((dirEntryRemain > 0) && entryStillValid) {
			// Get the gapEnd before incrementing it, so the second to last one is saved
			entry->dataEnd = gapEnd;
			// Increment gapEnd, moving onto the next entry
			entryStillValid = _FAT_directory_incrementDirEntryPosition (partition, &gapEnd, true);
			-- dirEntryRemain;
			// Fill the entry with blanks
			_FAT_cache_writePartialSector (partition->cache, entryData, _FAT_fat_clusterToSector(partition, gapEnd.cluster) + gapEnd.sector, gapEnd.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE);
		}
		if (!entryStillValid) {
			return false;
		}
	} else {
		entry->dataEnd = gapEnd;
	}

	return true;
}

static bool _FAT_directory_entryExists (PARTITION* partition, const char* name, u32 dirCluster) {
	DIR_ENTRY tempEntry;
	bool foundFile;
	//char alias[MAX_ALIAS_LENGTH];
	u32 dirnameLength;
	//u16 unicodeName[MAX_FILENAME_LENGTH];

	dirnameLength = strnlen(name, MAX_FILENAME_LENGTH);

	if (dirnameLength >= MAX_FILENAME_LENGTH) {
		return false;
	}
	
	//_FAT_utf8_to_unicode16( name, unicodeName );

	// Make sure the entry doesn't already exist
	foundFile = _FAT_directory_getFirstEntry (partition, &tempEntry, dirCluster);

	while (foundFile) {			// It hasn't already found the file
		// Check if the filename matches
		//if (/*(dirnameLength == _unistrnlen(tempEntry.unicodeFilename, MAX_FILENAME_LENGTH))*/
		//	//&& (strcasecmp(tempEntry.filename, name) == 0)) {
		//	/*&&*/ (_unistrncmp( unicodeName, tempEntry.unicodeFilename, MAX_FILENAME_LENGTH ) == 0 ))
		//{
		//		return true;
		//}
		//if(!strncasecmp(name, tempEntry.d_name, dirnameLength))
		if(!strcasecmp(name, tempEntry.d_name))
			return true;

		// Check if the alias matches
		//_FAT_directory_entryGetAlias (tempEntry.entryData, alias);
		//if ((dirnameLength == strnlen(alias, MAX_ALIAS_LENGTH))
		//	&& (strcasecmp(alias, name) == 0)) {
		//		return true;
		//}
		foundFile = _FAT_directory_getNextEntry (partition, &tempEntry);
	}
	return false;
}



bool _FAT_directory_addEntry (PARTITION* partition, DIR_ENTRY* entry, u32 dirCluster) {
	u32 entrySize;
	u8 lfnEntry[DIR_ENTRY_DATA_SIZE];
	s32 i,j; // Must be signed for use when decrementing in for loop
	char *tmpCharPtr;
	DIR_ENTRY_POSITION curEntryPos;
	bool entryStillValid;
	u8 aliasCheckSum = 0;
	char alias [MAX_ALIAS_LENGTH];
	u16 unicodeFilename[256];

	// Make sure the filename is not 0 length
	if (strnlen (entry->d_name, MAX_FILENAME_LENGTH) < 1) {
		return false;
	}
	// Make sure the filename is at least a valid LFN
	if ( !(_FAT_directory_isValidLfn (entry->d_name))) {
		return false;
	}

	// Remove trailing spaces
	for (i = strlen (entry->d_name) - 1; (i > 0) && (entry->d_name[i] == ' '); --i) {
		entry->d_name[i] = '\0';
	}
	// Remove leading spaces
	for (i = 0; (i < strlen (entry->d_name)) && (entry->d_name[i] == ' '); ++i) ;
	if (i > 0) {
		memmove (entry->d_name, entry->d_name + i, strlen (entry->d_name + i));
	}

	// Remove junk in filename
	i = strlen (entry->d_name);
	memset (entry->d_name + i, '\0', MAX_FILENAME_LENGTH - i);

	// Make sure the entry doesn't already exist
	if (_FAT_directory_entryExists (partition, entry->d_name, dirCluster)) {
		return false;
	}

	// Clear out alias, so we can generate a new one
	memset (entry->entryData, ' ', 11);

	if ( strncmp(entry->d_name, ".", MAX_FILENAME_LENGTH) == 0) {
		// "." entry
		entry->entryData[0] = '.';
		entrySize = 1;
	} else if ( strncmp(entry->d_name, "..", MAX_FILENAME_LENGTH) == 0) {
		// ".." entry
		entry->entryData[0] = '.';
		entry->entryData[1] = '.';
		entrySize = 1;
	} else if ( _FAT_directory_isValidAlias (entry->d_name)) {
		// Short filename
		strupr (entry->d_name);
		entrySize = 1;
		// Copy into alias
		for (i = 0, j = 0; (j < 8) && (entry->d_name[i] != '.') && (entry->d_name[i] != '\0'); i++, j++) {
			entry->entryData[j] = entry->d_name[i];
		}
		while (j < 8) {
			entry->entryData[j] = ' ';
			++ j;
		}
		if (entry->d_name[i] == '.') {
			// Copy extension
			++ i;
			while ((entry->d_name[i] != '\0') && (j < 11)) {
				entry->entryData[j] = entry->d_name[i];
				++ i;
				++ j;
			}
		}
		while (j < 11) {
			entry->entryData[j] = ' ';
			++ j;
		}
	} else {
		// Long filename needed
		//memset( entry->unicodeFilename, 0, 512 );
		//_FAT_utf8_to_unicode16( (const char*)entry->d_name, entry->unicodeFilename );
		memset( unicodeFilename, 0, 512 );
		_FAT_utf8_to_unicode16( (const char*)entry->d_name, unicodeFilename );

		//entrySize = ((strnlen (entry->filename, MAX_FILENAME_LENGTH) + LFN_ENTRY_LENGTH - 1) / LFN_ENTRY_LENGTH) + 1;
		//entrySize = ((_unistrnlen( entry->unicodeFilename, MAX_FILENAME_LENGTH ) + LFN_ENTRY_LENGTH - 1) / LFN_ENTRY_LENGTH) + 1;
		entrySize = ((_unistrnlen( unicodeFilename, MAX_FILENAME_LENGTH ) + LFN_ENTRY_LENGTH - 1) / LFN_ENTRY_LENGTH) + 1;		

		// Generate alias
		tmpCharPtr = strrchr (entry->d_name, '.');
		if (tmpCharPtr == NULL) {
			tmpCharPtr = strrchr (entry->d_name, '\0');
		}
		for (i = 0, j = 0; (j < 6) && (entry->d_name + i < tmpCharPtr); i++) {
			if ( _uniisalnum((u8)(entry->d_name[i]))) {
				alias[j] = entry->d_name[i];
				++ j;
			}
		}
		while (j < 8) {
			alias[j] = '_';
			++ j;
		}
		tmpCharPtr = strrchr (entry->d_name, '.');
		if (tmpCharPtr != NULL) {
			alias[8] = '.';
			// Copy extension
			while ((tmpCharPtr != '\0') && (j < 12)) {
				alias[j] = tmpCharPtr[0];
				++ tmpCharPtr;
				++ j;
			}
			alias[j] = '\0';
		} else {
			for (j = 8; j < MAX_ALIAS_LENGTH; j++) {
				alias[j] = '\0';
			}
		}

		// Get a valid tail number
		alias[5] = '~';
		i = 0;
		do {
			i++;
			alias[6] = '0' + ((i / 10) % 10);	// 10's digit
			alias[7] = '0' + (i % 10);	// 1's digit
		} while (_FAT_directory_entryExists (partition, alias, dirCluster) && (i < 100));
		if (i == 100) {
			// Couldn't get a tail number
			return false;
		}

		// Make it upper case
		strupr (alias);

		// Now copy it into the directory entry data
		memcpy (entry->entryData, alias, 8);
		memcpy (entry->entryData + 8, alias + 9, 3);
		for (i = 0; i < 10; i++) {
			if (entry->entryData[i] < 0x20) {
				// Replace null and control characters with spaces
				entry->entryData[i] = 0x20;
			}
		}	
		// Generate alias checksum
		for (i=0; i < 11; i++)
		{
			// NOTE: The operation is an unsigned char rotate right
			aliasCheckSum = ((aliasCheckSum & 1) ? 0x80 : 0) + (aliasCheckSum >> 1) + entry->entryData[i];
		}

	}

	// Find or create space for the entry
	if (_FAT_directory_findEntryGap (partition, entry, dirCluster, entrySize) == false) {
		return false;
	}

	// Write out directory entry
	curEntryPos = entry->dataStart;

	for (entryStillValid = true, i = entrySize; entryStillValid && i > 0; 
		entryStillValid = _FAT_directory_incrementDirEntryPosition (partition, &curEntryPos, false), -- i )
	{
		if (i > 1) {
			// Long filename entry
			lfnEntry[LFN_offset_ordinal] = (i - 1) | (i == entrySize ? LFN_END : 0);
			for (j = 0; j < 13; j++) {
				//if (entry->unicodeFilename[(i - 2) * 13 + j] == '\0') {
				if (unicodeFilename[(i - 2) * 13 + j] == '\0') {
					//if ((j > 1) && (entry->unicodeFilename[(i - 2) * 13 + (j-1)] == '\0')) {
					if ((j > 1) && (unicodeFilename[(i - 2) * 13 + (j-1)] == '\0')) {
						u16_to_u8array (lfnEntry, LFN_offset_table[j], 0xffff);		// Padding
					} else {
						u16_to_u8array (lfnEntry, LFN_offset_table[j], 0x0000);		// Terminating null character
					}
				} else {
					//u16_to_u8array (lfnEntry, LFN_offset_table[j], entry->unicodeFilename[(i - 2) * 13 + j]);
					u16_to_u8array (lfnEntry, LFN_offset_table[j], unicodeFilename[(i - 2) * 13 + j]);
				}
			}

			lfnEntry[LFN_offset_checkSum] = aliasCheckSum;
			lfnEntry[LFN_offset_flag] = ATTRIB_LFN;
			lfnEntry[LFN_offset_reserved1] = 0;
			u16_to_u8array (lfnEntry, LFN_offset_reserved2, 0);
			_FAT_cache_writePartialSector (partition->cache, lfnEntry, _FAT_fat_clusterToSector(partition, curEntryPos.cluster) + curEntryPos.sector, curEntryPos.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE);
		} else {
			// Alias & file data
			_FAT_cache_writePartialSector (partition->cache, entry->entryData, _FAT_fat_clusterToSector(partition, curEntryPos.cluster) + curEntryPos.sector, curEntryPos.offset * DIR_ENTRY_DATA_SIZE, DIR_ENTRY_DATA_SIZE);
		}
	}

	return true;
}

bool _FAT_directory_chdir (PARTITION* partition, const char* path) {
	DIR_ENTRY entry;

	if (!_FAT_directory_entryFromPath (partition, &entry, path, NULL)) {
		return false;
	}

	if (!(entry.entryData[DIR_ENTRY_attributes] & ATTRIB_DIR)) {
		return false;
	}

	partition->cwdCluster = _FAT_directory_entryGetCluster (entry.entryData);

	return true;
}

void _FAT_directory_entryStat (PARTITION* partition, DIR_ENTRY* entry, struct stat *st) {
	// Fill in the stat struct
	// Some of the values are faked for the sake of compatibility
	st->st_dev = _FAT_disc_hostType(partition->disc);					// The device is the 32bit ioType value
  	st->st_ino = (ino_t)(_FAT_directory_entryGetCluster(entry->entryData));		// The file serial number is the start cluster
	st->st_mode = (_FAT_directory_isDirectory(entry) ? S_IFDIR : S_IFREG) |
		(S_IRUSR | S_IRGRP | S_IROTH) |
		(_FAT_directory_isWritable (entry) ? (S_IWUSR | S_IWGRP | S_IWOTH) : 0);		// Mode bits based on dirEntry ATTRIB byte
	st->st_nlink = 1;								// Always one hard link on a FAT file
	st->st_uid = 1;									// Faked for FAT
	st->st_gid = 2;									// Faked for FAT
	st->st_rdev = st->st_dev;
	st->st_size = u8array_to_u32 (entry->entryData, DIR_ENTRY_fileSize);		// File size 
	st->st_atime = _FAT_filetime_to_time_t (
		0,
		u8array_to_u16 (entry->entryData, DIR_ENTRY_aDate)
	);
//	st->st_spare1 = 0;
	st->st_mtime = _FAT_filetime_to_time_t (
		u8array_to_u16 (entry->entryData, DIR_ENTRY_mTime),
		u8array_to_u16 (entry->entryData, DIR_ENTRY_mDate)
	);
//	st->st_spare2 = 0;
	st->st_ctime = _FAT_filetime_to_time_t (
		u8array_to_u16 (entry->entryData, DIR_ENTRY_cTime),
		u8array_to_u16 (entry->entryData, DIR_ENTRY_cDate)
	);
//	st->st_spare3 = 0;
	st->st_blksize = BYTES_PER_READ;				// Prefered file I/O block size 
	st->st_blocks = (st->st_size + BYTES_PER_READ - 1) / BYTES_PER_READ;	// File size in blocks
//	st->st_spare4[0] = 0;
//	st->st_spare4[1] = 0;
}
