/*
    Copyright 2019 Hydr8gon

    This file is part of NooDS.

    NooDS is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    NooDS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with NooDS. If not, see <https://www.gnu.org/licenses/>.
*/

#include <cstdio>
#include <cstring>

#include "gpu_3d.h"
#include "defines.h"
#include "interpreter.h"

Gpu3D::Gpu3D(Interpreter *arm9): arm9(arm9)
{
    // Set the parameter counts
    paramCounts[0x10] = 1;
    paramCounts[0x11] = 0;
    paramCounts[0x12] = 1;
    paramCounts[0x13] = 1;
    paramCounts[0x14] = 1;
    paramCounts[0x15] = 0;
    paramCounts[0x16] = 16;
    paramCounts[0x17] = 12;
    paramCounts[0x18] = 16;
    paramCounts[0x19] = 12;
    paramCounts[0x1A] = 9;
    paramCounts[0x1B] = 3;
    paramCounts[0x1C] = 3;
    paramCounts[0x20] = 1;
    paramCounts[0x21] = 1;
    paramCounts[0x22] = 1;
    paramCounts[0x23] = 2;
    paramCounts[0x24] = 1;
    paramCounts[0x25] = 1;
    paramCounts[0x26] = 1;
    paramCounts[0x27] = 1;
    paramCounts[0x28] = 1;
    paramCounts[0x29] = 1;
    paramCounts[0x2A] = 1;
    paramCounts[0x2B] = 1;
    paramCounts[0x30] = 1;
    paramCounts[0x31] = 1;
    paramCounts[0x32] = 1;
    paramCounts[0x33] = 1;
    paramCounts[0x34] = 32;
    paramCounts[0x40] = 1;
    paramCounts[0x41] = 0;
    paramCounts[0x50] = 1;
    paramCounts[0x60] = 1;
    paramCounts[0x70] = 3;
    paramCounts[0x71] = 2;
    paramCounts[0x72] = 1;
}

void Gpu3D::runCycle()
{
    // Fetch the next geometry command
    Entry entry = pipe.front();
    pipe.pop();

    // Execute the geometry command
    switch (entry.command)
    {
        case 0x10: mtxModeCmd(entry.param);       break; // MTX_MODE
        case 0x11: mtxPushCmd();                  break; // MTX_PUSH
        case 0x12: mtxPopCmd(entry.param);        break; // MTX_POP
        case 0x13: mtxStoreCmd(entry.param);      break; // MTX_STORE
        case 0x14: mtxRestoreCmd(entry.param);    break; // MTX_RESTORE
        case 0x15: mtxIdentityCmd();              break; // MTX_IDENTITY
        case 0x16: mtxLoad44Cmd(entry.param);     break; // MTX_LOAD_4x4
        case 0x17: mtxLoad43Cmd(entry.param);     break; // MTX_LOAD_4x3
        case 0x18: mtxMult44Cmd(entry.param);     break; // MTX_MULT_4x4
        case 0x19: mtxMult43Cmd(entry.param);     break; // MTX_MULT_4x3
        case 0x1A: mtxMult33Cmd(entry.param);     break; // MTX_MULT_3x3
        case 0x1B: mtxScaleCmd(entry.param);      break; // MTX_SCALE
        case 0x1C: mtxTransCmd(entry.param);      break; // MTX_TRANS
        case 0x20: colorCmd(entry.param);         break; // COLOR
        case 0x22: texCoordCmd(entry.param);      break; // TEXCOORD
        case 0x23: vtx16Cmd(entry.param);         break; // VTX_16
        case 0x24: vtx10Cmd(entry.param);         break; // VTX_10
        case 0x25: vtxXYCmd(entry.param);         break; // VTX_XY
        case 0x26: vtxXZCmd(entry.param);         break; // VTX_XZ
        case 0x27: vtxYZCmd(entry.param);         break; // VTX_YZ
        case 0x28: vtxDiffCmd(entry.param);       break; // VTX_DIFF
        case 0x2A: texImageParamCmd(entry.param); break; // TEXIMAGE_PARAM
        case 0x2B: plttBaseCmd(entry.param);      break; // PLTT_BASE
        case 0x40: beginVtxsCmd(entry.param);     break; // BEGIN_VTXS
        case 0x41:                                break; // END_VTXS
        case 0x50: swapBuffersCmd(entry.param);   break; // SWAP_BUFFERS

        default:
        {
            printf("Unknown GXFIFO command: 0x%X\n", entry.command);
            break;
        }
    }

    // Keep track of how many parameters have been sent
    paramCount++;
    if (paramCount >= paramCounts[entry.command])
        paramCount = 0;

    // Move 2 FIFO entries into the PIPE if it runs half empty
    if (pipe.size() < 3)
    {
        for (int i = 0; i < ((fifo.size() > 2) ? 2 : fifo.size()); i++)
        {
            pipe.push(fifo.front());
            fifo.pop();
        }
    }

    // Update the counters
    gxStat = (gxStat & ~0x00001F00) | (coordinatePtr <<  8); // Coordinate stack pointer
    gxStat = (gxStat & ~0x00002000) | (projectionPtr << 13); // Projection stack pointer
    gxStat = (gxStat & ~0x01FF0000) | (fifo.size()   << 16); // FIFO entries

    // Update the FIFO status
    if (fifo.size() < 128) gxStat |=  BIT(25); // Less than half full
    if (fifo.size() == 0)  gxStat |=  BIT(26); // Empty
    if (pipe.size() == 0)  gxStat &= ~BIT(27); // Commands not executing

    // Send a GXFIFO interrupt if enabled
    switch ((gxStat & 0xC0000000) >> 30)
    {
        case 1: if (gxStat & BIT(25)) arm9->sendInterrupt(21); break;
        case 2: if (gxStat & BIT(26)) arm9->sendInterrupt(21); break;
    }
}

