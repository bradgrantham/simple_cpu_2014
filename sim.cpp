#include <cstdlib>
#include <cstdio>
#include <chrono>
#include <boost/program_options.hpp>

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include "simple_cpu_2014.hpp"

const int memsize = 32 * 1024 * 1024;
const int registercount = 8;
const int CONSOLE_OUTPUT = 0xf0000000;

using namespace simple_cpu_2014;

// cpu knows how to decode uint32ts and execute
// cpu fetches memory, knows how to read exceptions

// machine has instruction_bus, memory_bus
// instruction_bus may be separate for harvard architecture
// machine saves up memory changes for eventual read

// who does GL operation for framebuffer?

int32_t sign_extend(uint32_t v, int bits)
{
    bool negative = v & (1 << (bits - 1));
    return negative ? (v | (0xffffffff << bits)) : v;
}

struct cpu
{
    int32_t registers[registercount];
    bool lt, eq, gt, halted;
    int carry;

    void reset()
    {
        for(int i = 0; i < registercount; i++)
            registers[i] = 0 ;
        carry = 0;
        lt = false;
        eq = false;
        gt = false;
        halted = false;
    }
    cpu() { reset(); }
};

struct machine
{
    uint8_t memory[memsize];
    std::set<uint32_t> memory_changed;

    void fetch32(uint32_t addr, uint32_t* data, bool* fault)
    {
        if(addr > memsize - 4) {
            *fault = true;
            *data = 0;
        }

        *fault = false;
        *data =
            (memory[addr + 0] << 0) | 
            (memory[addr + 1] << 8) | 
            (memory[addr + 2] << 16) | 
            (memory[addr + 3] << 24);
    }

    void fetch16(uint32_t addr, uint32_t* data, bool* fault)
    {
        if(addr > memsize - 2) {
            *fault = true;
            *data = 0;
        }

        *fault = false;
        *data =
            (memory[addr + 0] << 0) | 
            (memory[addr + 1] << 8);
    }

    void fetch8(uint32_t addr, uint32_t* data, bool* fault)
    {
        if(addr > memsize - 1) {
            *fault = true;
            *data = 0;
        }

        *fault = false;
        *data = memory[addr + 0];
    }

    void store32(uint32_t addr, uint32_t value, bool *fault)
    {
        if(addr > memsize - 4) {
            *fault = true;
            return;
        }
        *fault = false;

        memory[addr + 0] = (value >> 0) & 0xff;
        memory[addr + 1] = (value >> 8) & 0xff;
        memory[addr + 2] = (value >> 16) & 0xff;
        memory[addr + 3] = (value >> 24) & 0xff;

        memory_changed.insert(addr + 0);
        memory_changed.insert(addr + 1);
        memory_changed.insert(addr + 2);
        memory_changed.insert(addr + 3);
    }

    void store16(uint32_t addr, uint32_t value, bool *fault)
    {
        if(addr > memsize - 2) {
            *fault = true;
            return;
        }
        *fault = false;

        memory[addr + 0] = (value >> 0) & 0xff;
        memory[addr + 1] = (value >> 8) & 0xff;
        memory_changed.insert(addr + 0);
        memory_changed.insert(addr + 1);
    }

    void store8(uint32_t addr, uint32_t value, bool *fault)
    {
        if(addr == CONSOLE_OUTPUT) {
            *fault = false;
            putchar(value & 0xff);
            return;
        }

        if(addr > memsize - 1) {
            *fault = true;
            return;
        }
        *fault = false;

        memory[addr] = value & 0xff;
        memory_changed.insert(addr);
        memory_changed.insert(addr + 1);
    }

    machine() { }
};

struct instruction
{
    uint opcode;
    uint dst;
    uint src;
    uint modifier;
    uint data;
    instruction(uint32_t value);
};

typedef void (*instructionfunc)(cpu& c, machine&, const instruction&);

struct opcode_info {
    int datasize;
    const char *name;
    instructionfunc func;
};

extern opcode_info opcodes[];

void moviu(cpu& c, machine& s, const instruction& instr)
{
    c.registers[instr.dst] = instr.data << 16;
    c.registers[reg::PC] += 4;
}

void addi(cpu& c, machine& s, const instruction& instr)
{
    c.registers[instr.dst] += sign_extend(instr.data, opcodes[instr.opcode].datasize);
    c.registers[reg::PC] += 4;
}

void addiu(cpu& c, machine& s, const instruction& instr)
{
    c.registers[instr.dst] += instr.data;
    c.registers[reg::PC] += 4;
}

