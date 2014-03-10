#include <stdint.h>
#include <assert.h>

#include "compiler.h"
#include "memory.h"

MemoryReference *makeShaderCode(AllocatorBase *allocator) {
	MemoryReference *shaderCode = allocator->Allocate(0x50);
	uint8_t *shadervirt = (uint8_t*)shaderCode->mmap();
	uint8_t *p = shadervirt;
	addword(&p, 0x958e0dbf);
	addword(&p, 0xd1724823); /* mov r0, vary; mov r3.8d, 1.0 */
	addword(&p, 0x818e7176);
	addword(&p, 0x40024821); /* fadd r0, r0, r5; mov r1, vary */
	addword(&p, 0x818e7376);
	addword(&p, 0x10024862); /* fadd r1, r1, r5; mov r2, vary */
	addword(&p, 0x819e7540);
	addword(&p, 0x114248a3); /* fadd r2, r2, r5; mov r3.8a, r0 */
	addword(&p, 0x809e7009);
	addword(&p, 0x115049e3); /* nop; mov r3.8b, r1 */
	addword(&p, 0x809e7012);
	addword(&p, 0x116049e3); /* nop; mov r3.8c, r2 */
	addword(&p, 0x159e76c0);
	addword(&p, 0x30020ba7); /* mov tlbc, r3; nop; thrend */
	addword(&p, 0x009e7000);
	addword(&p, 0x100009e7); /* nop; nop; nop */
	addword(&p, 0x009e7000);
	addword(&p, 0x500009e7); /* nop; nop; sbdone */
	assert((p - shadervirt) < 0x50);
	shaderCode->munmap();
	return shaderCode;
}
MemoryReference *makeVertextData(AllocatorBase *allocator,int redx, int redy) {
	MemoryReference *vertexData = allocator->Allocate(0x60);
	uint8_t *vertexvirt = (uint8_t*)vertexData->mmap();
	uint8_t *p = vertexvirt;

	// Vertex: Top, red
	addshort(&p, redx << 4); // X in 12.4 fixed point
	addshort(&p, redy << 4); // Y in 12.4 fixed point
	addfloat(&p, 1.0f); // Z
	addfloat(&p, 1.0f); // 1/W
	addfloat(&p, 1.0f); // Varying 0 (Red)
	addfloat(&p, 0.0f); // Varying 1 (Green)
	addfloat(&p, 0.0f); // Varying 2 (Blue)

	// Vertex: bottom left, Green
	addshort(&p, 560 << 4); // X in 12.4 fixed point
	addshort(&p, 800 << 4); // Y in 12.4 fixed point
	addfloat(&p, 1.0f); // Z
	addfloat(&p, 1.0f); // 1/W
	addfloat(&p, 0.0f); // Varying 0 (Red)
	addfloat(&p, 1.0f); // Varying 1 (Green)
	addfloat(&p, 0.0f); // Varying 2 (Blue)

	// Vertex: bottom right, Blue
	addshort(&p, 1360 << 4); // X in 12.4 fixed point
	addshort(&p, 800 << 4); // Y in 12.4 fixed point
	addfloat(&p, 1.0f); // Z
	addfloat(&p, 1.0f); // 1/W
	addfloat(&p, 0.0f); // Varying 0 (Red)
	addfloat(&p, 0.0f); // Varying 1 (Green)
	addfloat(&p, 1.0f); // Varying 2 (Blue)

	assert((p - vertexvirt) < 0x60);
	vertexData->munmap();
	return vertexData;
}

