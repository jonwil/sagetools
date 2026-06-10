/*==========================================================================;
 *
 *  Copyright (C) Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       d3d9xps.h
 *  Content:    Xbox 360 include file for using Xbox Procedural Synthesis
 *
 ****************************************************************************/

#ifndef _D3D9D3DXPS_H_
#define _D3D9D3DXPS_H_

// Prototype for the D3D-maintained, thread-specific XPS context given to
// an XPS routine:

typedef struct D3DXpsThread D3DXpsThread;

// Prototype for XPS routines:

typedef void (*D3DXpsCallback)(D3DXpsThread* pThreadContext, void* pCallbackContext, CONST void* pSubmitData, DWORD InstanceIndex);

// Size, in bytes, of the ring-buffer space consumed by various
// 'D3DXps' calls:

#define D3DXPS_DRAWVERTICES_SIZE                132
#define D3DXPS_DRAWINDEXEDVERTICES_SIZE         140

// Default command size:

#define D3DXPS_COMMAND_SIZE                     D3DXPS_DRAWINDEXEDVERTICES_SIZE

// Maximum number of threads that can be enabled for XPS:

#define D3DXPS_MAX_THREADS                      6

// Hardware fetch constant used for XPS:

#define D3DXPS_FETCH_CONSTANT (GPU_CONVERT_D3D_TO_HARDWARE_VERTEXFETCHCONSTANT(0) / 3)

//------------------------------------------------------------------------------
// The D3DXps class is only available with C++.

#ifdef __cplusplus

class D3DXps;

// Builder function prototypes:

void D3DXps_Initialize(D3DXps* pD3DXps, D3DXpsThread* pThreadContext);
void D3DXps_Uninitialize(D3DXps* pD3DXps);
void* D3DXps_Allocate(D3DXps* pD3DXps, DWORD DataSize, DWORD CommandSize);
void D3DXps_DrawVertices(D3DXps* pD3DXps, D3DPRIMITIVETYPE PrimitiveType, DWORD VertexCount, CONST void* pVertexData);
void D3DXps_DrawIndexedVertices(D3DXps* pD3DXps, D3DPRIMITIVETYPE PrimitiveType, DWORD IndexCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexData);
void D3DXps_KickOff(D3DXps* pD3DXps);
BOOL D3DXps_KickOffAndGet(D3DXps* pD3DXps, DWORD* pInstanceIndex);
BOOL D3DXps_Get(D3DXps* pD3DXps, DWORD* pInstanceIndex);

// 'D3DXps' is the class used by XPS routines to synchronize with the
// other XPS threads, to write to the L2 buffer, and to kick-off commands
// to the GPU.
//
// NOTE: This object is intended to be instantiated on the stack at the
//       entry of every XPS data generation routine.
//
// NOTE: Once final hardware arrives, all of the member functions will become
//       '__forceinline' and so will always be inlined in the calling routine.

class D3DXps
{
private:

    DWORD m_Reserved[100];

public:

    D3DXps(D3DXpsThread* pThreadContext)
    {
        D3DXps_Initialize(this, pThreadContext);
    }

    ~D3DXps()
    {
        D3DXps_Uninitialize(this);
    }

    void* WINAPI Allocate(DWORD DataSize, DWORD CommandSize = D3DXPS_COMMAND_SIZE)
    {
        return D3DXps_Allocate(this, DataSize, CommandSize);
    }

    void WINAPI DrawVertices(D3DPRIMITIVETYPE PrimitiveType, DWORD VertexCount, CONST void* pVertexData)
    {
        D3DXps_DrawVertices(this, PrimitiveType, VertexCount, pVertexData);
    }

    void WINAPI DrawIndexedVertices(D3DPRIMITIVETYPE PrimitiveType, DWORD IndexCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexData)
    {
        D3DXps_DrawIndexedVertices(this, PrimitiveType, IndexCount, pIndexData, IndexDataFormat, pVertexData);
    }

    void WINAPI KickOff()
    {
        D3DXps_KickOff(this);
    }

    BOOL WINAPI KickOffAndGet(DWORD* pInstanceIndex)
    {
        return D3DXps_KickOffAndGet(this, pInstanceIndex);
    }
};

#endif /* __cplusplus */

#endif /* _D3D9D3DXPS_H_ */
