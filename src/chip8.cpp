#include "chip8.hpp"
#include <iostream>
#include <iomanip>
#include <cstdlib>

word combine(byte leftByte, byte rightByte) {
    return ((leftByte << 8) | rightByte);
}

Chip8::Chip8() {
    // Load user settings
    copyBeforeShifting = false;

    // Seed random number generator
    srand(time(nullptr));

    reset();    
}

void Chip8::execute(word opcode) {

    byte X = (opcode & 0x0F00) >> 8;    // nib 2
    byte Y = (opcode & 0x00F0) >> 4;    // nib 3
    byte N = (opcode & 0x000F);         // nib 4
    word NN = opcode & 0x00FF;          // nib 3, 4
    word NNN = opcode & 0x0FFF;         // nib 2, 3, 4

    switch (opcode & 0xF000) {
        case 0x0000:    executeClearReturn(opcode);                 break;
        case 0x1000:    opJump(NNN);                                break;
        case 0x2000:    opCall(NNN);                                break;
        case 0x3000:    opSkipByteEqual(X, NN);                     break;
        case 0x4000:    opSkipByteUnequal(X, NN);                   break;
        case 0x5000:    opSkipRegEqual(X, Y);                       break;
        case 0x6000:    opSetRegister(X, NN);                       break;
        case 0x7000:    opAdd(X, NN);                               break;
        case 0x8000:    executeLogicMathInstruction(opcode, X, Y);  break;
        case 0x9000:    opSkipRegUnequal(X, Y);                     break;
        case 0xA000:    opSetIndex(NNN);                            break;
        case 0xC000:    opRandom(X, NN);                            break;
        case 0xD000:    opDraw(X, Y, N);                            break;
        case 0xE000:    executeKeyInstruction(opcode, X);           break;
        case 0xF000:    executeMiscInstruction(opcode, X);          break;
        default:
            std::cerr << "Unsupported instruction: " << std::hex << opcode << std::endl;
            exit(1);
            break;
    }
}

void Chip8::executeKeyInstruction(word opcode, byte X)
{
    switch (opcode & 0x000F) {
        case 0x000E:    opSkipKeyDown(X);       break;
        case 0x0001:    opSkipKeyNotDown(X);    break;
    }
}

void Chip8::executeMiscInstruction(word opcode, byte X)
{
    switch (opcode & 0x00FF) {
        case 0x0029:    opFontChar(X);              break;
        case 0x0033:    opBinaryCodedDecimal(X);    break;
        case 0x0007:    opDelayToReg(X);            break;
        case 0x0015:    opSetDelayTimer(X);         break;
        case 0x0018:    opSetSoundTimer(X);         break;
        case 0x000A:    opGetKey(X);                break;
        case 0x001E:    opAddRegToIndex(X);         break;
        case 0x0055:    opRegistersToRam(X);        break;
        case 0x0065:    opRamToRegisters(X);        break;
    }
}

void Chip8::executeClearReturn(word opcode)
{
    switch (opcode & 0x000F) {
        case 0x0000:    opClear();      break;
        case 0x000E:    opReturn();     break;
    }
}

void Chip8::executeLogicMathInstruction(word opcode, byte X, byte Y) {
    switch (opcode & 0x000F) {
        case 0x0000:    opCopyRegister(X, Y);   break;
        case 0x0001:    opOr(X, Y);             break;
        case 0x0002:    opAnd(X, Y);            break;
        case 0x0003:    opXor(X, Y);            break;
        case 0x0004:    opAddReg(X, Y);         break;
        case 0x0005:    opSubLR(X, Y);          break;
        case 0x0006:    opRightShift(X, Y);     break;
        case 0x0007:    opSubRL(X, Y);          break;
        case 0x000E:    opLeftShift(X, Y);      break;
    }
}

void Chip8::opClear() {
    for (int y = 0; y < CHIP8_SCREEN_HEIGHT; y++) {
        for (int x = 0; x < CHIP8_SCREEN_WIDTH; x++) {
            displayBuffer[y][x] = 0x00;
        }
    }
}

void Chip8::opJump(word NNN) {
    programCounter = NNN;
}

void Chip8::opSetRegister(byte X, word NN)
{
    variableRegisters[X] = NN;
}

void Chip8::opAdd(byte X, word NN)
{
    variableRegisters[X] += NN;
}

void Chip8::opSetIndex(word NNN)
{
    indexRegister = NNN;
}

