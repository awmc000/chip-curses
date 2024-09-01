#ifndef CHIP8_HPP
#define CHIP8_HPP

#define CHIP8_SCREEN_WIDTH 64
#define CHIP8_SCREEN_HEIGHT 32
#define CHIP8_RAM_BYTES 4096
#define CHIP8_VARIABLE_REGISTERS 16
#define CHIP8_STACK_HEIGHT 16
#define CHIP8_ROM_BYTES 3584

// 16 bit type
typedef unsigned short word;

// 8 bit type
typedef unsigned char byte;

word combine(byte leftByte, byte rightByte);

class Chip8 {
public:
    
    // Blocks/containers
    
    byte ram[CHIP8_RAM_BYTES];
    byte variableRegisters[CHIP8_VARIABLE_REGISTERS];
    word stack[CHIP8_STACK_HEIGHT];
    
    // Words
    
    byte stackPointer;
    word programCounter;
    word indexRegister;
    
    // Bytes
    
    byte delayTimer;
    byte soundTimer;
        
    // Display buffer: displayBuffer[y][x] = 0x01, on / 0x00, off
    byte displayBuffer[CHIP8_SCREEN_HEIGHT][CHIP8_SCREEN_WIDTH];

    bool draw;
    bool sound;
    bool copyBeforeShifting;
    bool blockingForKey;

    byte keyState[16];
    byte lastKey;
    bool lastKeyFromBlock;

    void execute(word opcode);

    void executeKeyInstruction(word opcode, byte X);

    void executeMiscInstruction(word opcode, byte X);

    void executeClearReturn(word opcode);

    void executeLogicMathInstruction(word opcode, byte X, byte Y);

    // 00E0: Clear screen
    void opClear();

    // 1NNN: Jump
    void opJump(word NNN);

    // 6XNN: Set register VX to value NN
    void opSetRegister(byte X, word NN);
    
    // 7XNN: Add
    void opAdd(byte X, word NN);

    // ANNN: Set index
    void opSetIndex(word NNN);
    
    // DYXN: Draw
    void opDraw(byte X, byte Y, byte N);

    // 2NNN: Call subroutine
    void opCall(word NNN);

    // 00EE: Return from subroutine
    void opReturn();

    // 3XNN: Skip if VX == NN
    void opSkipByteEqual(byte X, byte NN);
    
    // 4XNN: Skip if VX != NN
    void opSkipByteUnequal(byte X, byte NN);
    
    // 5XY0: Skip if VX == VY
    void opSkipRegEqual(byte X, byte Y);
    
    // 9XY0: Skip if VX != VY
    void opSkipRegUnequal(byte X, byte Y);
    
    // 8XY0: Set VX to VY
    void opCopyRegister(byte X, byte Y);

    // 8XY1: Set VX to VX | VY
    void opOr(byte X, byte Y);
    
    // 8XY2: Set VX to VX & VY
    void opAnd(byte X, byte Y);

    // 8XY3: Set VX to VX ^ VY
    void opXor(byte X, byte Y);

    // 8XY4: Set VX to VX + VY
    void opAddReg(byte X, byte Y);

    // 8XY5: Set VX to VX - VY
    void opSubLR(byte X, byte Y);

    // 8XY7: Set VX to VY - VX
    void opSubRL(byte X, byte Y);

    // 8XYE: Left shift
    void opLeftShift(byte X, byte Y);
    
    // 8XY6: Right shift
    void opRightShift(byte X, byte Y);

    // CXNN: Random number & NN
    void opRandom(byte X, byte NN);

    // EX9E: Skip if key X is pressed
    void opSkipKeyDown(byte X);
    
    // EXA1: Skip if key X is not pressed
    void opSkipKeyNotDown(byte X);

    // FX07: Set VX to DT
    void opDelayToReg(byte X);

    // FX15: Set DT to VX
    void opSetDelayTimer(byte X);

    // FX18: Set ST to VX
    void opSetSoundTimer(byte X);

    // FX1E: Add VX to I
    void opAddRegToIndex(byte X);

    // FX0A: All execution stops until a key K is pressed, VX is set to K.
    void opGetKey(byte X);

    // FX29: Set I to the hex char in VX
    void opFontChar(byte X);

    // FX33: Store the decimal digits of VX in I, I+1, and I+2
    void opBinaryCodedDecimal(byte X);

    // FX55: Store registers 0 to X to memory at I.
    void opRegistersToRam(byte X);

    // FX65: Load registers 0 to X from memory at I.
    void opRamToRegisters(byte X);

    Chip8();
    void cycle();
    void reset();
    void load(byte * rom);
    void dumpState();
    void dumpDisplay();
};

#endif // CHIP8_HPP