void shift(cpu& c, machine& s, const instruction& instr)
{
    if(instr.modifier == shifttype::RL)
        c.registers[instr.dst] >>= (instr.data & 0x1f);
    else if(instr.modifier == shifttype::LL)
        c.registers[instr.dst] <<= (instr.data & 0x1f);
    else if(instr.modifier == shifttype::RA)
        c.registers[instr.dst] = c.registers[instr.dst] >> (instr.data & 0x1f);
    else if(instr.modifier == shifttype::LA)
        c.registers[instr.dst] = c.registers[instr.dst] << (instr.data & 0x1f);

    c.registers[reg::PC] += 4;
}

void cmpiu(cpu& c, machine& s, const instruction& instr)
{
    c.eq = (((uint32_t)c.registers[instr.dst] & 0xffffff) == instr.data);
    c.lt = (((uint32_t)c.registers[instr.dst] & 0xffffff) < instr.data);
    c.gt = (((uint32_t)c.registers[instr.dst] & 0xffffff) > instr.data);

    c.registers[reg::PC] += 4;
}

void store(cpu& c, machine& s, const instruction& instr)
{
    uint32_t addr = c.registers[instr.dst] + sign_extend(instr.data, 18);
    bool fault;
    switch(instr.modifier) {
        case opsize::SIZE_8: s.store8(addr, c.registers[instr.src], &fault); break;
        case opsize::SIZE_16: s.store16(addr, c.registers[instr.src], &fault); break;
        case opsize::SIZE_32: s.store32(addr, c.registers[instr.src], &fault); break;
    }
    if(fault)
        return;
    c.registers[reg::PC] += 4;
}

void load(cpu& c, machine& s, const instruction& instr)
{
    uint32_t addr = c.registers[instr.src] + sign_extend(instr.data, 18);
    uint32_t data = 0xffffffff;
    bool fault;
    switch(instr.modifier) {
        case opsize::SIZE_8: s.fetch8(addr, &data, &fault); break;
        case opsize::SIZE_16: s.fetch16(addr, &data, &fault); break;
        case opsize::SIZE_32: s.fetch32(addr, &data, &fault); break;
    }
    if(fault)
        return;
    c.registers[instr.dst] = data;
    c.registers[reg::PC] += 4;
}

void mov(cpu& c, machine& s, const instruction& instr)
{
    c.registers[instr.dst] = c.registers[instr.src];
    c.registers[reg::PC] += 4;
}

void push(cpu& c, machine& s, const instruction& instr)
{
    bool fault;
    s.store32(c.registers[reg::SP - 4], c.registers[instr.dst], &fault); // dst is first reg
    if(fault)
        return;
    c.registers[reg::SP] -= 4;
    c.registers[reg::PC] += 4;
}

void pop(cpu& c, machine& s, const instruction& instr)
{
    bool fault;
    uint32_t data;
    s.fetch32(c.registers[reg::SP], &data, &fault);
    if(fault)
        return;
    c.registers[instr.dst] = data;
    c.registers[reg::SP] += 4;
    c.registers[reg::PC] += 4;
}

void op_and(cpu& c, machine& s, const instruction& instr)
{
    c.registers[instr.dst] &= c.registers[instr.src];
    c.registers[reg::PC] += 4;
}

void op_or(cpu& c, machine& s, const instruction& instr)
{
    c.registers[instr.dst] |= c.registers[instr.src];
    c.registers[reg::PC] += 4;
}

void op_xor(cpu& c, machine& s, const instruction& instr)
{
    c.registers[instr.dst] ^= c.registers[instr.src];
    c.registers[reg::PC] += 4;
}

void op_not(cpu& c, machine& s, const instruction& instr)
{
    c.registers[instr.dst] = ~c.registers[instr.src];
    c.registers[reg::PC] += 4;
}

void add(cpu& c, machine& s, const instruction& instr)
{
    c.registers[instr.dst] += c.registers[instr.src];
    c.registers[reg::PC] += 4;
    // XXX carry
}

void adc(cpu& c, machine& s, const instruction& instr)
{
    c.registers[instr.dst] += c.registers[instr.src] + c.carry;
    c.registers[reg::PC] += 4;
}

void sub(cpu& c, machine& s, const instruction& instr)
{
    c.registers[instr.dst] -= c.registers[instr.src];
    c.registers[reg::PC] += 4;
}

