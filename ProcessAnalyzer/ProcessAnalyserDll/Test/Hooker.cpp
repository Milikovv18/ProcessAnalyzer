#include "Hooker.h"


uint32_t Hooker::buildTrampoline(void* startAddr, const void* FUNC2HOOK)
{
    Instructs stolenInstrs = stealBytes(FUNC2HOOK);

    uint8_t* stolenByteMem = (uint8_t*)startAddr;
    uint8_t* jumpBackMem = stolenByteMem + stolenInstrs.numBytes;
    uint8_t* absTableMem = jumpBackMem + 14/*is the size of a push/mov/ret instruction pair*/;

    for (uint32_t i = 0; i < stolenInstrs.numInstructions; ++i)
    {
        cs_insn& inst = stolenInstrs.instructions[i];
        if (inst.id >= X86_INS_LOOP && inst.id <= X86_INS_LOOPNE)
        {
            throw "Fuck loops";
        }

        if (isRIPRelativeInstr(inst))
        {
            relocateInstruction(&inst, stolenByteMem);
        }
        else if (isRelativeJump(inst))
        {
            uint32_t aitSize = addJmpToAbsTable(absTableMem, inst);
            rewriteJumpInstruction(&inst, stolenByteMem, absTableMem);
            absTableMem += aitSize;
        }
        else if (inst.id == X86_INS_CALL)
        {
            uint32_t aitSize = addCallToAbsTable(absTableMem, inst, jumpBackMem);
            rewriteCallInstruction(&inst, stolenByteMem, absTableMem);
            absTableMem += aitSize;
        }
        memcpy(stolenByteMem, inst.bytes, inst.size);
        stolenByteMem += inst.size;
    }

    // In the end jump right after our injected jmp instruction
    writeAbsoluteJump64(jumpBackMem, (uint8_t*)FUNC2HOOK + 5);
    // C-style
    free(stolenInstrs.instructions);

    return uint32_t(absTableMem - (uint8_t*)startAddr);
}

Instructs Hooker::stealBytes(const void* FROM)
{
    csh handle;
    cs_open(CS_ARCH_X86, CS_MODE_64, &handle);
    // Need details enabled for relocating RIP relative instructions
    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    cs_insn* disassembledInstructions; // Auto allocating
    size_t count = cs_disasm(handle, (uint8_t*)FROM, 20, (uint64_t)FROM, 20, &disassembledInstructions);

    // Get the instructions covered by the first 5 bytes of the original function
    // Bc only 5 bytes are replaced with near jump
    uint32_t byteCount = 0;
    uint32_t stolenInstrCount = 0;
    for (int32_t i = 0; i < count; ++i)
    {
        cs_insn& inst = disassembledInstructions[i];
        byteCount += inst.size;
        stolenInstrCount++;
        if (byteCount >= 5) break;
    }

    // Instructions might be a bit longer than 5 bytes, so we
    // replace stolen instructions in target func with NOPs
    m_totalBytesCount = byteCount;
    m_totalBytesArr = new char[m_totalBytesCount];
    memset(m_totalBytesArr, 0x90, m_totalBytesCount);

    auto ptr = &handle;
    cs_close(ptr);
    return { disassembledInstructions, stolenInstrCount, byteCount };
}

void Hooker::buildRelay(void* startAddr, void* payloadAddr, uint64_t* counter)
{
    // Incrementing function call counter
    uint8_t incrementCounterInstr[] = {
        0x50, // Saving rax in stack
        0x48, 0xA1, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, // mov rax, qword ptr  ; Moving var val into rax
        0x48, 0xFF, 0xC0, // inc rax
        0x48, 0xA3, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, // mov qword ptr, rax  ; Set var val back
        0x58 // Restore rax from stack
    };

    memcpy(&incrementCounterInstr[3], &counter, sizeof(uint64_t*));
    memcpy(&incrementCounterInstr[16], &counter, sizeof(uint64_t*));
    memcpy(startAddr, incrementCounterInstr, sizeof(incrementCounterInstr));

    // Adding 64bit jump to payload
    writeAbsoluteJump64((char*)startAddr + sizeof(incrementCounterInstr), payloadAddr);
}


uint32_t Hooker::addJmpToAbsTable(uint8_t* absTableEntre, cs_insn& jmp)
{
    char* targetAddrStr = jmp.op_str; //where the instruction intended to go
    uint64_t targetAddr = _strtoui64(targetAddrStr, NULL, 0);
    writeAbsoluteJump64(absTableEntre, (void*)targetAddr);
    return 14; // Size of push/mov/ret instructions for absolute jump
}

