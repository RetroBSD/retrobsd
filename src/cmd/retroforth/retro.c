/* Ngaro VM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Copyright (c) 2008 - 2011, Charles Childers
   Copyright (c) 2009 - 2010, Luke Parrish
   Copyright (c) 2010,        Marc Simpson
   Copyright (c) 2010,        Jay Skeer
   Copyright (c) 2011,        Kenneth Keating
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#if 0
#   include <termios.h>
#else
#   define termios sgttyb
#endif
#include <sys/ioctl.h>

/* Configuration ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

                +---------+---------+---------+
                | 16 bit  | 32 bit  | 64 bit  |
   +------------+---------+---------+---------+
   | IMAGE_SIZE | 32000   | 1000000 | 1000000 |
   +------------+---------+---------+---------+
   | CELL       | int16_t | int32_t | int64_t |
   +------------+---------+---------+---------+

   If memory is tight, cut the MAX_FILE_NAME and MAX_REQUEST_LENGTH.

   You can also cut the ADDRESSES stack size down, but if you have
   heavy nesting or recursion this may cause problems. If you do modify
   it and experience odd problems, try raising it a bit higher.

   Use -DRX16 to select defaults for 16-bit, or -DRX64 to select the
   defaults for 64-bit. Without these, the compiler will generate a
   standard 32-bit VM.

   Use -DRXBE to enable the BE suffix for big endian images. This is
   only useful on big endian systems.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define CELL            int32_t
#define IMAGE_SIZE        10000
#define ADDRESSES          1024
#define STACK_DEPTH         128
#define PORTS                12
#define MAX_FILE_NAME      1024
#define MAX_REQUEST_LENGTH 1024
#define MAX_OPEN_FILES        8
#define LOCAL                 "/lib/retroImage"
#define CELLSIZE             32

#ifdef RX64
#undef CELL
#undef CELLSIZE
#undef LOCAL
#define CELL     int64_t
#define CELLSIZE 64
#define LOCAL    "retroImage64"
#endif

#ifdef RX16
#undef CELL
#undef CELLSIZE
#undef LOCAL
#undef IMAGE_SIZE
#define CELL        int16_t
#define CELLSIZE    16
#define IMAGE_SIZE  32000
#define LOCAL       "retroImage16"
#endif

#ifdef RXBE
#define LOCAL_FNAME (LOCAL "BE")
#define VM_ENDIAN 1
#else
#define LOCAL_FNAME (LOCAL)
#define VM_ENDIAN 0
#endif


enum vm_opcode {VM_NOP, VM_LIT, VM_DUP, VM_DROP, VM_SWAP, VM_PUSH, VM_POP,
                VM_LOOP, VM_JUMP, VM_RETURN, VM_GT_JUMP, VM_LT_JUMP,
                VM_NE_JUMP,VM_EQ_JUMP, VM_FETCH, VM_STORE, VM_ADD,
                VM_SUB, VM_MUL, VM_DIVMOD, VM_AND, VM_OR, VM_XOR, VM_SHL,
                VM_SHR, VM_ZERO_EXIT, VM_INC, VM_DEC, VM_IN, VM_OUT,
                VM_WAIT };
#define NUM_OPS VM_WAIT + 1

typedef struct {
  CELL sp, rsp, ip;
  CELL data[STACK_DEPTH];
  CELL address[ADDRESSES];
  CELL ports[PORTS];
  FILE *files[MAX_OPEN_FILES];
  FILE *input[MAX_OPEN_FILES];
  CELL isp;
  CELL image[IMAGE_SIZE];
  CELL shrink, padding;
  int stats[NUM_OPS + 1];
  int max_sp, max_rsp;
  char filename[MAX_FILE_NAME];
  char request[MAX_REQUEST_LENGTH];
  struct termios new_termios, old_termios;
} VM;

/* Macros ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define IP   vm->ip
#define SP   vm->sp
#define RSP  vm->rsp
#define DROP { vm->data[SP] = 0; if (--SP < 0) IP = IMAGE_SIZE; }
#define TOS  vm->data[SP]
#define NOS  vm->data[SP-1]
#define TORS vm->address[RSP]

/* Helper Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void rxGetString(VM *vm, int starting)
{
  CELL i = 0;
  while(vm->image[starting] && i < MAX_REQUEST_LENGTH)
    vm->request[i++] = (char)vm->image[starting++];
  vm->request[i] = 0;
}

/* Console I/O Support ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void rxWriteConsole(VM *vm, CELL c) {
  (c > 0) ? putchar((char)c) : printf("\033[2J\033[1;1H");
  /* Erase the previous character if c = backspace */
  if (c == 8) {
    putchar(32);
    putchar(8);
  }
}