void mult(cpu& c, machine& s, const instruction& instr)
{
    long long v = c.registers[instr.dst] * c.registers[instr.src];
    c.registers[instr.dst] = v >> 32;
    c.registers[instr.src] = v & 0xffffffff;
    c.registers[reg::PC] += 4;
}

void div(cpu& c, machine& s, const instruction& instr)
{
    int32_t d = c.registers[instr.dst] / c.registers[instr.src];
    int32_t m = c.registers[instr.dst] % c.registers[instr.src];
    c.registers[instr.dst] = d;
    c.registers[instr.src] = m;
    c.registers[reg::PC] += 4;
}

void cmp(cpu& c, machine& s, const instruction& instr)
{
    c.eq = (c.registers[instr.dst] == c.registers[instr.src]);
    c.lt = (c.registers[instr.dst] < c.registers[instr.src]);
    c.gt = (c.registers[instr.dst] > c.registers[instr.src]);
    c.registers[reg::PC] += 4;
}

void xchg(cpu& c, machine& s, const instruction& instr)
{
    int32_t t = c.registers[instr.dst];
    c.registers[instr.dst] = c.registers[instr.src];
    c.registers[instr.src] = t;
    c.registers[reg::PC] += 4;
}

void jne(cpu& c, machine& s, const instruction& instr)
{
    if(c.eq)
        c.registers[reg::PC] += 4;
    else
        c.registers[reg::PC] += sign_extend(instr.data, 27) << 2;
}

void jl(cpu& c, machine& s, const instruction& instr)
{
    if(c.lt)
        c.registers[reg::PC] += sign_extend(instr.data, 27) << 2; // XXX proposed
    else
        c.registers[reg::PC] += 4;
}

void jsr(cpu& c, machine& s, const instruction& instr)
{
    // Rx <= pc, pc <= pc + (sdata24 << 2))
    c.registers[instr.dst] = c.registers[reg::PC] + 4; // XXX proposed
    c.registers[reg::PC] += sign_extend(instr.data, 27) << 2;
}

void jmp(cpu& c, machine& s, const instruction& instr)
{
    c.registers[reg::PC] = sign_extend(instr.data, 27) << 2;
}

void jr(cpu& c, machine& s, const instruction& instr)
{
    c.registers[reg::PC] = c.registers[instr.dst] + (sign_extend(instr.data, 24) << 2);
}

void sys(cpu& c, machine& s, const instruction& instr)
{
    bool fault;
    s.store32(c.registers[reg::SP - 4], c.registers[reg::PC], &fault); // dst is first reg
    if(fault)
        return;
    c.registers[reg::SP] -= 4;
    c.registers[reg::PC] = instr.data << 2;
}

void halt(cpu& c, machine& s, const instruction& instr)
{
    c.halted = true;
}

void swapcc(cpu& c, machine& s, const instruction& instr)
{
    int32_t t = c.registers[instr.dst];

    c.registers[instr.dst] =
        ((c.carry ? 1 : 0) << 3) |
        ((c.lt ? 1 : 0) << 2) |
        ((c.gt ? 1 : 0) << 1) |
        ((c.eq ? 1 : 0) << 0);

    c.carry = (t & 0x8) ? 1 : 0;
    c.lt = t & 0x4;
    c.gt = t & 0x2;
    c.eq = t & 0x1;

    c.registers[reg::PC] += 4;
}

opcode_info opcodes[] =
{
    [opcode::MOVIU] = {24, "moviu", moviu},
    [opcode::ADDI] = {24, "addi", addi},
    [opcode::SHIFT] = {18, "shift", shift},
    [opcode::CMPIU] = {24, "cmpiu", cmpiu},
    [opcode::ADDIU] = {24, "addiu", addiu},

    [opcode::STORE] = {18, "store", store},
    [opcode::LOAD] = {18, "load", load},
    [opcode::MOV] = {18, "mov", mov},
    [opcode::PUSH] = {24, "push", push},
    [opcode::POP] = {24, "pop", pop},

    [opcode::AND] = {18, "and", op_and},
    [opcode::OR] = {18, "or", op_or},
    [opcode::XOR] = {18, "xor", op_xor},
    [opcode::NOT] = {18, "not", op_not},
    [opcode::ADD] = {18, "add", add},
    [opcode::ADC] = {18, "adc", adc},
    [opcode::SUB] = {18, "sub", sub},
    [opcode::MULT] = {18, "mult", mult},
    [opcode::DIV] = {18, "div", div},
    [opcode::CMP] = {18, "cmp", cmp},
    [opcode::XCHG] = {18, "xchg", xchg},

    [opcode::JNE] = {27, "jne", jne},
    [opcode::JL] = {27, "jl", jl},
    [opcode::JSR] = {27, "jsr", jsr},
    [opcode::JMP] = {27, "jmp", jmp},
    [opcode::JR] = {24, "jr", jr},
    [opcode::SWAPCC] = {24, "swapcc", swapcc},

    [opcode::SYS] = {6, "sys", sys}, // 6 bits is special case
    [opcode::HALT] = {27, "halt", halt},
};