MemoryReference *makeShaderRecord(AllocatorBase *allocator,uint32_t shaderCode,uint32_t shaderUniforms, uint32_t vertextData) {
	MemoryReference *shader = allocator->Allocate(0x10);
	uint8_t *shadervirt = (uint8_t*)shader->mmap();
	uint8_t *p = shadervirt;
	addbyte(&p, 0x01); // flags
	addbyte(&p, 6*4); // stride
	addbyte(&p, 0xcc); // num uniforms (not used)
	addbyte(&p, 3); // num varyings
	addword(&p, shaderCode); // Fragment shader code
	addword(&p, shaderUniforms); // Fragment shader uniforms
	addword(&p, vertextData); // Vertex Data
	assert((p - shadervirt) < 0x10);
	shader->munmap();
	return shader;
}
uint8_t *makeBinner(uint32_t tileAllocationAddress,uint32_t tileAllocationSize, uint32_t tileState, uint32_t shaderRecordAddress, uint32_t primitiveIndexAddress) {
	uint8_t *binner = new uint8_t[0x80];
	uint8_t *p = binner;
	// Configuration stuff
	// Tile Binning Configuration.
	//   Tile state data is 48 bytes per tile, I think it can be thrown away
	//   as soon as binning is finished.
	addbyte(&p, 112);
	addword(&p, tileAllocationAddress); // tile allocation memory address
	addword(&p, tileAllocationSize); // tile allocation memory size
	addword(&p, tileState); // Tile state data address
	addbyte(&p, 30); // 1920/64
	addbyte(&p, 17); // 1080/64 (16.875)
	addbyte(&p, 0x04); // config

	// Start tile binning.
	addbyte(&p, 6);

	// Primitive type
	addbyte(&p, 56);
	addbyte(&p, 0x32); // 16 bit triangle

	// Clip Window
	addbyte(&p, 102);
	addshort(&p, 0);
	addshort(&p, 0);
	addshort(&p, 1920); // width
	addshort(&p, 1080); // height

	// State
	addbyte(&p, 96);
	addbyte(&p, 0x03); // enable both foward and back facing polygons
	addbyte(&p, 0x00); // depth testing disabled
	addbyte(&p, 0x02); // enable early depth write

	// Viewport offset
	addbyte(&p, 103);
	addshort(&p, 0);
	addshort(&p, 0);

	// The triangle
	// No Vertex Shader state (takes pre-transformed vertexes, 
	// so we don't have to supply a working coordinate shader to test the binner.
	addbyte(&p, 65);
	addword(&p, shaderRecordAddress); // Shader Record

	// primitive index list
	addbyte(&p, 32);
	addbyte(&p, 0x04); // 8bit index, triangles
	addword(&p, 3); // Length
	addword(&p, primitiveIndexAddress); // address
	addword(&p, 2); // Maximum index

	// End of bin list
	// Flush
	addbyte(&p, 5);
	// Nop
	addbyte(&p, 1);
	// Halt
	addbyte(&p, 0);

	int length = p - binner;
	assert(length < 0x80);
	return binner;
}
MemoryReference *makePrimitiveList(AllocatorBase *allocator) {
	MemoryReference *list = allocator->Allocate(3);
	uint8_t *p = (uint8_t*)list->mmap();
	p[0] = 0;
	p[1] = 1;
	p[2] = 2;
	list->munmap();
	return list;
}
void testTriangle(AllocatorBase *allocator, int redx,int redy) {
	int width = 1920;
	int height = 1080;
	int tilewidth = 30; // 1920/64
	int tileheight = 17; // 1080/64 (16.875)
	MemoryReference *tileAllocation = allocator->Allocate(0x8000);
	MemoryReference *tileState = allocator->Allocate(48 * tilewidth * tileheight);
	MemoryReference *shaderCode = makeShaderCode(allocator);
	MemoryReference *shaderUniforms = allocator->Allocate(0x10);
	MemoryReference *vertexData = makeVertextData(allocator,redx,redy);
	MemoryReference *shaderRecord = makeShaderRecord(allocator,shaderCode->getBusAddress(),shaderUniforms->getBusAddress(),vertexData->getBusAddress());
	MemoryReference *primitiveList = makePrimitiveList(allocator);
	uint8_t *binner = makeBinner(tileAllocation->getBusAddress(),0x8000,
			tileState->getBusAddress(),
			shaderRecord->getBusAddress(),primitiveList->getBusAddress());
}