CELL rxReadConsole(VM *vm) {
  CELL c;
  if ((c = getc(vm->input[vm->isp])) == EOF && vm->input[vm->isp] != stdin) {
    fclose(vm->input[vm->isp--]);
    c = 0;
  }
  if (c == EOF && vm->input[vm->isp] == stdin)
    exit(0);
  return c;
}

void rxIncludeFile(VM *vm, char *s) {
  FILE *file;
  if ((file = fopen(s, "r")))
    vm->input[++vm->isp] = file;
}

void rxPrepareInput(VM *vm) {
  vm->isp = 0;
  vm->input[vm->isp] = stdin;
}

void rxPrepareOutput(VM *vm) {
#if 0
  tcgetattr(0, &vm->old_termios);
  vm->new_termios = vm->old_termios;
  vm->new_termios.c_iflag &= ~(BRKINT+ISTRIP+IXON+IXOFF);
  vm->new_termios.c_iflag |= (IGNBRK+IGNPAR);
  vm->new_termios.c_lflag &= ~(ICANON+ISIG+IEXTEN+ECHO);
  vm->new_termios.c_cc[VMIN] = 1;
  vm->new_termios.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, &vm->new_termios);
#else
  ioctl(0, TIOCGETP, &vm->old_termios);
  vm->new_termios = vm->old_termios;
  vm->new_termios.sg_flags &= ~(ECHO | CRMOD | XTABS | RAW);
  vm->new_termios.sg_flags |= CBREAK;
  ioctl(0, TIOCSETP, &vm->new_termios);
#endif
}

void rxRestoreIO(VM *vm) {
#if 0
  tcsetattr(0, TCSANOW, &vm->old_termios);
#else
  ioctl(0, TIOCSETP, &vm->old_termios);
#endif
}