// TODO remove r10
uint32_t Hooker::addCallToAbsTable(uint8_t* absTableEntre, cs_insn& call, uint8_t* jumpBackToHookedFunc)
{
    char* targetAddrStr = call.op_str; //where the instruction intended to go
    uint64_t targetAddr = _strtoui64(targetAddrStr, NULL, 0);

    uint8_t* dstMem = absTableEntre;

    uint8_t callAsmBytes[] =
    {
        0x49, 0xBA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, //movabs 64 bit value into r10
        0x41, 0xFF, 0xD2, //call r10
    };
    memcpy(&callAsmBytes[2], &targetAddr, sizeof(void*));
    memcpy(dstMem, &callAsmBytes, sizeof(callAsmBytes));
    dstMem += sizeof(callAsmBytes);

    //after the call, we need to add a second 2 byte jump, which will jump back to the 
      //final jump of the stolen bytes
    uint8_t jmpBytes[2] = { 0xEB, uint8_t(jumpBackToHookedFunc - (absTableEntre + sizeof(jmpBytes))) };
    memcpy(dstMem, jmpBytes, sizeof(jmpBytes));

    return sizeof(callAsmBytes) + sizeof(jmpBytes); // = 15
}


void* Hooker::allocatePageNearAddress(const void* TARGET_ADDR)
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    const uint64_t PAGE_SIZE = sysInfo.dwPageSize;

    // Round down to nearest page boundary
    uint64_t startAddr = (uint64_t(TARGET_ADDR) & ~(PAGE_SIZE - 1));
    // Max near jump is 0x7FFFFF00 in both sides
    uint64_t minAddr = min(startAddr - 0x7FFFFF00, (uint64_t)sysInfo.lpMinimumApplicationAddress);
    uint64_t maxAddr = max(startAddr + 0x7FFFFF00, (uint64_t)sysInfo.lpMaximumApplicationAddress);

    uint64_t startPage = (startAddr - (startAddr % PAGE_SIZE));

    // Iterating through all near pages in hope to find a free one
    uint64_t pageOffset = 1;
    while (1)
    {
        uint64_t byteOffset = pageOffset * PAGE_SIZE;
        uint64_t highAddr = startPage + byteOffset;
        uint64_t lowAddr = (startPage > byteOffset) ? startPage - byteOffset : 0;

        bool needsExit = highAddr > maxAddr && lowAddr < minAddr;

        // Page higher than target address
        if (highAddr < maxAddr)
        {
            void* outAddr = VirtualAlloc((void*)highAddr, PAGE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (outAddr)
                return outAddr;
        }

        // Page lower than target address
        if (lowAddr > minAddr)
        {
            void* outAddr = VirtualAlloc((void*)lowAddr, PAGE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (outAddr != nullptr)
                return outAddr;
        }

        pageOffset++;

        if (needsExit)
        {
            break;
        }
    }

    return nullptr;
}

void Hooker::writeAbsoluteJump64(void* startAddr, void* addrToJumpTo)
{
    uint8_t absJumpInstructions[] =
    {
        0x68, 0x00, 0x00, 0x00, 0x00, // push first half of the address
        0xC7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, // mov the second half of addr into stack
        0xC3 // ret (jmp to addr)
    };

    uint64_t addr64 = (uint64_t)addrToJumpTo;
    memcpy(&absJumpInstructions[1], &addr64, sizeof(addr64) / 2);
    memcpy(&absJumpInstructions[9], (char*)&addr64 + sizeof(addr64) / 2, sizeof(addr64) / 2);
    memcpy(startAddr, absJumpInstructions, sizeof(absJumpInstructions));
}

template<class T> T Hooker::getDisplacement(cs_insn* inst, uint8_t offset)
{
    T disp;
    memcpy(&disp, &inst->bytes[offset], sizeof(T));
    return disp;
}

void Hooker::relocateInstruction(cs_insn* inst, void* dstLocation)
{
    cs_x86* x86 = &(inst->detail->x86);
    uint8_t offset = x86->encoding.disp_offset;

    switch (x86->encoding.disp_size)
    {
    case 1:
    {
        int8_t disp = getDisplacement<uint8_t>(inst, offset);
        disp -= int8_t(uint64_t(dstLocation) - inst->address);
        memcpy(&inst->bytes[offset], &disp, 1);
    }break;

    case 2:
    {
        int16_t disp = getDisplacement<uint16_t>(inst, offset);
        disp -= int16_t(uint64_t(dstLocation) - inst->address);
        memcpy(&inst->bytes[offset], &disp, 2);
    }break;

    case 4:
    {
        int32_t disp = getDisplacement<int32_t>(inst, offset);
        disp -= int32_t(uint64_t(dstLocation) - inst->address);
        memcpy(&inst->bytes[offset], &disp, 4);
    }break;
    }
}

void Hooker::rewriteJumpInstruction(cs_insn* instr, uint8_t* instrPtr, uint8_t* absTableEntry)
{
    uint8_t distToJumpTable = uint8_t(absTableEntry - (instrPtr + instr->size));

    // jmp instructions can have a 1 or 2 byte opcode, and need a 1-4 byte operand
    // rewrite the operand for the jump to go to the jump table
    uint8_t instrByteSize = instr->bytes[0] == 0x0F ? 2 : 1;
    uint8_t operandSize = instr->size - instrByteSize;

    switch (operandSize)
    {
    case 1: instr->bytes[instrByteSize] = distToJumpTable; break;
    case 2: {uint16_t dist16 = distToJumpTable; memcpy(&instr->bytes[instrByteSize], &dist16, 2); } break;
    case 4: {uint32_t dist32 = distToJumpTable; memcpy(&instr->bytes[instrByteSize], &dist32, 4); } break;
    }
}

void Hooker::rewriteCallInstruction(cs_insn* instr, uint8_t* instrPtr, uint8_t* absTableEntry)
{
    uint8_t distToJumpTable = uint8_t(absTableEntry - (instrPtr + instr->size));

    //calls need to be rewritten as relative jumps to the abs table
    //but we want to preserve the length of the instruction, so pad with NOPs
    uint8_t jmpBytes[2] = { 0xEB, distToJumpTable };
    memset(instr->bytes, 0x90, instr->size);
    memcpy(instr->bytes, jmpBytes, sizeof(jmpBytes));
}


bool Hooker::isRelativeJump(cs_insn& inst)
{
    bool isAnyJumpInstruction = inst.id >= X86_INS_JAE && inst.id <= X86_INS_JS;
    bool isJmp = inst.id == X86_INS_JMP;
    bool startsWithEBorE9 = inst.bytes[0] == 0xEB || inst.bytes[0] == 0xE9;
    return isJmp ? startsWithEBorE9 : isAnyJumpInstruction;
}

bool Hooker::isRelativeCall(cs_insn& inst)
{
    bool isCall = inst.id == X86_INS_CALL;
    bool startsWithE8 = inst.bytes[0] == 0xE8;
    return isCall && startsWithE8;
}

bool Hooker::isRIPRelativeInstr(cs_insn& inst)
{
    cs_x86* x86 = &(inst.detail->x86);

    for (uint32_t i = 0; i < inst.detail->x86.op_count; i++)
    {
        cs_x86_op* op = &(x86->operands[i]);

        // Mem type is rip relative, like lea rcx,[rip+0xbeef]
        if (op->type == X86_OP_MEM)
        {
            // If we're relative to rip
            return op->mem.base == X86_REG_RIP;
        }
    }

    return false;
}


void Hooker::installHook(void* func2hook, void* payloadFunc, void** trampolinePtr, uint64_t* counter)
{
    DWORD oldProtect;
    VirtualProtect(func2hook, 1024, PAGE_EXECUTE_READWRITE, &oldProtect);

    void* hookMemory = allocatePageNearAddress(func2hook);
    if (!hookMemory)
        throw "Fuck your near memory";

    uint32_t trampolineSize = buildTrampoline(hookMemory, func2hook);
    *trampolinePtr = hookMemory;

    // Create the relay function
    void* relayFuncMemory = (char*)hookMemory + trampolineSize;
    buildRelay(relayFuncMemory, payloadFunc, counter); // TODO

    // Install the hook (main part)
    uint8_t jmpInstruction[5] = { 0xE9, 0x0, 0x0, 0x0, 0x0 };
    const int64_t relAddr = (int64_t)relayFuncMemory - ((int64_t)func2hook + sizeof(jmpInstruction));
    memcpy(jmpInstruction + 1, &relAddr, 4);
    memcpy(m_totalBytesArr, jmpInstruction, sizeof(jmpInstruction));

    // Finally substituting actual function bytes
    memcpy(func2hook, m_totalBytesArr, m_totalBytesCount);
    delete[] m_totalBytesArr;
}