void Chip8::opDraw(byte X, byte Y, byte N)
{
    byte xCoord = variableRegisters[X] % CHIP8_SCREEN_WIDTH;
    byte startX = xCoord;
    byte yCoord = variableRegisters[Y] % CHIP8_SCREEN_HEIGHT;

    // Turn off draw flag
    draw = false;

    // Turn off VF
    variableRegisters[0xF] = 0;

    // Draw bytes I up to I+N 8px wide
    for (int y = 0; y < N; y++) {
        // Get the row of the sprite
        byte spriteRow = ram[indexRegister + y];
        
        // For bits in the byte, from MSB to LSB
        for (int x = 7; x >= 0; x--) {
            bool spriteBitSet = (spriteRow >> x) & 1;
            bool bufferBitSet = (displayBuffer[yCoord][xCoord] & 1);


            if (spriteBitSet && bufferBitSet) {
                displayBuffer[yCoord][xCoord] = 0x00;
                variableRegisters[0x0F] = 1;
                draw = true;
            } else if (spriteBitSet) {
                displayBuffer[yCoord][xCoord] = 0x01;
                draw = true;
            }

            xCoord++;
    
            if (xCoord >= CHIP8_SCREEN_WIDTH) {
                break;
            }
        }
        xCoord = startX;
        yCoord++;

        if (yCoord >= CHIP8_SCREEN_HEIGHT) {
            break;
        }
    }
}

void Chip8::opCall(word NNN) {
    stack[stackPointer++] = programCounter;
	programCounter = NNN;
}

void Chip8::opReturn() {
    programCounter = stack[--stackPointer];
}

void Chip8::opSkipByteEqual(byte X, byte NN) {
    if (variableRegisters[X] == NN) {
        programCounter += 2;
    }
}

void Chip8::opSkipByteUnequal(byte X, byte NN) {
    if (variableRegisters[X] != NN) {
        programCounter += 2;
    }
}

void Chip8::opSkipRegEqual(byte X, byte Y) {
    if (variableRegisters[X] == variableRegisters[Y]) {
        programCounter += 2;
    }
}

void Chip8::opSkipRegUnequal(byte X, byte Y) {
    if (variableRegisters[X] != variableRegisters[Y]) {
        programCounter += 2;
    }
}

void Chip8::opCopyRegister(byte X, byte Y) {
    variableRegisters[X] = variableRegisters[Y];
}

void Chip8::opOr(byte X, byte Y) {
    variableRegisters[X] = variableRegisters[X] | variableRegisters[Y];
}

void Chip8::opAnd(byte X, byte Y) {
    variableRegisters[X] = variableRegisters[X] & variableRegisters[Y];
}

void Chip8::opXor(byte X, byte Y) {
    variableRegisters[X] = variableRegisters[X] ^ variableRegisters[Y];
}

void Chip8::opAddReg(byte X, byte Y) {
    if (((int) variableRegisters[X] + (int) variableRegisters[Y]) > 0xFF) {
        variableRegisters[0xF] = 1;
    } else {
        variableRegisters[0xF] = 0;
    }
    variableRegisters[X] += variableRegisters[Y];
}

void Chip8::opSubLR(byte X, byte Y) {
    if (variableRegisters[X] >= variableRegisters[Y]) {
        variableRegisters[0xF] = 1;
    } else {
        variableRegisters[0xF] = 0;
    }

    variableRegisters[X] -= variableRegisters[Y];
}

void Chip8::opSubRL(byte X, byte Y) {
    if (variableRegisters[Y] >= variableRegisters[X]) {
        variableRegisters[0xF] = 1;
    } else {
        variableRegisters[0xF] = 0;
    }

    variableRegisters[X] = variableRegisters[Y] - variableRegisters[X];
}

void Chip8::opLeftShift(byte X, byte Y) {
    if (copyBeforeShifting) {
        variableRegisters[X] = variableRegisters[Y];
    }
    variableRegisters[0xF] = (variableRegisters[X] & 0x80) >> 7;
    variableRegisters[X] = variableRegisters[X] << 1;
}

void Chip8::opRightShift(byte X, byte Y) {
    if (copyBeforeShifting) {
        variableRegisters[X] = variableRegisters[Y];
    }
    variableRegisters[0xF] = variableRegisters[X] & 0x01;
    variableRegisters[X] = variableRegisters[X] >> 1;

}

void Chip8::opRandom(byte X, byte NN) {
    variableRegisters[X] = rand() & NN;
}

void Chip8::opSkipKeyDown(byte X) {
    byte state = keyState[variableRegisters[X]];
    
    if (state > 0xF)
        exit(1);
    
    if (state == 1) {
        programCounter += 2;
    }
}

void Chip8::opSkipKeyNotDown(byte X) {
    byte state = keyState[variableRegisters[X]];
    
    if (state > 0xF)
        exit(1);
    
    if (state == 0) {
        programCounter += 2;
    }
}

void Chip8::opDelayToReg(byte X) {
    variableRegisters[X] = delayTimer;
}

void Chip8::opSetDelayTimer(byte X) {
    delayTimer = X;
}

void Chip8::opSetSoundTimer(byte X) {
    soundTimer = X;
}

void Chip8::opAddRegToIndex(byte X) {
    if ((int) indexRegister + (int) X > 0x1000) {
        variableRegisters[0xF] = 1;
    }
    indexRegister += variableRegisters[X];
}

