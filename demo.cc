#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include "compiler.h"
#include "lexer.h"
#include <iostream> 
#include <string> 
#include <map>
#include <stack>

using namespace std;

//used for matching block scopes
struct Scope {
    InstructionNode* start;
    InstructionNode* end;
};

LexicalAnalyzer lexer;
map<string, int> variableAddressMap; //stores the memory location for all variables
map<int, int> constantAddressMap; //stores the memory location for all the constants
stack<pair<string, InstructionNode*>> pendingJump; //stores all instructions that need a jump stitched to them
stack<int> switchVariable; //stores the variable saved for comparison in the cases/switch statements
stack<int> switchMemory; //stores ther variable that tracks if a switch statement has entered a case
int jumpNextInstruction = 0; //stores if jump should be stitched
bool debugMode = false;

//this processes instructions "Line by Line"
InstructionNode* processLine(Token& token, int& tokenCount) {
    vector<Token> tokens;

    //push all the tokens to vector that make a statement
    while(token.token_type != END_OF_FILE) {
        tokenCount++;
        if(token.token_type == SEMICOLON)
            break;

        //LBRACE we only want to include it in the LINE statement for certain scenarios
        else if(token.token_type == LBRAC || token.token_type == LBRACE) {
            if(tokens.size() == 4 && tokens[0].token_type == IF)
                tokens.push_back(token);
            else if(tokens.size() == 4 && tokens[0].token_type == WHILE) 
                tokens.push_back(token);
            else if(tokens.size() == 2 && tokens[0].token_type == SWITCH) 
                tokens.push_back(token);
            else if(tokens.size() == 2 && tokens[0].token_type == DEFAULT)
                tokens.push_back(token);
            else if(tokens.size() == 3 && tokens[0].token_type == CASE) 
                tokens.push_back(token);
            else if(tokens.size() == 0) 
                tokens.push_back(token);
            else
                if(debugMode) cout << "NOT FOUR\n";
            break;
        } 
        //RBRACE we only include by itself, otherwise something is broken
        else if(token.token_type == RBRAC || token.token_type == RBRACE) {
            if(tokens.size() == 0) { 
                tokens.push_back(token);
                break;
            } 
            if(debugMode) cout << "NON LONE R BRACKET\n";
        }

        //PUSH AND KEEP GOING
        tokens.push_back(token);
        token = lexer.GetToken();
    }
    //DONE PARSING STATEMENT

    //TIME TO INITIALIZE INSTRUCTION
    struct InstructionNode* node = new InstructionNode;
    node->next = NULL;

    switch(tokens.size()) {
        case 1:
            //RBRACKET, BLOCK SCOPE ENDED - Signal to stitch jump instruction
            if(tokens[0].token_type == RBRACE) {
                if(!pendingJump.empty()) {
                    jumpNextInstruction++;
                    node->type = NOOP;
                    return node;
                } 
                if(debugMode) cout << "Empty pending jump\n";
            }

            break;
        case 2:
            //handle input variable ie: input a
            if(tokens[0].token_type == INPUT && tokens[1].token_type == ID) {
                if(debugMode) cout << "INPUT " << tokens[1].lexeme << "\n";
                node->type = IN;
                node->input_inst.var_index = variableAddressMap[tokens[1].lexeme]; //addr of variable
                return node;
            }

            //handle output variable ie: output a
            if(tokens[0].token_type == OUTPUT && tokens[1].token_type == ID) {
                if(debugMode) cout << "OUTPUT " << tokens[1].lexeme << "\n";
                node->type = OUT;
                node->output_inst.var_index = variableAddressMap[tokens[1].lexeme]; //addr of variable
                return node;
            }

            break;
        case 3:
            //handle "variable = number/memory" ie: "a = 5" OR "a = b"
            if(tokens[0].token_type == ID && tokens[1].token_type == EQUAL) {
                node->type = ASSIGN;
                node->assign_inst.left_hand_side_index = variableAddressMap[tokens[0].lexeme]; //addr of variable
                node->assign_inst.op = OPERATOR_NONE;
                if(tokens[2].token_type == NUM) {
                    if(debugMode) cout << tokens[0].lexeme << " = " << tokens[2].lexeme << "\n";
                    node->assign_inst.operand1_index = constantAddressMap[stoi(tokens[2].lexeme)]; //addr of num
                    return node;
                } else if(tokens[2].token_type == ID) {
                    if(debugMode) cout << tokens[0].lexeme << " = " << tokens[2].lexeme << "\n";
                    node->assign_inst.operand1_index = variableAddressMap[tokens[2].lexeme]; //addr of variable
                    return node;
                } 
                if(debugMode) cout << "ERROR UNEXPECTED CASE 3" << "\n";
                return NULL;
            } 
            //handle "SWITCH variable/number" { ie: "SWITCH a {" OR "SWITCH 1 {"
            else if(tokens[0].token_type == SWITCH && (tokens[1].token_type == ID 
            || tokens[1].token_type == NUM) && tokens[2].token_type == LBRACE) {
                if(debugMode) cout << "SWITCH " << tokens[1].lexeme << "\n";

                //here we add to switchStack the variable in question
                if(tokens[1].token_type == ID)
                    switchVariable.push(variableAddressMap[tokens[1].lexeme]); 
                else 
                    switchVariable.push(constantAddressMap[stoi(tokens[1].lexeme)]); 

                pendingJump.push({"SWITCH", node}); //mark as needing a potential jump
            }
            //handle "DEFAULT : {" ie: "DEFAULT : {"
            else if(tokens[0].token_type == DEFAULT && tokens[1].token_type == COLON 
            && tokens[2].token_type == LBRACE) {
                if(debugMode) cout << "DEFAULT :\n";
                node->type = CJMP;
                node->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
                node->cjmp_inst.operand1_index = switchMemory.top(); //addr of variable
                node->cjmp_inst.operand2_index = constantAddressMap[0]; //addr of num
                pendingJump.push({"DEFAULT", node}); //mark as needing a potential jump
                return node;
            }

            break;
        case 4:
            //handle "CASE NUM : {" ie: "CASE 1 : {"
            if(tokens[0].token_type == CASE && tokens[1].token_type == NUM
            && tokens[2].token_type == COLON && tokens[3].token_type == LBRACE) {
                if(debugMode) cout << "CASE " << tokens[1].lexeme << " :\n";
                node->type = CJMP;
                node->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
                node->cjmp_inst.operand1_index = switchVariable.top(); //addr of variable
                node->cjmp_inst.operand2_index = constantAddressMap[stoi(tokens[1].lexeme)]; //addr of num
                pendingJump.push({"CASE", node}); //mark as needing a potential jump
                return node;
            }

            break;
        case 5:
            //handle assignment instructions
            if(tokens[0].token_type == ID && tokens[1].token_type == EQUAL) {
                if(debugMode) cout << tokens[0].lexeme << " = " << tokens[2].lexeme << " " << tokens[3].lexeme << "\n";

                node->type = ASSIGN;
                node->assign_inst.left_hand_side_index = variableAddressMap[tokens[0].lexeme]; //addr of variable

                //handle left variable/num
                if(tokens[2].token_type == ID)
                    node->assign_inst.operand1_index = variableAddressMap[tokens[2].lexeme]; //addr of variable
                else if(tokens[2].token_type == NUM)
                    node->assign_inst.operand1_index = constantAddressMap[stoi(tokens[2].lexeme)]; //addr of num
                else
                    if(debugMode) cout << "TOKEN 2 NO CONSTANT OR VARIABLE\n";

                //handle arithmetic
                if(tokens[3].token_type == PLUS)
                    node->assign_inst.op = OPERATOR_PLUS;
                else if(tokens[3].token_type == MINUS)
                    node->assign_inst.op = OPERATOR_MINUS;
                else if(tokens[3].token_type == DIV)
                    node->assign_inst.op = OPERATOR_DIV;
                else if(tokens[3].token_type == MULT)
                    node->assign_inst.op = OPERATOR_MULT;
                else
                    if(debugMode) cout << "TOKEN 3 NO ARITHMETIC SIGN\n";

                //handle right variable/num
                if(tokens[4].token_type == ID)
                    node->assign_inst.operand2_index = variableAddressMap[tokens[4].lexeme]; // addr of variable
                else if(tokens[4].token_type == NUM)
                    node->assign_inst.operand2_index = constantAddressMap[stoi(tokens[4].lexeme)]; // addr of num
                else
                    if(debugMode) cout << "TOKEN 4 NO CONSTANT OR VARIABLE\n";

               return node;
            }
            //handle if and while statements
            else if(tokens[0].token_type == IF || tokens[0].token_type == WHILE) {
                if(debugMode) cout << tokens[0].lexeme << " " << tokens[1].lexeme << " > " << tokens[3].lexeme << "\n";

                node->type = CJMP;

                //handle left variable/num
                if(tokens[1].token_type == ID)
                    node->cjmp_inst.operand1_index = variableAddressMap[tokens[1].lexeme]; //addr of variable
                else if(tokens[1].token_type == NUM)
                    node->cjmp_inst.operand1_index = constantAddressMap[stoi(tokens[1].lexeme)]; //addr of num
                else
                    if(debugMode) cout << "TOKEN 1 NO CONSTANT OR VARIABLE\n";

                //handle comparison signs
                if(tokens[2].token_type == GREATER)
                    node->cjmp_inst.condition_op = CONDITION_GREATER;
                else if(tokens[2].token_type == LESS)
                    node->cjmp_inst.condition_op = CONDITION_LESS;
                else if(tokens[2].token_type == NOTEQUAL)
                    node->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
                else
                    cout << "TOKEN 2 NON CONDITION FOUND\n";

                //handle right variable/num
                if(tokens[3].token_type == ID)
                    node->cjmp_inst.operand2_index = variableAddressMap[tokens[3].lexeme]; //addr of variable
                else if(tokens[3].token_type == NUM)
                    node->cjmp_inst.operand2_index = constantAddressMap[stoi(tokens[3].lexeme)]; //addr of num
                else
                    if(debugMode) cout << "TOKEN 1 NO CONSTANT OR VARIABLE\n";
                
                //handle if statement vs while statement
                if(tokens[0].token_type == IF)
                    pendingJump.push({"IF", node}); //signal that jump may be needed
                else 
                    pendingJump.push({"WHILE", node}); //signal that jump may be needed

                return node;
            }

            break;
        default:
            if(debugMode) cout << "Something passed unhandled\n";
            break;
    }

    return NULL;
}