void Gpu3D::swapBuffers()
{
    // Normalize the vertices and convert the X and Y coordinates to DS screen coordinates
    for (int i = 0; i < vertexCountIn; i++)
    {
        verticesIn[i].x = (( verticesIn[i].x *    128) / verticesIn[i].w) +    128;
        verticesIn[i].y = ((-verticesIn[i].y *     96) / verticesIn[i].w) +     96;
        verticesIn[i].z = (((verticesIn[i].z * 0x4000) / verticesIn[i].w) + 0x3FFF) * 0x200;
    }

    // Swap the vertex buffers
    Vertex *vertices = verticesOut;
    verticesOut = verticesIn;
    verticesIn = vertices;
    vertexCountOut = vertexCountIn;
    vertexCountIn = 0;
    vertexCount = 0;

    // Swap the polygon buffers
    _Polygon *polygons = polygonsOut;
    polygonsOut = polygonsIn;
    polygonsIn = polygons;
    polygonCountOut = polygonCountIn;
    polygonCountIn = 0;

    // Unhalt the geometry engine
    halted = false;
}

Matrix Gpu3D::multiply(Matrix *mtx1, Matrix *mtx2)
{
    Matrix matrix;

    // Multiply 2 matrices
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            int64_t *index = &matrix.data[y * 4 + x];
            *index = 0;
            for (int i = 0; i < 4; i++) *index += mtx1->data[y * 4 + i] * mtx2->data[i * 4 + x];
            *index >>= 12;
        }
    }

    return matrix;
}

Vertex Gpu3D::multiply(Vertex *vtx, Matrix *mtx)
{
    Vertex vertex = *vtx;

    // Multiply a vertex with a matrix
    vertex.x = (vtx->x * mtx->data[0] + vtx->y * mtx->data[4] + vtx->z * mtx->data[8]  + vtx->w * mtx->data[12]) >> 12;
    vertex.y = (vtx->x * mtx->data[1] + vtx->y * mtx->data[5] + vtx->z * mtx->data[9]  + vtx->w * mtx->data[13]) >> 12;
    vertex.z = (vtx->x * mtx->data[2] + vtx->y * mtx->data[6] + vtx->z * mtx->data[10] + vtx->w * mtx->data[14]) >> 12;
    vertex.w = (vtx->x * mtx->data[3] + vtx->y * mtx->data[7] + vtx->z * mtx->data[11] + vtx->w * mtx->data[15]) >> 12;

    return vertex;
}

void Gpu3D::addVertex()
{
    if (vertexCountIn >= 6144) return;

    // Set the new vertex
    verticesIn[vertexCountIn] = savedVertex;
    verticesIn[vertexCountIn].w = 1 << 12;

    // Update the clip matrix if necessary
    if (clipDirty)
    {
        clip = multiply(&coordinate, &projection);
        clipDirty = false;
    }

    // Transform the vertex
    verticesIn[vertexCountIn] = multiply(&verticesIn[vertexCountIn], &clip);

    // Move to the next vertex
    vertexCountIn++;
    vertexCount++;

    // Move to the next polygon if one has been completed
    switch (polygonType)
    {
        case 0: if (vertexCount % 3 == 0)                     addPolygon(); break; // Separate triangles
        case 1: if (vertexCount % 4 == 0)                     addPolygon(); break; // Separate quads
        case 2: if (vertexCount >= 3)                         addPolygon(); break; // Triangle strips
        case 3: if (vertexCount >= 4 && vertexCount % 2 == 0) addPolygon(); break; // Quad strips
    }
}