void Chip8::opGetKey(byte X) {
    
    // If not blocking, zero out the key state and set to blocking
    if (!blockingForKey) {
        blockingForKey = true;
    }

    // If the last key was fetched during a block and has been released, get it & return
    if (lastKeyFromBlock && (keyState[lastKey] == 0)) {
        variableRegisters[X] = lastKey;
        blockingForKey = false;
        return;
    }

    // No key yet: If no key is pressed, decrement PC
    programCounter -= 2;
}

void Chip8::opFontChar(byte X) {
    indexRegister = ram[0x050 + (5 * X)];
}

void Chip8::opBinaryCodedDecimal(byte X) {
    ram[indexRegister] = variableRegisters[X] / 100;
    ram[indexRegister + 1] = (variableRegisters[X] / 10) % 10;
    ram[indexRegister + 2] = variableRegisters[X] % 10;
}

void Chip8::opRegistersToRam(byte X) {
    for (int i = 0; i <= X; i++) {
        ram[indexRegister + i] = variableRegisters[i];   
    }
}

void Chip8::opRamToRegisters(byte X) {
    for (int i = 0; i <= X; i++) {
        variableRegisters[i] = ram[indexRegister + i];
    }
}

void Chip8::cycle() {
    // Fetch
    word opcode = combine(ram[programCounter], ram[programCounter + 1]);
    programCounter += 2;
    
    // Decode, Execute
    execute(opcode);
    
    // Update timers
    if (delayTimer > 0) {
        delayTimer--;
    }
    
    if (soundTimer > 0) {
        sound = true;
        soundTimer--;
    } else {
        sound = false;
    }
}

void Chip8::reset() {
    // Clear display buffer
    opClear();
    
    // Clear RAM
    for (int i = 0; i < CHIP8_RAM_BYTES; i++) {
        ram[i] = 0x00;
    }

    // Clear registers
    for (int i = 0; i < CHIP8_VARIABLE_REGISTERS; i++) {
        variableRegisters[i] = 0x0000;
    }
    
    // Clear stack
    for (int i = 0; i < CHIP8_STACK_HEIGHT; i++) {
        stack[i] = 0x0000;
    }

    // Clear key buffer
    for (int i = 0; i < 0xF; i++) {
        keyState[i] = 0x00;
    }

    // Clear SP, PC, I, DT, ST 
    stackPointer    = 0x00;
    programCounter  = 0x0000;
    indexRegister   = 0x0000;
    delayTimer      = 0x00;
    soundTimer      = 0x00;

    // Load font
    byte font[80] = { 
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    for (int i = 0; i < 80; i++) {
        ram[0x050 + i] = font[i];
    }

    // Set PC to start
    programCounter = 0x200;

    // Set switch for FX0A
    blockingForKey = false;
    lastKeyFromBlock = false;
}

void Chip8::load(byte * rom)
{
    for (int i = 0; i < CHIP8_RAM_BYTES - 512; i++) {
        ram[512 + i] = rom[i];
    }
}

void Chip8::dumpState() {
    std::cerr << "==== CHIP8 =====" << std::endl;
    
    std::cerr << "PC: " << std::hex << (int) programCounter << std::endl;
    std::cerr << "SP: " << std::hex << (int) stackPointer << std::endl;
    std::cerr << "IR: " << std::hex << (int) indexRegister << std::endl;
    
    std::cerr << "== REGISTERS ===" << std::endl;
    for (int i = 0; i < CHIP8_VARIABLE_REGISTERS; i++) {
        std::cerr << "V" << std::hex << i 
        << ": " << std::hex << (int) variableRegisters[i] << std::endl;
    }
    std::cerr << "=== TIMERS =====" << std::endl;
    std::cerr << "DT: " << std::hex << (int) delayTimer << std::endl;
    std::cerr << "ST: " << std::hex << (int) soundTimer << std::endl;

    std::cerr << "===== KEYS =====" << std::endl;
    for (int i = 0; i < 0xF; i++) {
        char key = keyState[i] + '0';
        std::cerr << "KEY " << std::hex << i << ": " << std::hex << key << std::endl; 
    }

    dumpDisplay();

    std::cerr << "===== RAM ======" << std::endl;
    for (int i = 0; i < CHIP8_RAM_BYTES; i++) {
        if ( (i > 0) && (i % 4 == 0))
            std::cerr << std::endl;
        std::cerr << std::hex << i << ": " << std::hex << (int) ram[i] << "\t\t\t";
    }
}

void Chip8::dumpDisplay() {
    std::cerr << "===== DISPLAY ======" << std::endl;
    for (int y = 0; y < CHIP8_SCREEN_HEIGHT; y++) {
        for (int x = 0; x < CHIP8_SCREEN_WIDTH; x++) {
            
            if (displayBuffer[y][x]) {
                std::cerr << "█";
            } else {
                std::cerr << "_";
            }
        }
        std::cerr << "\n";
    }
}