//PROGRAM INSTRUCTIONS
struct InstructionNode * parse_generate_intermediate_representation()
{
    //STEP 1: Read tokens
    Token token;
    struct InstructionNode *head = NULL;

    //STEP A: Reserve space for all constants and variables
    int tokenCount = 0; //track tokens processed so we can go back
    token = lexer.GetToken();

    while (token.token_type != END_OF_FILE) {
        tokenCount++;
        // Allocate space for the variable if it's new
        if (token.token_type == ID) {
            if (variableAddressMap.find(token.lexeme) == variableAddressMap.end()) {
                variableAddressMap[token.lexeme] = next_available;
                mem[next_available] = 0; // Initialize memory to 0
                next_available++;
            }
        } 
        // Allocate space for the constant if it's new
        else if (token.token_type == NUM) {
            int numValue = stoi(token.lexeme);
            if (constantAddressMap.find(numValue) == constantAddressMap.end()) {
                constantAddressMap[numValue] = next_available;
                mem[next_available] = numValue;
                next_available++;
            }
        } 
        // Allocate space for a variable to track if a switch statement has entered a case (used for default)
        else if(token.token_type == SWITCH) {
            switchMemory.push(next_available);
            mem[next_available] = 0; // Initialize memory to 0
            next_available++;
            // Also, allocate space for 0 & 1 if it's new
            if (constantAddressMap.find(0) == constantAddressMap.end()) {
                constantAddressMap[0] = next_available;
                mem[next_available] = 0;
                next_available++;
            }
            if (constantAddressMap.find(1) == constantAddressMap.end()) {
                constantAddressMap[1] = next_available;
                mem[next_available] = 1;
                next_available++;
            }
            
        }
        token = lexer.GetToken();
    }