void Gpu3D::addPolygon()
{
    if (polygonCountIn >= 2048) return;

    // Set the polygon vertex information
    int size = 3 + (polygonType & 1);
    savedPolygon.size = size;
    savedPolygon.vertices = &verticesIn[vertexCountIn - size];

    Vertex unclipped[8];
    Vertex clipped[8];
    Vertex temp[8];

    // Save a copy of the unclipped vertices
    for (int i = 0; i < size; i++)
        unclipped[i] = savedPolygon.vertices[i];

    // Rearrange quad strip vertices to work with the clipping algorithm
    if (polygonType == 3)
    {
        Vertex vertex = unclipped[2];
        unclipped[2] = unclipped[3];
        unclipped[3] = vertex;
    }

    // Clip the polygon on all 6 sides of the view area
    bool clip = clipPolygon(unclipped, temp, 0);
    clip |= clipPolygon(temp, clipped, 1);
    clip |= clipPolygon(clipped, temp, 2);
    clip |= clipPolygon(temp, clipped, 3);
    clip |= clipPolygon(clipped, temp, 4);
    clip |= clipPolygon(temp, clipped, 5);

    // Discard polygons that are completely outside of the view area
    if (savedPolygon.size == 0)
    {
        switch (polygonType)
        {
            case 0: case 1: // Separate polygons
            {
                // Discard the vertices
                vertexCountIn -= size;
                return;
            }

            case 2: // Triangle strips
            {
                if (vertexCount == 3) // First triangle in the strip
                {
                    // Discard the first vertex, but keep the other 2 for the next triangle
                    verticesIn[vertexCountIn - 3] = verticesIn[vertexCountIn - 2];
                    verticesIn[vertexCountIn - 2] = verticesIn[vertexCountIn - 1];
                    vertexCountIn--;
                    vertexCount--;
                }
                else if (vertexCountIn < 6144)
                {
                    // End the previous strip, and start a new one with the last 2 vertices
                    verticesIn[vertexCountIn - 1] = verticesIn[vertexCountIn - 2];
                    verticesIn[vertexCountIn - 0] = verticesIn[vertexCountIn - 1];
                    vertexCountIn++;
                    vertexCount = 2;
                }
                return;
            }

            case 3: // Quad strips
            {
                if (vertexCount == 4) // First quad in the strip
                {
                    // Discard the first 2 vertices, but keep the other 2 for the next quad
                    verticesIn[vertexCountIn - 4] = verticesIn[vertexCountIn - 2];
                    verticesIn[vertexCountIn - 3] = verticesIn[vertexCountIn - 1];
                    vertexCountIn -= 2;
                    vertexCount -= 2;
                }
                else
                {
                    // End the previous strip, and start a new one with the last 2 vertices
                    vertexCount = 2;
                }
                return;
            }
        }
    }

    // Update the vertices of clipped polygons
    if (clip)
    {
        switch (polygonType)
        {
            case 0: case 1: // Separate polygons
            {
                // Remove the unclipped vertices
                vertexCountIn -= size;

                // Add the clipped vertices
                for (int i = 0; i < savedPolygon.size; i++)
                {
                    if (vertexCountIn >= 6144) return;
                    verticesIn[vertexCountIn] = clipped[i];
                    vertexCountIn++;
                }
                break;
            }

            case 2: // Triangle strips
            {
                // Remove the unclipped vertices
                vertexCountIn -= (vertexCount == 3) ? 3 : 1;
                savedPolygon.vertices = &verticesIn[vertexCountIn];

                // Add the clipped vertices
                for (int i = 0; i < savedPolygon.size; i++)
                {
                    if (vertexCountIn >= 6144) return;
                    verticesIn[vertexCountIn] = clipped[i];
                    vertexCountIn++;
                }

                // End the previous strip, and start a new one with the last 2 vertices
                for (int i = 0; i < 2; i++)
                {
                    if (vertexCountIn >= 6144) return;
                    verticesIn[vertexCountIn] = unclipped[1 + i];
                    vertexCountIn++;
                }
                vertexCount = 2;
                break;
            }

            case 3: // Quad strips
            {
                // Remove the unclipped vertices
                vertexCountIn -= (vertexCount == 4) ? 4 : 2;
                savedPolygon.vertices = &verticesIn[vertexCountIn];

                // Add the clipped vertices
                for (int i = 0; i < savedPolygon.size; i++)
                {
                    if (vertexCountIn >= 6144) return;
                    verticesIn[vertexCountIn] = clipped[i];
                    vertexCountIn++;
                }

                // End the previous strip, and start a new one with the last 2 vertices
                for (int i = 0; i < 2; i++)
                {
                    if (vertexCountIn >= 6144) return;
                    verticesIn[vertexCountIn] = unclipped[3 - i];
                    vertexCountIn++;
                }
                vertexCount = 2;
                break;
            }
        }
    }

    // Set the new polygon
    polygonsIn[polygonCountIn] = savedPolygon;
    polygonsIn[polygonCountIn].paletteAddr *= ((savedPolygon.textureFmt == 2) ? 8 : 16);

    // Move to the next polygon
    polygonCountIn++;
}