instruction::instruction(uint32_t value)
{
    opcode = (value >> 27);
    dst = (value >> 24) & 0x7;
    src = (value >> 21) & 0x7;
    modifier = (value >> 18) & 0x7;
    data = value & maskbits(opcodes[opcode].datasize);
}

namespace po = boost::program_options;

enum VerbosityLevel {
    ERROR = 0,
    WARNING = 1,
    INFO = 2,
    DEBUG = 3,
};
int gVerbosity = 0;

typedef void (*progressFunc)(cpu& c, machine& s);

void run_to_halt(cpu& c, machine& m, bool harvard, uint32_t *program, unsigned long long* instructions, progressFunc progress)
{
    std::chrono::time_point<std::chrono::system_clock> then = std::chrono::system_clock::now();

    while(!c.halted) {
        uint32_t pc = c.registers[reg::PC];
        bool fault = false;
        uint32_t word;
        if(harvard)
            word = program[pc / 4];
        else
            m.fetch32(pc, &word, &fault);
        instruction instr(word);

        if(fault) {
            printf("memory fault at 0x%08X\n", pc);
            continue;
        }

        if(gVerbosity >= VerbosityLevel::DEBUG) {
            printf("decoded %s", opcodes[instr.opcode].name);
            if(opcodes[instr.opcode].datasize == 18) {
                printf(", dst = %d, src = %d, size = %d, data = 0x%X\n", instr.dst, instr.src, instr.modifier, instr.data);
            } else if(opcodes[instr.opcode].datasize == 24) {
                printf(", dst = %d, src = %d, data = 0x%X\n", instr.dst, instr.src, instr.data);
            } else /* if(opcodes[instr.opcode].datasize == 27) or 6 */ {
                printf(", dst = %d, data = 0x%X\n", instr.dst, instr.data);
            }
        }

        opcodes[instr.opcode].func(c, m, instr);

        // if(m.memory_fault) {
            // printf("memory fault at 0x%08X\n", m.fault_address);
            // continue;
        // }

        if(gVerbosity >= VerbosityLevel::DEBUG) {
            printf("R0:%08X R1:%08X R2:%08X R3:%08X\n",
                c.registers[0], c.registers[1], c.registers[2], c.registers[3]);
            printf("R4:%08X R5:%08X SP:%08X PC:%08X\n", 
                c.registers[4], c.registers[5], c.registers[6], c.registers[7]);

            for(auto it = m.memory_changed.begin(); it != m.memory_changed.end(); it++) {
                uint32_t addr = *it;
                if(addr < memsize) {
                    uint32_t data;
                    bool fault;
                    m.fetch8(addr, &data, &fault);
                    printf("memory changed %08x : %02X\n", addr, data);
                }
            }
            m.memory_changed.clear();
        }

        (*instructions)++;

        std::chrono::time_point<std::chrono::system_clock> now =
            std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = now - then;
        if(elapsed_seconds.count() > .01) {
            progress(c, m);
            then = std::chrono::system_clock::now();
        }
    }
}

#ifdef USE_OPENGL

bool gPrintShaderLog = true;

GLuint gFramebufferTexture;

static bool CheckShaderCompile(GLuint shader, const std::string& shader_name)
{
    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status == GL_TRUE)
	return true;

    if(gPrintShaderLog) {
        int length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

        if (length > 0) {
            char log[length];
            glGetShaderInfoLog(shader, length, NULL, log);
            fprintf(stderr, "%s shader error log:\n%s\n", shader_name.c_str(), log);
        }

        fprintf(stderr, "%s compile failure.\n", shader_name.c_str());
        fprintf(stderr, "shader text:\n");
        glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &length);
        char source[length];
        glGetShaderSource(shader, length, NULL, source);
        fprintf(stderr, "%s\n", source);
    }
    return false;
}

static bool CheckProgramLink(GLuint program)
{
    int status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if(status == GL_TRUE)
	return true;

    if(gPrintShaderLog) {
        int log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

        if (log_length > 0) {
            char log[log_length];
            glGetProgramInfoLog(program, log_length, NULL, log);
            fprintf(stderr, "program error log: %s\n",log);
        }
    }

    return false;
}