    // After processing all tokens, unget them
    lexer.UngetToken(tokenCount);
    tokenCount = 0;

    //STEP B: Make InstructionNodes
    token = lexer.GetToken();

    //PROCESS EACH LINE AND CREATE INSTRUCTIONS
    struct InstructionNode* lastInstruct = NULL; 
    while(token.token_type != END_OF_FILE) {
        struct InstructionNode* currInstruct = processLine(token, tokenCount);

        if(currInstruct != NULL) {
            if(head == NULL) 
                head = currInstruct;

            //IF JUMP NEEDS TO BE LINKED
            if(jumpNextInstruction > 0) {
                pair<string, InstructionNode*> topElement = pendingJump.top();

                //IF ITS A WHILE, WE NEED TO JUMP BACK TO CONDITION
                //ALSO, WE NEED TO POINT TO NOOP
                if(topElement.first == "WHILE") {
                    struct InstructionNode* whileInstruct = topElement.second;
                    struct InstructionNode* newJmp = new InstructionNode;
                    newJmp->type = JMP;
                    newJmp->jmp_inst.target = whileInstruct;
                    newJmp->next = currInstruct;

                    lastInstruct->next = newJmp;
                    lastInstruct = newJmp;
                } 
                //IF ITS A CASE, WE NEED TO MARK SWITCH AS ENTERED
                //ALSO, WE NEED TO POINT TO NOOP
                else if(topElement.first == "CASE") {
                    struct InstructionNode* ifInstruct = topElement.second;

                    //next and jump are reversed to make intended functionality
                    ifInstruct->cjmp_inst.target = ifInstruct->next; //make case jump to next instruct
                    ifInstruct->next = currInstruct; //make next instruct noop

                    //add 1 to increment variable
                    struct InstructionNode* incrementVariable = new InstructionNode;
                    incrementVariable->type = ASSIGN;
                    incrementVariable->assign_inst.left_hand_side_index = switchMemory.top();
                    incrementVariable->assign_inst.op = OPERATOR_PLUS;
                    incrementVariable->assign_inst.operand1_index = switchMemory.top();
                    incrementVariable->assign_inst.operand2_index = constantAddressMap[1];
                    
                    lastInstruct->next = incrementVariable; //last instruct goes to increment
                    lastInstruct = incrementVariable; //last instruct is now incrementVariable so it gets linked
                }
                //IF ITS A SWITCH, WE NEED TO POP TO MARK AS DONE
                else if(topElement.first == "SWITCH") {
                    if(debugMode) cout << "Switch Popped\n";
                    switchMemory.pop();
                } 
                //IF ITS A DEFAULT, 
                else if(topElement.first == "DEFAULT") {
                    struct InstructionNode* defaultInstruct = topElement.second;
                    
                    //swap next and jump to get intended functionality
                    defaultInstruct->cjmp_inst.target = defaultInstruct->next; //jump to next
                    defaultInstruct->next = currInstruct; //next to end
                }
            }

            //link last struct to next
            if(lastInstruct != NULL) 
                lastInstruct->next = currInstruct;
 
            //IF JUMP NEEDS TO BE LINKED
            if(jumpNextInstruction > 0) {
                pair<string, InstructionNode*> topElement = pendingJump.top();
                struct InstructionNode* jumpInstruct = topElement.second;

                if(topElement.first != "CASE" && topElement.first != "DEFAULT" && topElement.first != "SWITCH")
                    jumpInstruct->cjmp_inst.target = currInstruct;

                pendingJump.pop();
                jumpNextInstruction--;  
            }

            //prepare for next instruct
            lastInstruct = currInstruct;
        }

        token = lexer.GetToken();
    }