/* File I/O Support ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
CELL rxGetFileHandle(VM *vm)
{
  CELL i;
  for(i = 1; i < MAX_OPEN_FILES; i++)
    if (vm->files[i] == 0)
      return i;
  return 0;
}

void rxAddInputSource(VM *vm) {
  CELL name = TOS; DROP;
  rxGetString(vm, name);
  rxIncludeFile(vm, vm->request);
}

CELL rxOpenFile(VM *vm) {
  CELL slot, mode, name;
  slot = rxGetFileHandle(vm);
  mode = TOS; DROP;
  name = TOS; DROP;
  rxGetString(vm, name);
  if (slot > 0)
  {
    if (mode == 0)  vm->files[slot] = fopen(vm->request, "r");
    if (mode == 1)  vm->files[slot] = fopen(vm->request, "w");
    if (mode == 2)  vm->files[slot] = fopen(vm->request, "a");
    if (mode == 3)  vm->files[slot] = fopen(vm->request, "r+");
  }
  if (vm->files[slot] == NULL)
  {
    vm->files[slot] = 0;
    slot = 0;
  }
  return slot;
}

CELL rxReadFile(VM *vm) {
  CELL c = fgetc(vm->files[TOS]); DROP;
  return (c == EOF) ? 0 : c;
}

CELL rxWriteFile(VM *vm) {
  CELL slot, c, r;
  slot = TOS; DROP;
  c = TOS; DROP;
  r = fputc(c, vm->files[slot]);
  return (r == EOF) ? 0 : 1;
}

CELL rxCloseFile(VM *vm) {
  fclose(vm->files[TOS]);
  vm->files[TOS] = 0;
  DROP;
  return 0;
}

CELL rxGetFilePosition(VM *vm) {
  CELL slot = TOS; DROP;
  return (CELL) ftell(vm->files[slot]);
}

CELL rxSetFilePosition(VM *vm) {
  CELL slot, pos, r;
  slot = TOS; DROP;
  pos  = TOS; DROP;
  r = fseek(vm->files[slot], pos, SEEK_SET);
  return r;
}

CELL rxGetFileSize(VM *vm) {
  CELL slot, current, r, size;
  slot = TOS; DROP;
  current = ftell(vm->files[slot]);
  r = fseek(vm->files[slot], 0, SEEK_END);
  size = ftell(vm->files[slot]);
  fseek(vm->files[slot], current, SEEK_SET);
  return (r == 0) ? size : 0;
}

CELL rxDeleteFile(VM *vm) {
  CELL name = TOS; DROP;
  rxGetString(vm, name);
  return (unlink(vm->request) == 0) ? -1 : 0;
}

CELL rxLoadImage(VM *vm, char *image) {
  FILE *fp;
  CELL x = 0;

  if ((fp = fopen(image, "rb")) != NULL) {
    x = fread(&vm->image, sizeof(CELL), IMAGE_SIZE, fp);
    fclose(fp);
  }
  else {
    printf("Unable to find the retroImage!\n");
    exit(1);
  }
  return x;
}

CELL rxSaveImage(VM *vm, char *image) {
  FILE *fp;
  CELL x = 0;

  if ((fp = fopen(image, "wb")) == NULL)
  {
    printf("Unable to save the retroImage!\n");
    rxRestoreIO(vm);
    exit(2);
  }

  if (vm->shrink == 0)
    x = fwrite(&vm->image, sizeof(CELL), IMAGE_SIZE, fp);
  else
    x = fwrite(&vm->image, sizeof(CELL), vm->image[3], fp);
  fclose(fp);

  return x;
}

/* Environment Query ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void rxQueryEnvironment(VM *vm) {
  CELL req, dest;
  char *r;
  req = TOS;  DROP;
  dest = TOS; DROP;

  rxGetString(vm, req);
  r = getenv(vm->request);

  if (r != 0)
    while (*r != '\0')
    {
      vm->image[dest] = *r;
      dest++;
      r++;
    }
  else
    vm->image[dest] = 0;
}

/* Device I/O Handler ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void rxDeviceHandler(VM *vm) {
  struct winsize w;
  if (vm->ports[0] != 1) {
    /* Input */
    if (vm->ports[0] == 0 && vm->ports[1] == 1) {
      vm->ports[1] = rxReadConsole(vm);
      vm->ports[0] = 1;
    }

    /* Output (character generator) */
    if (vm->ports[2] == 1) {
      rxWriteConsole(vm, TOS); DROP
      vm->ports[2] = 0;
      vm->ports[0] = 1;
    }

    /* File IO and Image Saving */
    if (vm->ports[4] != 0) {
      vm->ports[0] = 1;
      switch (vm->ports[4]) {
        case  1: rxSaveImage(vm, vm->filename);
                 vm->ports[4] = 0;
                 break;
        case  2: rxAddInputSource(vm);
                 vm->ports[4] = 0;
                 break;
        case -1: vm->ports[4] = rxOpenFile(vm);
                 break;
        case -2: vm->ports[4] = rxReadFile(vm);
                 break;
        case -3: vm->ports[4] = rxWriteFile(vm);
                 break;
        case -4: vm->ports[4] = rxCloseFile(vm);
                 break;
        case -5: vm->ports[4] = rxGetFilePosition(vm);
                 break;
        case -6: vm->ports[4] = rxSetFilePosition(vm);
                 break;
        case -7: vm->ports[4] = rxGetFileSize(vm);
                 break;
        case -8: vm->ports[4] = rxDeleteFile(vm);
                 break;
        default: vm->ports[4] = 0;
      }
    }

    /* Capabilities */
    if (vm->ports[5] != 0) {
      vm->ports[0] = 1;
      switch(vm->ports[5]) {
        case -1:  vm->ports[5] = IMAGE_SIZE;
                  break;
        case -2:  vm->ports[5] = 0;
                  break;
        case -3:  vm->ports[5] = 0;
                  break;
        case -4:  vm->ports[5] = 0;
                  break;
        case -5:  vm->ports[5] = SP;
                  break;
        case -6:  vm->ports[5] = RSP;
                  break;
        case -7:  vm->ports[5] = 0;
                  break;
        case -8:  vm->ports[5] = time(NULL);
                  break;
        case -9:  vm->ports[5] = 0;
                  IP = IMAGE_SIZE;
                  break;
        case -10: vm->ports[5] = 0;
                  rxQueryEnvironment(vm);
                  break;
        case -11: ioctl(0, TIOCGWINSZ, &w);
                  vm->ports[5] = w.ws_col;
                  break;
        case -12: ioctl(0, TIOCGWINSZ, &w);
                  vm->ports[5] = w.ws_row;
                  break;
        case -13: vm->ports[5] = CELLSIZE;
                  break;
        case -14: vm->ports[5] = VM_ENDIAN;
                  break;
        default:  vm->ports[5] = 0;
      }
    }
  }
}