GLint modelview_uniform = -1;
GLint texture_uniform = -1;
GLint pos_attrib = 0, texcoord_attrib = 1;
GLuint decalprogram;

static const char *gDecalVertexShaderText =
"#version 140\n"
"uniform mat4 modelview;\n"
"in vec4 pos;\n"
"in vec2 vtex;\n"
"out vec2 ftex;\n"
"\n"
"void main()\n"
"{\n"
"\n"
"    gl_Position = modelview * pos;\n"
"    ftex = vtex;\n"
"}\n";

static const char *gDecalFragmentShaderText =
"#version 140\n"
"uniform sampler2D texture_image;\n"
"\n"
"in vec2 ftex;\n"
"out vec4 color;\n"
"\n"
"void main()\n"
"{\n"
"    color = texture(texture_image, ftex);\n"
"}\n";

GLuint vert_buffer;
GLuint texcoord_buffer;
GLuint screenquad_vao;
GLuint decal_vertex_shader;

static void CheckOpenGL(const char *filename, int line)
{
    int glerr;

    if((glerr = glGetError()) != GL_NO_ERROR) {
        printf("GL Error: %04X at %s:%d\n", glerr, filename, line);
    }
}


void init_decal_program()
{
    CheckOpenGL(__FILE__, __LINE__);

    decal_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    const char *string = gDecalVertexShaderText;
    glShaderSource(decal_vertex_shader, 1, &string, NULL);
    glCompileShader(decal_vertex_shader);
    if(!CheckShaderCompile(decal_vertex_shader, "vertex shader"))
	exit(1);

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    string = gDecalFragmentShaderText;
    glShaderSource(fragment_shader, 1, &string, NULL);
    glCompileShader(fragment_shader);
    if(!CheckShaderCompile(fragment_shader, "fragment shader"))
	exit(1);

    decalprogram = glCreateProgram();
    glAttachShader(decalprogram, decal_vertex_shader);
    glAttachShader(decalprogram, fragment_shader);
    glBindAttribLocation(decalprogram, pos_attrib, "pos");
    glBindAttribLocation(decalprogram, texcoord_attrib, "vtex");
    glLinkProgram(decalprogram);
    if(!CheckProgramLink(decalprogram))
	exit(1);

    modelview_uniform = glGetUniformLocation(decalprogram, "modelview");
    texture_uniform = glGetUniformLocation(decalprogram, "texture_image");
    glUseProgram(decalprogram);
}

