
#include "KeyHash.h"

void CalculateHash(VOID *RawBuffer, UINTN Length, UINT32* H)
{   
    KVH_MESSAGE_BLOCK   MessageBlock;
    UINTN               NumberBitsRead;
    KVH_STATUS          State;

    UINT32 _A;
    UINT32 _B;
    UINT32 _C;
    UINT32 _D;
    UINT32 _E;
    UINT32 _F;
    UINT32 _G;
    UINT32 _H;
    UINTN  J;
    UINT32 T1;
    UINT32 T2;
    UINT32 W[64];

    UINT32 K[] =
    {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    

    UINT32 H_TEMPLATE[] = 
    {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    DEBUG((DEBUG_INFO, "%a: %p %p %p\n", __func__, &MessageBlock, &W, &H_TEMPLATE));

    NumberBitsRead  = 0;
    State           = READ;

    CopyMem(H, &H_TEMPLATE, sizeof(H_TEMPLATE));

    while(FillMessageBlock(RawBuffer, Length, &MessageBlock, &State, &NumberBitsRead))
    {
        for(J = 0; J < 16; ++J) {
            W[J] = (CheckIsBigEndian ? (MessageBlock.t[J]) : (ByteSwap32(MessageBlock.t[J])));
        }

        for (J = 16; J < 64; ++J)
        {
            W[J] = SIG1( W[J-2] ) + W[J-7] + SIG0( W[J-15] ) + W[J-16];
        }

        _A = H[0];
        _B = H[1];
        _C = H[2];
        _D = H[3];
        _E = H[4];
        _F = H[5];
        _G = H[6];
        _H = H[7];

        for(J = 0; J < 64; ++J)
        {
            T1 = _H + _SIG1(_E) + CH(_E, _F, _G) + K[J] + W[J];
            T2 = _SIG0(_A) + MAJ(_A, _B, _C);
            _H = _G;
            _G = _F;
            _F = _E;
            _E = _D + T1;
            _D = _C;
            _C = _B;
            _B = _A;
            _A = T1 + T2;
        }

        H[0] = _A + H[0];
        H[1] = _B + H[1];
        H[2] = _C + H[2];
        H[3] = _D + H[3];
        H[4] = _E + H[4];
        H[5] = _F + H[5];
        H[6] = _G + H[6];
        H[7] = _H + H[7];
    }
}

UINTN FillMessageBlock(
    IN      VOID                *RawBuffer,
    IN      UINTN               Length,
    IN OUT  KVH_MESSAGE_BLOCK   *MessageBlock,
    IN OUT  KVH_STATUS          *State,
    IN OUT  UINTN               *NumberBitsRead
    )
{   
    UINTN NumberBytesLeft;
    UINTN I;

    if(*State == FINISH)
    {
        return 0;
    }

    if(*State == PAD0 || *State == PAD1)
    {
        for(I = 0; I < 56; ++I)
        {
            MessageBlock->e[I] = 0x00;
        }

        MessageBlock->s[7] = ByteSwap64(*NumberBitsRead);
        *State = FINISH;
        return 1;
    }

    NumberBytesLeft = Length - (*NumberBitsRead / 8);
    NumberBytesLeft = ((NumberBytesLeft > 64) ? 64 : 0);
    CopyMem((VOID *)&(MessageBlock->e), (VOID *)RawBuffer, NumberBytesLeft);

    *NumberBitsRead = *NumberBitsRead + (NumberBytesLeft * 8);

    if (NumberBytesLeft < 56)
    {
        MessageBlock->e[NumberBytesLeft] = 0x80;
        while(NumberBytesLeft < 56)
        {
            NumberBytesLeft = NumberBytesLeft + 1;
            MessageBlock->e[NumberBytesLeft] = 0x00;
        }

        MessageBlock->s[7] = ByteSwap64(*NumberBitsRead);
        *State = FINISH;
    } else if (NumberBytesLeft < 64) {   
        *State = PAD0;
        MessageBlock->e[NumberBytesLeft] = 0x80;

        while(NumberBytesLeft < 64)
        {
            NumberBytesLeft = NumberBytesLeft + 1;
            MessageBlock->e[NumberBytesLeft] = 0x00;
        }
    } else if (NumberBytesLeft == 0)
    {
        *State = PAD1;
    }
    
    return 1;
}

BOOLEAN CheckIsBigEndian()
{
    UINTN N = 1;
    if(*(CHAR8 *)&N == 1) {
        return FALSE;
    }

    return TRUE;
}

UINT32 SIG0(UINT32 x)
{
	return (ROTR(x, 7) ^ ROTR(x, 18) ^ SHR(x, 3));
};

UINT32 SIG1(UINT32 x)
{
	return (ROTR(x, 17) ^ ROTR(x, 19) ^ SHR(x, 10));
};

UINT32 ROTR(UINT32 x, UINT16 a)
{
	return (x >> a) | (x << (32 - a));
};

UINT32 SHR(UINT32 x, UINT16 b)
{
	return (x >> b);
};

UINT32 _SIG0(UINT32 x)
{
	return (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22));
};

UINT32 _SIG1(UINT32 x)
{
	return (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25));
};

UINT32 CH(UINT32 x,UINT32 y,UINT32 z)
{
	return ((x & y) ^ (~(x)&z));
};

UINT32 MAJ(UINT32 x,UINT32 y,UINT32 z)
{
	return ((x & y) ^ (x & z) ^ (y & z));
};