Vertex Gpu3D::intersection(Vertex *vtx1, Vertex *vtx2, int64_t val1, int64_t val2)
{
    Vertex vertex;

    // Calculate the interpolation coefficients
    int64_t d1 = val1 + vtx1->w;
    int64_t d2 = val2 + vtx2->w;
    if (d2 == d1) return *vtx1;

    // Interpolate the vertex coordinates
    vertex.x = ((vtx1->x * d2) - (vtx2->x * d1)) / (d2 - d1);
    vertex.y = ((vtx1->y * d2) - (vtx2->y * d1)) / (d2 - d1);
    vertex.z = ((vtx1->z * d2) - (vtx2->z * d1)) / (d2 - d1);
    vertex.w = ((vtx1->w * d2) - (vtx2->w * d1)) / (d2 - d1);
    vertex.s = ((vtx1->s * d2) - (vtx2->s * d1)) / (d2 - d1);
    vertex.t = ((vtx1->t * d2) - (vtx2->t * d1)) / (d2 - d1);

    // Interpolate the vertex color
    uint8_t r = ((((vtx1->color >>  0) & 0x3F) * d2) - (((vtx2->color >>  0) & 0x3F) * d1)) / (d2 - d1);
    uint8_t g = ((((vtx1->color >>  6) & 0x3F) * d2) - (((vtx2->color >>  6) & 0x3F) * d1)) / (d2 - d1);
    uint8_t b = ((((vtx1->color >> 12) & 0x3F) * d2) - (((vtx2->color >> 12) & 0x3F) * d1)) / (d2 - d1);
    vertex.color = (b << 12) | (g << 6) | r;

    return vertex;
}

bool Gpu3D::clipPolygon(Vertex *unclipped, Vertex *clipped, int side)
{
    bool clip = false;

    int size = savedPolygon.size;
    savedPolygon.size = 0;

    // Clip a polygon using the Sutherland-Hodgman algorithm
    for (int i = 0; i < size; i++)
    {
        // Get the unclipped vertices 
        Vertex *current = &unclipped[i];
        Vertex *previous = &unclipped[(i - 1 + size) % size];

        // Choose which coordinates to check based on the current side being clipped against
        int64_t currentVal, previousVal;
        switch (side)
        {
            case 0:  currentVal =  current->x; previousVal =  previous->x; break;
            case 1:  currentVal = -current->x; previousVal = -previous->x; break;
            case 2:  currentVal =  current->y; previousVal =  previous->y; break;
            case 3:  currentVal = -current->y; previousVal = -previous->y; break;
            case 4:  currentVal =  current->z; previousVal =  previous->z; break;
            default: currentVal = -current->z; previousVal = -previous->z; break;
        }

        // Add the clipped vertices
        if (currentVal >= -current->w) // Current vertex in bounds
        {
            if (previousVal < -previous->w) // Previous vertex not in bounds
            {
                clipped[savedPolygon.size] = intersection(current, previous, currentVal, previousVal);
                savedPolygon.size++;
                clip = true;
            }

            clipped[savedPolygon.size] = *current;
            savedPolygon.size++;
        }
        else if (previousVal >= -previous->w) // Previous vertex in bounds
        {
            clipped[savedPolygon.size] = intersection(current, previous, currentVal, previousVal);
            savedPolygon.size++;
            clip = true;
        }
    }

    return clip;
}

void Gpu3D::mtxModeCmd(uint32_t param)
{
    // Set the matrix mode
    matrixMode = param & 0x00000003;
}

void Gpu3D::mtxPushCmd()
{
    // Push the current matrix onto a stack
    switch (matrixMode)
    {
        case 0: // Projection stack
        {
            if (projectionPtr < 1)
            {
                // Push to the single projection stack slot and increment the pointer
                projectionStack = projection;
                projectionPtr++;
            }
            else
            {
                // Indicate a matrix stack overflow error
                gxStat |= BIT(15);
            }
            break;
        }

        case 1: case 2: // Coordinate and directional stacks
        {
            // Indicate a matrix stack overflow error
            // Even though the 31st slot exists, it still causes an overflow error
            if (coordinatePtr >= 30) gxStat |= BIT(15);

            // Push to the current coordinate and directional stack slots and increment the pointer
            if (coordinatePtr < 31)
            {
                coordinateStack[coordinatePtr] = coordinate;
                directionStack[coordinatePtr] = direction;
                coordinatePtr++;
            }
            break;
        }

        case 3: // Texture stack
        {
            // Push to the single texture stack slot
            textureStack = texture;
            break;
        }
    }
}

void Gpu3D::mtxPopCmd(uint32_t param)
{
    // Pop a matrix from a stack
    switch (matrixMode)
    {
        case 0: // Projection stack
        {
            if (projectionPtr > 0)
            {
                // Pop from the single projection stack slot and decrement the pointer
                projectionPtr--;
                projection = projectionStack;
                clipDirty = true;
            }
            else
            {
                // Indicate a matrix stack underflow error
                gxStat |= BIT(15);
            }
            break;
        }

        case 1: case 2: // Coordinate and directional stacks
        {
            // Calculate the stack address to pop from
            int address = coordinatePtr - (((param & BIT(5)) ? 0xFFFFFFC0 : 0) | (param & 0x0000003F));

            // Indicate a matrix stack underflow or overflow error
            // Even though the 31st slot exists, it still causes an overflow error
            if (address < 0 || address >= 30) gxStat |= BIT(15);

            // Pop from the current coordinate and directional stack slots and update the pointer
            if (address >= 0 && address < 31)
            {
                coordinate = coordinateStack[address];
                direction = directionStack[address];
                coordinatePtr = address;
                clipDirty = true;
            }
            break;
        }

        case 3: // Texture stack
        {
            // Pop from the single texture stack slot
            texture = textureStack;
            break;
        }
    }
}

