#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE 500 //process address space size

int PAS[ARRAY_SIZE] = {0}; //process address space array initialized to 0
int TRACK[ARRAY_SIZE];  //to store base pointers for AR boundaries
int recordSize = 0;     //keeps track of the number of stored AR boundaries

//base function: find base L levels down
int base(int BP, int L) {
    int arb = BP;
    while (L > 0) {
        arb = PAS[arb];
        L--;
    }
    return arb;
}

//function to print the stack with activation record bars
void printStack(int PC, int BP, int SP, int L, int M) {
    printf("\t%d\t%d\t%d\t%d\t%d\t", L, M, PC, BP, SP);

    //use the existing record array for AR boundaries
    int currBase[recordSize]; // Temporary array to store base pointers
    int bpCount = 0;

    //copy base pointers from the record array
    for (int i = 0; i < recordSize; i++) {
        currBase[bpCount++] = TRACK[i];
    }

    //print stack content
    for (int i = ARRAY_SIZE - 1; i >= SP; i--) {
        //print a "|" for each AR boundary
        for (int j = 0; j < bpCount; j++) {
            if (i == currBase[j]) {
                printf("| ");
                break;
            }
        }
        printf("%d ", PAS[i]); //print stack value
    }
    printf("\n");
}


//main function to execute the PM/0 virtual machine
int main(int argc, char *argv[]) {
    int BP = 499;    //base pointer
    int SP = BP + 1; //stack pointer (stack grows downward)
    int PC = 10;     //program counter (execution starts at index 10)
    int IR_OP, IR_L, IR_M; //instruction Register
    int eop = 1; //halt flag for execution

    //check for correct number of arguments
    if (argc != 2) {
        printf("Usage: %s input.txt\n", argv[0]);
        return 1;
    }

    //open input file
    FILE *input = fopen(argv[1], "r");
    if (!input) {
        printf("Error: Cannot open file %s\n", argv[1]);
        return 1;
    }

    //load instructions into PAS starting at index 10
    int i = 10;
    while (fscanf(input, "%d %d %d", &PAS[i], &PAS[i + 1], &PAS[i + 2]) == 3) {
        i += 3;
        if (i >= ARRAY_SIZE) {
            printf("Error: Instructions exceed PAS array size.\n");
            fclose(input);
            return 1;
        }
    }
    fclose(input);

    //initial CPU state
    printf("\t\t\t\tPC\tBP\tSP\tstack\n");
    printf("Initial values:\t\t\t%d\t%d\t%d\n\n", PC, BP, SP);

    //execution loop
    while (eop) {
        //fetch cycle
        IR_OP = PAS[PC];
        IR_L = PAS[PC + 1];
        IR_M = PAS[PC + 2];
        PC += 3;

        //instruction handling with direct printing
        switch (IR_OP) {
            case 1: //LIT: pushes a constant value (literal) M onto the stack
                printf("\tLIT");
                SP--;
                PAS[SP] = IR_M;
                break;

            case 2: //OPR: Operation to be performed on the data at the top of the stack. (or return from function) 
                switch (IR_M) {
                    case 0: //RTN
                        printf("\tRTN");
                        SP = BP + 1;
                        BP = PAS[SP - 2];
                        PC = PAS[SP - 3];
                        recordSize--;  //remove current Activation Record base from record
                        break;
                    //arithmetic operations
                    case 1: //ADD
                        printf("\tADD");
                        PAS[SP + 1] = PAS[SP + 1] + PAS[SP];
                        SP++;
                        break;

                    case 2: //SUB
                        printf("\tSUB");
                        PAS[SP + 1] = PAS[SP + 1] - PAS[SP];
                        SP++;
                        break;

                    case 3: //MUL
                        printf("\tMUL");
                        PAS[SP + 1] = PAS[SP + 1] * PAS[SP];
                        SP++;
                        break;

                    case 4: //DIV
                        printf("\tDIV");
                        PAS[SP + 1] = PAS[SP + 1] / PAS[SP];
                        SP++;
                        break;

                    case 5: //EQL
                        printf("\tEQL");
                        PAS[SP + 1] = (PAS[SP + 1] == PAS[SP]);
                        SP++;
                        break;

                    case 6: //NEQ
                        printf("\tNEQ");
                        PAS[SP + 1] = (PAS[SP + 1] != PAS[SP]);
                        SP++;
                        break;

                    case 7: //LSS
                        printf("\tLSS");
                        PAS[SP + 1] = (PAS[SP + 1] < PAS[SP]);
                        SP++;
                        break;

                    case 8: //LEQ
                        printf("\tLEQ");
                        PAS[SP + 1] = (PAS[SP + 1] <= PAS[SP]);
                        SP++;
                        break;

                    case 9: //GTR
                        printf("\tGTR");
                        PAS[SP + 1] = (PAS[SP + 1] > PAS[SP]);
                        SP++;
                        break;

                    case 10: //GEQ
                        printf("\tGEQ");
                        PAS[SP + 1] = (PAS[SP + 1] >= PAS[SP]);
                        SP++;
                        break;

                    default:
                        printf("Error: Unknown OPR instruction %d\n", IR_M);
                        eop = 0;
                        break;
                }
                break;

            case 3: //LOD: Load value to top of stack from the stack location at offset M from L lexicographical levels down
                printf("\tLOD");
                SP--;
                PAS[SP] = PAS[base(BP, IR_L) - IR_M];
                break;

            case 4: //STO: Store value at top of stack in the stack location at offset M from L lexicographical levels down
                printf("\tSTO");
                PAS[base(BP, IR_L) - IR_M] = PAS[SP];
                SP++;
                break;

            case 5: //CAL: Call procedure at code index M (generates new Activation Record and PC  M)
                printf("\tCAL");
                PAS[SP - 1] = base(BP, IR_L);
                PAS[SP - 2] = BP;
                PAS[SP - 3] = PC;
                BP = SP - 1;
                PC = IR_M;
                TRACK[recordSize++] = BP;  //store the base pointer for AR
                break;

            case 6: //INC: Allocate M memory words (increment SP by M). First three are reserved to Static Link (SL), Dynamic Link (DL), and Return Address (RA)
                printf("\tINC");
                SP = SP - IR_M;
                break;

            case 7: //JMP: Jump to instruction M (PC  M)
                printf("\tJMP");
                PC = IR_M;
                break;

            case 8: //JPC: Jump to instruction M if top stack element is 0
                printf("\tJPC");
                if (PAS[SP] == 0) {
                    PC = IR_M;
                }
                SP++;
                break;

            case 9: //SYS: Write the top stack element to the screen
                switch (IR_M) {
                    case 1: //output
                        printf("Output result is: %d\n", PAS[SP]);
                        SP++;
                        break;

                    case 2: //input
                        SP--;
                        printf("Please Enter an Integer: ");
                        scanf("%d", &PAS[SP]);
                        break;

                    case 3: //halt
                        eop = 0;
                        break;

                    default:
                        printf("Error: Unknown SYS instruction %d\n", IR_M);
                        eop = 0;
                        break;
                }
                printf("\tSYS");
                break;

            default:
                printf("Error: Unknown instruction %d\n", IR_OP);
                eop = 0;
                break;
        }

        //function call to print stack after each instruction
        printStack(PC, BP, SP, IR_L, IR_M);
    }

    return 0;
}