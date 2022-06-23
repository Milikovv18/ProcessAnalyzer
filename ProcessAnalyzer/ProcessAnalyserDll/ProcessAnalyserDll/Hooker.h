#pragma once

#include <Windows.h>
#include <iostream>

#include "Capstone/include/capstone/capstone.h"


struct Instructs
{
	cs_insn* instructions;
	uint32_t numInstructions;
	uint32_t numBytes;
};


class Hooker
{
public:
	void previewHook(void* func2hook, uint8_t** previewBytes, uint8_t** previewText);
	void installHook(void* func2hook, void* payloadFunc, void** trampolinePtr, uint64_t* counter);
	void uninstallHook(void* func2hook, void** trampolinePtr, uint8_t* rsrvBytes, const int& byteLen);

private:
	char* m_totalBytesArr;
	uint64_t m_totalBytesCount;

	// Main actions
	uint32_t buildTrampoline(void* startAddr, const void* FUNC2HOOK);
	Instructs stealBytes(const void* FROM);
	void buildRelay(void* startAddress, void* payload, uint64_t* counter);

	// Abs table
	uint32_t addJmpToAbsTable(uint8_t* absTableEntre, cs_insn& jmp);
	uint32_t addCallToAbsTable(uint8_t* absTableEntre, cs_insn& call, uint8_t* jumpBackToHookedFunc);

	// Helpers
	void* allocatePageNearAddress(const void* targetAddr);
	void writeAbsoluteJump64(void* startAddr, void* addrToJumpTo);
	template<class T> T getDisplacement(cs_insn* inst, uint8_t offset);
	void relocateInstruction(cs_insn* inst, void* dstLocation);
	void rewriteJumpInstruction(cs_insn* instr, uint8_t* instrPtr, uint8_t* absTableEntry);
	void rewriteCallInstruction(cs_insn* instr, uint8_t* instrPtr, uint8_t* absTableEntry);

	// Asm getters
	bool isRelativeJump(cs_insn& inst);
	bool isRelativeCall(cs_insn& inst);
	bool isRIPRelativeInstr(cs_insn& inst);
};