void Gpu3D::mtxStoreCmd(uint32_t param)
{
    // Store a matrix to the stack
    switch (matrixMode)
    {
        case 0: // Projection stack
        {
            // Store to the single projection stack slot
            projectionStack = projection;
            break;
        }

        case 1: case 2: // Coordinate and directional stacks
        {
            // Get the stack address to store to
            int address = param & 0x0000001F;

            // Indicate a matrix stack overflow error
            // Even though the 31st slot exists, it still causes an overflow error
            if (address == 31) gxStat |= BIT(15);

            // Store to the current coordinate and directional stack slots
            coordinateStack[address] = coordinate;
            directionStack[address] = direction;
            break;
        }

        case 3: // Texture stack
        {
            // Store to the single texture stack slot
            textureStack = texture;
            break;
        }
    }
}

void Gpu3D::mtxRestoreCmd(uint32_t param)
{
    // Restore a matrix from the stack
    switch (matrixMode)
    {
        case 0: // Projection stack
        {
            // Restore from the single projection stack slot
            projection = projectionStack;
            clipDirty = true;
            break;
        }

        case 1: case 2: // Coordinate and directional stacks
        {
            // Get the stack address to store to
            int address = param & 0x0000001F;

            // Indicate a matrix stack overflow error
            // Even though the 31st slot exists, it still causes an overflow error
            if (address == 31) gxStat |= BIT(15);

            // Restore from the current coordinate and directional stack slots
            coordinate = coordinateStack[address];
            direction = directionStack[address];
            clipDirty = true;
            break;
       }

        case 3: // Texture stack
        {
            // Restore from the single texture stack slot
            texture = textureStack;
            break;
        }
    }
}

void Gpu3D::mtxIdentityCmd()
{
    // Set a matrix to the identity matrix
    switch (matrixMode)
    {
        case 0: // Projection stack
        {
            projection = Matrix();
            clipDirty = true;
            break;
        }

        case 1: // Coordinate stack
        {
            coordinate = Matrix();
            clipDirty = true;
            break;
        }

        case 2: // Coordinate and directional stacks
        {
            coordinate = Matrix();
            direction = Matrix();
            clipDirty = true;
            break;
        }

        case 3: // Texture stack
        {
            texture = Matrix();
            break;
        }
    }
}

void Gpu3D::mtxLoad44Cmd(uint32_t param)
{
    // Store the paramaters to the temporary matrix
    temp.data[paramCount] = (int32_t)param;

    if (paramCount < 15) return;

    // Set a 4x4 matrix
    switch (matrixMode)
    {
        case 0: // Projection stack
        {
            projection = temp;
            clipDirty = true;
            break;
        }

        case 1: // Coordinate stack
        {
            coordinate = temp;
            clipDirty = true;
            break;
        }

        case 2: // Coordinate and directional stacks
        {
            coordinate = temp;
            direction = temp;
            clipDirty = true;
            break;
        }

        case 3: // Texture stack
        {
            texture = temp;
            break;
        }
    }
}

void Gpu3D::mtxLoad43Cmd(uint32_t param)
{
    // Store the paramaters to the temporary matrix
    if (paramCount == 0) temp = Matrix();
    temp.data[(paramCount / 3) * 4 + paramCount % 3] = (int32_t)param;

    if (paramCount < 11) return;

    // Set a 4x3 matrix
    switch (matrixMode)
    {
        case 0: // Projection stack
        {
            projection = temp;
            clipDirty = true;
            break;
        }

        case 1: // Coordinate stack
        {
            coordinate = temp;
            clipDirty = true;
            break;
        }

        case 2: // Coordinate and directional stacks
        {
            coordinate = temp;
            direction = temp;
            clipDirty = true;
            break;
        }

        case 3: // Texture stack
        {
            texture = temp;
            break;
        }
    }
}

void Gpu3D::mtxMult44Cmd(uint32_t param)
{
    // Store the paramaters to the temporary matrix
    temp.data[paramCount] = (int32_t)param;

    if (paramCount < 15) return;

    // Multiply a matrix by a 4x4 matrix
    switch (matrixMode)
    {
        case 0: // Projection stack
        {
            projection = multiply(&temp, &projection);
            clipDirty = true;
            break;
        }

        case 1: // Coordinate stack
        {
            coordinate = multiply(&temp, &coordinate);
            clipDirty = true;
            break;
        }

        case 2: // Coordinate and directional stacks
        {
            coordinate = multiply(&temp, &coordinate);
            direction = multiply(&temp, &direction);
            clipDirty = true;
            break;
        }

        case 3: // Texture stack
        {
            texture = multiply(&temp, &texture);
            break;
        }
    }
}