    lexer.UngetToken(tokenCount);
    tokenCount = 0;

    //STEP C: PUSH ALL INPUTS BACKS
    stack<Scope> braceStack;
    token = lexer.GetToken();
    bool isEndOfProgram = false;

    //WE WILL TRACK ALL BRACKETS TO CREATE SCOPES
    //ONCE WE FIND SCOPE, WE ASSUME THE INPUTS ARE EVERYTHING AFTER SO
    while(token.token_type != END_OF_FILE) {
        tokenCount++;

        //TRACK the brackets / braces
        if(token.token_type == LBRAC || token.token_type == LBRACE)
            braceStack.push(Scope{NULL, NULL});
        else if(token.token_type == RBRAC || token.token_type == RBRACE) {
            if (!braceStack.empty()) {
                Scope currentScope = braceStack.top();
                braceStack.pop();
                // Linking instructions within the block
                if (!braceStack.empty()) {
                    // Linking to the enclosing scope
                    Scope &enclosingScope = braceStack.top();
                    if (currentScope.start != NULL) {
                        if (enclosingScope.end != NULL) 
                            enclosingScope.end->next = currentScope.start;
                        else 
                            enclosingScope.start = currentScope.start;
                        enclosingScope.end = currentScope.end;
                    }
                } else isEndOfProgram = true;
            }
        }

        //IF END OF PROGRAM
        if(isEndOfProgram) {
            if(token.token_type == NUM) 
                inputs.push_back(stoi(token.lexeme));
        }

        token = lexer.GetToken();
    }

    lexer.UngetToken(tokenCount);
    tokenCount = 0;

    //STEP D: RETURN INSTRUCTION 1
    return head;
}