/* The VM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void rxProcessOpcode(VM *vm) {
  CELL a, b, opcode;
  opcode = vm->image[IP];

  if (opcode > NUM_OPS)
    vm->stats[NUM_OPS]++;
  else
    vm->stats[opcode]++;

  switch(opcode) {
    case VM_NOP:
         break;
    case VM_LIT:
         SP++;
         IP++;
         TOS = vm->image[IP];
         if (vm->max_sp < SP)
           vm->max_sp = SP;
         break;
    case VM_DUP:
         SP++;
         vm->data[SP] = NOS;
         if (vm->max_sp < SP)
           vm->max_sp = SP;
         break;
    case VM_DROP:
         DROP
         break;
    case VM_SWAP:
         a = TOS;
         TOS = NOS;
         NOS = a;
         break;
    case VM_PUSH:
         RSP++;
         TORS = TOS;
         DROP
         if (vm->max_rsp < RSP)
           vm->max_rsp = RSP;
         break;
    case VM_POP:
         SP++;
         TOS = TORS;
         RSP--;
         break;
    case VM_LOOP:
         TOS--;
           IP++;
         if (TOS != 0 && TOS > -1)
           IP = vm->image[IP] - 1;
         else
           DROP;
         break;
    case VM_JUMP:
         IP++;
         IP = vm->image[IP] - 1;
         if (IP < 0)
           IP = IMAGE_SIZE;
         else {
           if (vm->image[IP+1] == 0)
             IP++;
           if (vm->image[IP+1] == 0)
             IP++;
         }
         break;
    case VM_RETURN:
         IP = TORS;
         RSP--;
         if (IP < 0)
           IP = IMAGE_SIZE;
         else {
           if (vm->image[IP+1] == 0)
             IP++;
           if (vm->image[IP+1] == 0)
             IP++;
         }
         break;
    case VM_GT_JUMP:
         IP++;
         if(NOS > TOS)
           IP = vm->image[IP] - 1;
         DROP DROP
         break;
    case VM_LT_JUMP:
         IP++;
         if(NOS < TOS)
           IP = vm->image[IP] - 1;
         DROP DROP
         break;
    case VM_NE_JUMP:
         IP++;
         if(TOS != NOS)
           IP = vm->image[IP] - 1;
         DROP DROP
         break;
    case VM_EQ_JUMP:
         IP++;
         if(TOS == NOS)
           IP = vm->image[IP] - 1;
         DROP DROP
         break;
    case VM_FETCH:
         TOS = vm->image[TOS];
         break;
    case VM_STORE:
         vm->image[TOS] = NOS;
         DROP DROP
         break;
    case VM_ADD:
         NOS += TOS;
         DROP
         break;
    case VM_SUB:
         NOS -= TOS;
         DROP
         break;
    case VM_MUL:
         NOS *= TOS;
         DROP
         break;
    case VM_DIVMOD:
         a = TOS;
         b = NOS;
         TOS = b / a;
         NOS = b % a;
         break;
    case VM_AND:
         a = TOS;
         b = NOS;
         DROP
         TOS = a & b;
         break;
    case VM_OR:
         a = TOS;
         b = NOS;
         DROP
         TOS = a | b;
         break;
    case VM_XOR:
         a = TOS;
         b = NOS;
         DROP
         TOS = a ^ b;
         break;
    case VM_SHL:
         a = TOS;
         b = NOS;
         DROP
         TOS = b << a;
         break;
    case VM_SHR:
         a = TOS;
         DROP
         TOS >>= a;
         break;
    case VM_ZERO_EXIT:
         if (TOS == 0) {
           DROP
           IP = TORS;
           RSP--;
         }
         break;
    case VM_INC:
         TOS += 1;
         break;
    case VM_DEC:
         TOS -= 1;
         break;
    case VM_IN:
         a = TOS;
         TOS = vm->ports[a];
         vm->ports[a] = 0;
         break;
    case VM_OUT:
         vm->ports[0] = 0;
         vm->ports[TOS] = NOS;
         DROP DROP
         break;
    case VM_WAIT:
         rxDeviceHandler(vm);
         break;
    default:
         RSP++;
         TORS = IP;
         IP = vm->image[IP] - 1;

         if (IP < 0)
           IP = IMAGE_SIZE;
         else {
           if (vm->image[IP+1] == 0)
             IP++;
           if (vm->image[IP+1] == 0)
             IP++;
         }
         if (vm->max_rsp < RSP)
           vm->max_rsp = RSP;
         break;
  }
  vm->ports[3] = 1;
}

/* Stats ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
void rxDisplayStats(VM *vm)
{
  int s, i;

  printf("Runtime Statistics\n");
  printf("NOP:     %d\n", vm->stats[VM_NOP]);
  printf("LIT:     %d\n", vm->stats[VM_LIT]);
  printf("DUP:     %d\n", vm->stats[VM_DUP]);
  printf("DROP:    %d\n", vm->stats[VM_DROP]);
  printf("SWAP:    %d\n", vm->stats[VM_SWAP]);
  printf("PUSH:    %d\n", vm->stats[VM_PUSH]);
  printf("POP:     %d\n", vm->stats[VM_POP]);
  printf("LOOP:    %d\n", vm->stats[VM_LOOP]);
  printf("JUMP:    %d\n", vm->stats[VM_JUMP]);
  printf("RETURN:  %d\n", vm->stats[VM_RETURN]);
  printf(">JUMP:   %d\n", vm->stats[VM_GT_JUMP]);
  printf("<JUMP:   %d\n", vm->stats[VM_LT_JUMP]);
  printf("!JUMP:   %d\n", vm->stats[VM_NE_JUMP]);
  printf("=JUMP:   %d\n", vm->stats[VM_EQ_JUMP]);
  printf("FETCH:   %d\n", vm->stats[VM_FETCH]);
  printf("STORE:   %d\n", vm->stats[VM_STORE]);
  printf("ADD:     %d\n", vm->stats[VM_ADD]);
  printf("SUB:     %d\n", vm->stats[VM_SUB]);
  printf("MUL:     %d\n", vm->stats[VM_MUL]);
  printf("DIVMOD:  %d\n", vm->stats[VM_DIVMOD]);
  printf("AND:     %d\n", vm->stats[VM_AND]);
  printf("OR:      %d\n", vm->stats[VM_OR]);
  printf("XOR:     %d\n", vm->stats[VM_XOR]);
  printf("SHL:     %d\n", vm->stats[VM_SHL]);
  printf("SHR:     %d\n", vm->stats[VM_SHR]);
  printf("0;:      %d\n", vm->stats[VM_ZERO_EXIT]);
  printf("INC:     %d\n", vm->stats[VM_INC]);
  printf("DEC:     %d\n", vm->stats[VM_DEC]);
  printf("IN:      %d\n", vm->stats[VM_IN]);
  printf("OUT:     %d\n", vm->stats[VM_OUT]);
  printf("WAIT:    %d\n", vm->stats[VM_WAIT]);
  printf("CALL:    %d\n", vm->stats[NUM_OPS]);
  printf("Max SP:  %d\n", vm->max_sp);
  printf("Max RSP: %d\n", vm->max_rsp);

  for (s = i = 0; s < NUM_OPS; s++)
    i += vm->stats[s];
  printf("Total opcodes processed: %d\n", i);
}

/* Main ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
int main(int argc, char **argv) {
  VM *vm;
  int i, wantsStats;

  wantsStats = 0;
  vm = calloc(sizeof(VM), sizeof(char));
  strcpy(vm->filename, LOCAL_FNAME);

  rxPrepareInput(vm);

  /* Parse the command line arguments */
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--with") == 0)
      rxIncludeFile(vm, argv[++i]);
    if (strcmp(argv[i], "--image") == 0)
      strcpy(vm->filename, argv[++i]);
    if (strcmp(argv[i], "--shrink") == 0)
      vm->shrink = 1;
    if (strcmp(argv[i], "--stats") == 0)
      wantsStats = 1;
  }

  if (rxLoadImage(vm, vm->filename) == 0) {
    printf("Sorry, unable to find %s\n", vm->filename);
    free(vm);
    exit(1);
  }

  rxPrepareOutput(vm);
  for (IP = 0; IP < IMAGE_SIZE; IP++)
    rxProcessOpcode(vm);
  rxRestoreIO(vm);

  if (wantsStats == 1)
    rxDisplayStats(vm);

  free(vm);
  return 0;
}