void Gpu3D::mtxMult43Cmd(uint32_t param)
{
    // Store the paramaters to the temporary matrix
    if (paramCount == 0) temp = Matrix();
    temp.data[(paramCount / 3) * 4 + paramCount % 3] = (int32_t)param;

    if (paramCount < 11) return;

    // Multiply a matrix by a 4x3 matrix
    switch (matrixMode)
    {
        case 0: // Projection stack
        {
            projection = multiply(&temp, &projection);
            clipDirty = true;
            break;
        }

        case 1: // Coordinate stack
        {
            coordinate = multiply(&temp, &coordinate);
            clipDirty = true;
            break;
        }

        case 2: // Coordinate and directional stacks
        {
            coordinate = multiply(&temp, &coordinate);
            direction = multiply(&temp, &direction);
            clipDirty = true;
            break;
        }

        case 3: // Texture stack
        {
            texture = multiply(&temp, &texture);
            break;
        }
    }
}

void Gpu3D::mtxMult33Cmd(uint32_t param)
{
    // Store the paramaters to the temporary matrix
    if (paramCount == 0) temp = Matrix();
    temp.data[(paramCount / 3) * 4 + paramCount % 3] = (int32_t)param;

    if (paramCount < 8) return;

    // Multiply a matrix by a 3x3 matrix
    switch (matrixMode)
    {
        case 0: // Projection stack
        {
            projection = multiply(&temp, &projection);
            clipDirty = true;
            break;
        }

        case 1: // Coordinate stack
        {
            coordinate = multiply(&temp, &coordinate);
            clipDirty = true;
            break;
        }

        case 2: // Coordinate and directional stacks
        {
            coordinate = multiply(&temp, &coordinate);
            direction = multiply(&temp, &direction);
            clipDirty = true;
            break;
        }

        case 3: // Texture stack
        {
            texture = multiply(&temp, &texture);
            break;
        }
    }
}

void Gpu3D::mtxScaleCmd(uint32_t param)
{
    // Store the paramaters to the temporary matrix
    if (paramCount == 0) temp = Matrix();
    temp.data[paramCount * 5] = (int32_t)param;

    if (paramCount < 2) return;

    // Multiply a matrix by a scale matrix
    switch (matrixMode)
    {
        case 0: // Projection stack
        {
            projection = multiply(&temp, &projection);
            clipDirty = true;
            break;
        }

        case 1: case 2: // Coordinate stack
        {
            coordinate = multiply(&temp, &coordinate);
            clipDirty = true;
            break;
        }

        case 3: // Texture stack
        {
            texture = multiply(&temp, &texture);
            break;
        }
    }
}

void Gpu3D::mtxTransCmd(uint32_t param)
{
    // Store the paramaters to the temporary matrix
    if (paramCount == 0) temp = Matrix();
    temp.data[12 + paramCount] = (int32_t)param;

    if (paramCount < 2) return;

    // Multiply a matrix by a translation matrix
    switch (matrixMode)
    {
        case 0: // Projection stack
        {
            projection = multiply(&temp, &projection);
            clipDirty = true;
            break;
        }

        case 1: // Coordinate stack
        {
            coordinate = multiply(&temp, &coordinate);
            clipDirty = true;
            break;
        }

        case 2: // Coordinate and directional stacks
        {
            coordinate = multiply(&temp, &coordinate);
            direction = multiply(&temp, &direction);
            clipDirty = true;
            break;
        }

        case 3: // Texture stack
        {
            texture = multiply(&temp, &texture);
            break;
        }
    }
}

void Gpu3D::colorCmd(uint32_t param)
{
    // Set the vertex color (converted from RGB5 to RGB6)
    uint8_t r = ((param >>  0) & 0x1F); r = r * 2 + (r + 31) / 32;
    uint8_t g = ((param >>  5) & 0x1F); g = g * 2 + (g + 31) / 32;
    uint8_t b = ((param >> 10) & 0x1F); b = b * 2 + (b + 31) / 32;
    savedVertex.color = (b << 12) | (g << 6) | r;
}

void Gpu3D::texCoordCmd(uint32_t param)
{
    // Set the vertex texture coordinates
    savedVertex.s = (int16_t)(param >>  0);
    savedVertex.t = (int16_t)(param >> 16);

    // Transform the texture coordinates
    if (textureCoordMode == 1)
    {
        // Create a vertex with the texture coordinates
        Vertex vertex;
        vertex.x = savedVertex.s << 8;
        vertex.y = savedVertex.t << 8;
        vertex.z = 1 << 8;
        vertex.w = 1 << 8;

        // Multiply the vertex with the texture matrix
        vertex = multiply(&vertex, &texture);

        // Save the transformed coordinates
        savedVertex.s = vertex.x >> 8;
        savedVertex.t = vertex.y >> 8;
    }
}