void init_decal_geometry(void)
{
    float verts[4][4];
    float texcoords[4][2];

    verts[0][0] = -1;
    verts[0][1] = -1;
    verts[0][2] = 0;
    verts[0][3] = 1;
    verts[1][0] = 1; // gWindowWidth;
    verts[1][1] = -1;
    verts[1][2] = 0;
    verts[1][3] = 1;
    verts[2][0] = -1;
    verts[2][1] = 1; // gWindowHeight;
    verts[2][2] = 0;
    verts[2][3] = 1;
    verts[3][0] = 1; // gWindowWidth;
    verts[3][1] = 1; // gWindowHeight;
    verts[3][2] = 0;
    verts[3][3] = 1;

    texcoords[0][0] = 0;
    texcoords[0][1] = 1;
    texcoords[1][0] = 1;
    texcoords[1][1] = 1;
    texcoords[2][0] = 0;
    texcoords[2][1] = 0;
    texcoords[3][0] = 1;
    texcoords[3][1] = 0;

    glGenVertexArrays(1, &screenquad_vao);
    glBindVertexArray(screenquad_vao);
    glGenBuffers(1, &vert_buffer);
    glGenBuffers(1, &texcoord_buffer);

    CheckOpenGL(__FILE__, __LINE__);

    glBindBuffer(GL_ARRAY_BUFFER, vert_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(pos_attrib, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(pos_attrib);

    glBindBuffer(GL_ARRAY_BUFFER, texcoord_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
    glVertexAttribPointer(texcoord_attrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(texcoord_attrib);
}

void DrawFrame(GLFWwindow* window)
{
    glUseProgram(decalprogram);

    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, gFramebufferTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindBuffer(GL_ARRAY_BUFFER, vert_buffer);
    glVertexAttribPointer(pos_attrib, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(pos_attrib);

    glBindBuffer(GL_ARRAY_BUFFER, texcoord_buffer);
    glVertexAttribPointer(texcoord_attrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(texcoord_attrib);

    static float identity[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    glUniformMatrix4fv(modelview_uniform, 1, GL_FALSE, identity);

    glBindVertexArray(screenquad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glfwSwapBuffers(window);
}

void InitializeGL(machine& m)
{
    glGenTextures(1, &gFramebufferTexture);
    glBindTexture(GL_TEXTURE_2D, gFramebufferTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R3_G3_B2, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE_3_3_2, m.memory);
    glGenerateMipmap(GL_TEXTURE_2D);
    init_decal_program();
    init_decal_geometry();
}

GLFWwindow *gWindow;

void progress(cpu& c, machine& m)
{
    glBindTexture(GL_TEXTURE_2D, gFramebufferTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R3_G3_B2, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE_3_3_2, m.memory);
    glGenerateMipmap(GL_TEXTURE_2D);

    DrawFrame(gWindow);
    glfwPollEvents();
}

static void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "GLFW: %s\n", description);
}

static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_PRESS) {
        switch(key) {
            case 'Q': case '\033':
                glfwSetWindowShouldClose(window, GL_TRUE);
                break;
        }
    }
}

int gWindowWidth;
int gWindowHeight;

static void ResizeCallback(GLFWwindow *window, int x, int y)
{
    glfwGetFramebufferSize(window, &gWindowWidth, &gWindowHeight);
    glViewport(0, 0, gWindowWidth, gWindowHeight);
}

static void ButtonCallback(GLFWwindow *window, int b, int action, int mods)
{
}

static void MotionCallback(GLFWwindow *window, double x, double y)
{
}

static void ScrollCallback(GLFWwindow *window, double dx, double dy)
{
}


void InitializeGLFW(machine& m)
{
    glfwSetErrorCallback(ErrorCallback);

    if(!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 

    glfwWindowHint(GLFW_SAMPLES, 4);
    gWindow = glfwCreateWindow(512, 512, "JimSim", NULL, NULL);
    if (!gWindow) {
        glfwTerminate();
        fprintf(stdout, "Couldn't open main window\n");
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(gWindow);

    InitializeGL(m);

    glfwSetKeyCallback(gWindow, KeyCallback);
    glfwSetMouseButtonCallback(gWindow, ButtonCallback);
    glfwSetCursorPosCallback(gWindow, MotionCallback);
    glfwSetScrollCallback(gWindow, ScrollCallback);
    glfwSetFramebufferSizeCallback(gWindow, ResizeCallback);
    glfwSetWindowRefreshCallback(gWindow, DrawFrame);
}

void TerminateGLFW()
{
    glfwTerminate();
}

#else

void progress(cpu& c, machine& m)
{
}

#endif

int main(int argc, char **argv)
{
    static machine m; 
    static cpu c; 
    unsigned long long instructions = 0;
    bool harvard = false;
    const int programsize = 128 * 1024;
    uint32_t *program = NULL;

    po::options_description desc("Simulator options");
    desc.add_options()
        ("help", "produce help message")
        ("verbose", po::value<int>(&gVerbosity)->default_value(VerbosityLevel::ERROR), "set verbosity level")
        ("harvard", po::value(&harvard)->zero_tokens(), "use Harvard architecture (instructions separate from RAM)")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);    

    if (vm.count("help")) {
        std::cout << desc << "\n";
        exit(EXIT_SUCCESS);
    }

    if(harvard) {

        program = new uint32_t[programsize];
        uint32_t addr = 0;
        while(fread(&program[addr], 4, 1, stdin) == 1) {
            addr++;
        }

    } else {

        uint32_t addr = 0;
        uint32_t d;
        while(fread(&d, 4, 1, stdin) == 1) {
            bool fault;
            m.store32(addr, d, &fault);
            addr += 4;
        }
    }

#ifdef USE_OPENGL
    InitializeGLFW(m);
#endif

    run_to_halt(c, m, harvard, program, &instructions, progress);

    if(gVerbosity >= VerbosityLevel::INFO) {
        printf("R0:%08X R1:%08X R2:%08X R3:%08X\n",
            c.registers[0], c.registers[1], c.registers[2], c.registers[3]);
        printf("R4:%08X R5:%08X SP:%08X PC:%08X\n", 
            c.registers[4], c.registers[5], c.registers[6], c.registers[7]);
        printf("%llu instructions executed\n", instructions);
    }

#ifdef USE_OPENGL
    TerminateGLFW();
#endif
}
