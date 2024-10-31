// https://github.com/EddieEldridge/SHA256-in-C/ unlicensed

#ifndef _KEYHASH_H_
#define _KEYHASH_H_

#include <PiSmm.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/SmmMemLib.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/SmmMemoryAttribute.h>
#include <Protocol/SmmLegacyDispatch.h>

#define MAXCHAR 100000

#define ByteSwap32(x) (((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24))
#define ByteSwap64(x) ( \
	(((x) >> 56) & 0x00000000000000FF) | (((x) >> 40) & 0x000000000000FF00) | \
    (((x) >> 24) & 0x0000000000FF0000) | (((x) >>  8) & 0x00000000FF000000) | \
	(((x) <<  8) & 0x000000FF00000000) | (((x) << 24) & 0x0000FF0000000000) | \
	(((x) << 40) & 0x00FF000000000000) | (((x) << 56) & 0xFF00000000000000))

typedef union   KVH_MESSAGE_BLOCK KVH_MESSAGE_BLOCK; 
typedef enum    KVH_STATUS        KVH_STATUS;

union KVH_MESSAGE_BLOCK
{
    UINT8       e[64];
    UINT32      t[16];
    UINT64      s[8];
};

enum KVH_STATUS { READ, PAD0, PAD1, FINISH };

UINT32  SIG0  (UINT32 x);
UINT32  SIG1  (UINT32 x);
UINT32  ROTR  (UINT32 n, UINT16 x);
UINT32  SHR   (UINT32 n, UINT16 x);

UINT32  _SIG0 (UINT32 x);
UINT32  _SIG1 (UINT32 x);

UINT32  CH    (UINT32 x, UINT32 y, UINT32 z);
UINT32  MAJ   (UINT32 x, UINT32 y, UINT32 z);

BOOLEAN CheckIsBigEndian    ();
VOID    CalculateHash       (VOID*, UINTN, UINT32*);
UINTN   FillMessageBlock    (VOID*, UINTN, KVH_MESSAGE_BLOCK*, KVH_STATUS*, UINTN*);

#endif