void Gpu3D::vtx16Cmd(uint32_t param)
{
    if (paramCount == 0)
    {
        // Set the X and Y coordinates
        savedVertex.x = (int16_t)(param >>  0);
        savedVertex.y = (int16_t)(param >> 16);
    }
    else
    {
        // Set the Z coordinate
        savedVertex.z = (int16_t)param;

        addVertex();
    }
}

void Gpu3D::vtx10Cmd(uint32_t param)
{
    // Set the X, Y, and Z coordinates
    savedVertex.x = (int16_t)((param & 0x000003FF) << 6);
    savedVertex.y = (int16_t)((param & 0x000FFC00) >> 4);
    savedVertex.z = (int16_t)((param & 0x3FF00000) >> 14);

    addVertex();
}

void Gpu3D::vtxXYCmd(uint32_t param)
{
    // Set the X and Y coordinates
    savedVertex.x = (int16_t)(param >>  0);
    savedVertex.y = (int16_t)(param >> 16);

    addVertex();
}

void Gpu3D::vtxXZCmd(uint32_t param)
{
    // Set the X and Z coordinates
    savedVertex.x = (int16_t)(param >>  0);
    savedVertex.z = (int16_t)(param >> 16);

    addVertex();
}

void Gpu3D::vtxYZCmd(uint32_t param)
{
    // Set the Y and Z coordinates
    savedVertex.y = (int16_t)(param >>  0);
    savedVertex.z = (int16_t)(param >> 16);

    addVertex();
}

void Gpu3D::vtxDiffCmd(uint32_t param)
{
    // Add offsets to the X, Y, and Z coordinates
    savedVertex.x += ((int16_t)((param & 0x000003FF) << 6)  / 8) >> 3;
    savedVertex.y += ((int16_t)((param & 0x000FFC00) >> 4)  / 8) >> 3;
    savedVertex.z += ((int16_t)((param & 0x3FF00000) >> 14) / 8) >> 3;

    addVertex();
}

void Gpu3D::texImageParamCmd(uint32_t param)
{
    // Set the texture parameters
    savedPolygon.textureAddr = (param & 0x0000FFFF) * 8;
    savedPolygon.sizeS = 8 << ((param & 0x00700000) >> 20);
    savedPolygon.sizeT = 8 << ((param & 0x03800000) >> 23);
    savedPolygon.repeatS = param & BIT(16);
    savedPolygon.repeatT = param & BIT(17);
    savedPolygon.flipS = param & BIT(18);
    savedPolygon.flipT = param & BIT(19);
    savedPolygon.textureFmt = (param & 0x1C000000) >> 26;
    savedPolygon.transparent0 = param & BIT(29);
    textureCoordMode = (param & 0xC0000000) >> 30;
}

void Gpu3D::plttBaseCmd(uint32_t param)
{
    // Set the palette base address
    savedPolygon.paletteAddr = param & 0x00001FFF;
}

void Gpu3D::beginVtxsCmd(uint32_t param)
{
    // Clipping a polygon strip starts a new strip with the last 2 vertices of the old one
    // Discard these vertices if they're unused
    if (vertexCount < 3 + (polygonType & 1))
        vertexCountIn -= vertexCount;

    // Begin a new vertex list
    polygonType = param & 0x00000003;
    vertexCount = 0;
}

void Gpu3D::swapBuffersCmd(uint32_t param)
{
    // Set the W-buffering toggle
    savedPolygon.wBuffer = param & BIT(1);

    // Halt the geometry engine
    // The buffers will be swapped and the engine unhalted on next V-blank
    halted = true;
}

void Gpu3D::addEntry(Entry entry)
{
    if (fifo.empty() && pipe.size() < 4)
    {
        // Move data directly into the PIPE if the FIFO is empty and the PIPE isn't full
        pipe.push(entry);

        // Update the FIFO status
        gxStat |= BIT(27); // Commands executing
    }
    else
    {
        // If the FIFO is full, free space by running cycles
        // On real hardware, a GXFIFO overflow would halt the CPU until space is free
        while (fifo.size() >= 256)
            runCycle();

        // Move data into the FIFO
        fifo.push(entry);

        // Update the FIFO status
        gxStat = (gxStat & ~0x01FF0000) | (fifo.size() << 16); // Count
        if (fifo.size() >= 128) gxStat &= ~BIT(25); // Half or more full
        gxStat &= ~BIT(26); // Not empty
    }
}

void Gpu3D::writeGxFifo(uint32_t mask, uint32_t value)
{
    if (gxFifo == 0)
    {
        // Read new packed commands
        gxFifo = value & mask;
    }
    else
    {
        // Add a command parameter
        Entry entry(gxFifo, value & mask);
        addEntry(entry);
        gxFifoCount++;

        // Move to the next command once all parameters have been sent
        if (gxFifoCount == paramCounts[gxFifo & 0xFF])
        {
            gxFifo >>= 8;
            gxFifoCount = 0;
        }
    }

    // Add entries for commands with no parameters
    while (gxFifo != 0 && paramCounts[gxFifo & 0xFF] == 0)
    {
        Entry entry(gxFifo, 0);
        addEntry(entry);
        gxFifo >>= 8;
    }
}

void Gpu3D::writeMtxMode(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x10, value & mask);
    addEntry(entry);
}

void Gpu3D::writeMtxPush(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x11, value & mask);
    addEntry(entry);
}

void Gpu3D::writeMtxPop(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x12, value & mask);
    addEntry(entry);
}

void Gpu3D::writeMtxStore(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x13, value & mask);
    addEntry(entry);
}

void Gpu3D::writeMtxRestore(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x14, value & mask);
    addEntry(entry);
}

void Gpu3D::writeMtxIdentity(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x15, value & mask);
    addEntry(entry);
}

void Gpu3D::writeMtxLoad44(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x16, value & mask);
    addEntry(entry);
}

void Gpu3D::writeMtxLoad43(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x17, value & mask);
    addEntry(entry);
}

void Gpu3D::writeMtxMult44(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x18, value & mask);
    addEntry(entry);
}

void Gpu3D::writeMtxMult43(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x19, value & mask);
    addEntry(entry);
}

void Gpu3D::writeMtxMult33(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x1A, value & mask);
    addEntry(entry);
}

void Gpu3D::writeMtxScale(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x1B, value & mask);
    addEntry(entry);
}

void Gpu3D::writeMtxTrans(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x1C, value & mask);
    addEntry(entry);
}

void Gpu3D::writeColor(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x20, value & mask);
    addEntry(entry);
}

void Gpu3D::writeNormal(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x21, value & mask);
    addEntry(entry);
}

void Gpu3D::writeTexCoord(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x22, value & mask);
    addEntry(entry);
}

void Gpu3D::writeVtx16(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x23, value & mask);
    addEntry(entry);
}

void Gpu3D::writeVtx10(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x24, value & mask);
    addEntry(entry);
}

void Gpu3D::writeVtxXY(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x25, value & mask);
    addEntry(entry);
}

void Gpu3D::writeVtxXZ(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x26, value & mask);
    addEntry(entry);
}

void Gpu3D::writeVtxYZ(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x27, value & mask);
    addEntry(entry);
}

void Gpu3D::writeVtxDiff(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x28, value & mask);
    addEntry(entry);
}

void Gpu3D::writePolygonAttr(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x29, value & mask);
    addEntry(entry);
}

void Gpu3D::writeTexImageParam(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x2A, value & mask);
    addEntry(entry);
}

void Gpu3D::writePlttBase(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x2B, value & mask);
    addEntry(entry);
}

void Gpu3D::writeDifAmb(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x30, value & mask);
    addEntry(entry);
}

void Gpu3D::writeSpeEmi(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x31, value & mask);
    addEntry(entry);
}

void Gpu3D::writeLightVector(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x32, value & mask);
    addEntry(entry);
}

void Gpu3D::writeLightColor(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x33, value & mask);
    addEntry(entry);
}

void Gpu3D::writeShininess(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x34, value & mask);
    addEntry(entry);
}

void Gpu3D::writeBeginVtxs(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x40, value & mask);
    addEntry(entry);
}

void Gpu3D::writeEndVtxs(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x41, value & mask);
    addEntry(entry);
}

void Gpu3D::writeSwapBuffers(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x50, value & mask);
    addEntry(entry);
}

void Gpu3D::writeViewport(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x60, value & mask);
    addEntry(entry);
}

void Gpu3D::writeBoxTest(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x70, value & mask);
    addEntry(entry);
}

void Gpu3D::writePosTest(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x71, value & mask);
    addEntry(entry);
}

void Gpu3D::writeVecTest(uint32_t mask, uint32_t value)
{
    // Add an entry to the FIFO
    Entry entry(0x72, value & mask);
    addEntry(entry);
}

void Gpu3D::writeGxStat(uint32_t mask, uint32_t value)
{
    // Clear the error bit and reset the projection stack pointer
    if (value & BIT(15))
    {
        gxStat &= ~0x0000A000;
        projectionPtr = 0;
    }

    // Write to the GXSTAT register
    mask &= 0xC0000000;
    gxStat = (gxStat & ~mask) | (value & mask);
}

uint32_t Gpu3D::readRamCount()
{
    // Read from the RAM_COUNT register
    return (vertexCountIn << 16) | polygonCountIn;
}

uint32_t Gpu3D::readClipMtxResult(int index)
{
    // Update the clip matrix if necessary
    if (clipDirty)
    {
        clip = multiply(&coordinate, &projection);
        clipDirty = false;
    }

    // Read from one of the CLIPMTX_RESULT registers
    return clip.data[index];
}

uint32_t Gpu3D::readVecMtxResult(int index)
{
    // Read from one of the VECMTX_RESULT registers
    return direction.data[(index / 3) * 4 + index % 3];
}
