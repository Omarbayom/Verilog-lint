#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <regex>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <cmath>

using namespace std;

struct VerilogViolation {
    string moduleName;
    int lineNumber;
    string violationType;
    string expression;
};

struct VariableInfo {
    string name;
    int size;
    string Type;
};

class VerilogParser {
public:
    VerilogParser(const string& filePath) : filePath(filePath) {
        if (filePath.empty()) {
            cerr << "Error: File path is empty." << endl;
            exit(EXIT_FAILURE);
        }
    }
    int lov = 0;
    vector<VerilogViolation> violations1;
    vector<VerilogViolation> parse() {
        ifstream inFile(filePath);
        vector<VerilogViolation> violations;
        
        if (inFile.is_open()) {
            string line;
            int lineNumber = 0;

            while (getline(inFile, line)) {
                ++lineNumber;
                string lineWithoutComments = removeComments(line);
                if (line.find("always") != string::npos) {
                    lov = lineNumber;
                }
                extractViolations(lineWithoutComments, lineNumber, violations);
            }

            // Check for unreachable blocks after parsing the entire file
            violations = violations;

            inFile.close();
        }
        else {
            cerr << "Unable to open file: " << filePath << endl;
        }

        return violations;
    }

   

private:
    string filePath;
    string currentModuleName;
    int l;
    vector<VariableInfo> moduleVariables;


  
   

    // Add a new function to extract variable information from the module
    void extractModuleVariables(const string& line) {
        regex variablePattern(R"((reg|wire|integer|input\s*reg|input\s*wire|output\s*reg|output\s*wire)\s*((?:\[\d+:\d+\])?)\s*(\w+)(?:\s*,\s*(\w+))*)");
        smatch match;

        if (regex_search(line, match, variablePattern)) {
            // Process each matched variable
            do {
                string type = match[1].str();
                string size = match[2].str();
                string name1 = match[3].str();
                string name2 = match[4].str();

                // Calculate size based on type and provided size (if any)
                int calculatedSize = calculateVariableSize(type, size);

                // Add variables to moduleVariables vector
                moduleVariables.push_back({ name1, calculatedSize,type });
                if (!name2.empty()) {
                    moduleVariables.push_back({ name2, calculatedSize,type });
                   
                }

            } while (regex_search(match.suffix().first, match.suffix().second, match, variablePattern));
        }
    }

    // Add a new function to calculate the size of a variable
    int calculateVariableSize(const string& type, const string& size) {
        // If size is provided, use it; otherwise, default to 1
        if (!size.empty()) {
            return stoi(size.substr(1, size.size() - 2)) + 1;
        }

        // If 'int' or 'integer', default size to 32
        if ( type == "integer") {
            return 32;
        }

        // Default size to 1 if no size is provided
        return 1;
    }

    void extractViolations(const string& line, int lineNumber, vector<VerilogViolation>& violations) {
        extractModuleName(line);
        extractModuleVariables(line);
        l = lineNumber;
        if (containsArithmeticOverflow(line)) {
            violations.push_back({ currentModuleName, lineNumber, "Arithmetic Overflow", line });
        }

        if (isUnreachableBlock(line)) {
            violations.push_back({ currentModuleName, lineNumber, "Unreachable Block", line });
        }

        if (containsUninitializedRegister(line)) {
            violations.push_back({ currentModuleName, lineNumber, "Uninitialized Register", line });
        }
        if (isnonfullcase(line)) {
            violations.push_back({ currentModuleName, lineNumber, "Non Full Case", line });
        }
        if (isNonParallelCase(line)) {
            violations.push_back({ currentModuleName, lineNumber, "Non Parallel Case", line });
        }
        if (containsMultiDrivenNets(line)) {
            violations.push_back({ currentModuleName, lineNumber, "Multi-Driven", line });
        }
        if (containsLatchInference(line)) {
            violations.push_back({ currentModuleName, lineNumber, "Latch Inference", line });
        }
    }

    bool containsLatchInference(const string& line) {
        size_t assignPos = line.find("=");
        size_t always = line.find("always");
        // Check if there's an assignment operator in the line
        if (always != string::npos) {
            // Check if the assignment is conditional (e.g., inside if or case)
            if (latch(line)) {
                return true;
            }
        }

        return false;
    }
    bool latch(const string& line) {
        size_t ifPos = line.find("if");

        ifstream fileStream("C:/Users/dell/Desktop/test/test.v");
        if (!fileStream.is_open()) {
            cerr << "Error: Unable to open file." << endl;
            return false;
        }
        int currentLineNumber = 0;
        string savedLine;

        while (getline(fileStream, savedLine)) {
            currentLineNumber++;

            if (currentLineNumber == l) {
                break; // Found the starting line, exit the loop
            }
        }

        while (getline(fileStream, savedLine)) {
            currentLineNumber++;
            if (savedLine.find("if") != string::npos) {
               
                if (!hasElse(savedLine)) {
                    fileStream.close();                  
                    return true;
                }// Found the starting line, exit the loop
            }

            size_t endcasePos = savedLine.find("endcase");
            size_t casePos = savedLine.find("case");

            if (casePos != string::npos && endcasePos == string::npos) {
                if (isnonfullcase(savedLine)) {
                    fileStream.close();
                    return true;
                }
            }
            if (savedLine.find("endmodule") != string::npos) {
                fileStream.close();
                return false;
            }
            if (savedLine.find("always") != string::npos) {
                fileStream.close();
                return false;
            }
        }

        fileStream.close(); // Close the file stream

        return false;
    }
    bool containsMultiDrivenNets(const string& line) {
        // Assuming multi-driven nets are assigned in the form: net = a + b;
        regex multiDrivenNetsPattern(R"(\b(\w+)\s*=\s*)");
       

        smatch match;
        string multiDrivenNetVariable;
        if (regex_search(line, match, multiDrivenNetsPattern)) {
            if (match.size() >= 1) {
                multiDrivenNetVariable = match[1].str();
                if (multi(line, multiDrivenNetVariable)) {

                    return true;
                }
            }
            
        }

        return false;
    }
    bool multi(const string& Line, const string& Vname) {
                size_t always = Line.find("always");
                size_t initial = Line.find("initial");
                size_t endmodule = Line.find("endmodule");
            // Open a file stream for reading
            ifstream fileStream("C:/Users/dell/Desktop/test/test.v");
            if (!fileStream.is_open()) {
                cerr << "Error: Unable to open file." << endl;
                return false;
            }

            // Seek to the line where "case" is found
            int currentLineNumber = 0;
            bool tas = false;
            string savedLine;
            while (getline(fileStream, savedLine)) {
                currentLineNumber++;
                if (lov==currentLineNumber) {
                    break;
                }
            }
            
            while (getline(fileStream, savedLine)) {
                savedLine = removeComments(savedLine);
                size_t always = savedLine.find("always");
                size_t initial = savedLine.find("initial");
                size_t endmodule = savedLine.find("endmodule");
                
                currentLineNumber++;
               
                if (always != string::npos || initial != string::npos) {
                    
                    break;
                }
                if (endmodule != string::npos) {
                   
                    return false;
                }
            }

            // Iterate until "endcase" is found

            while (getline(fileStream, savedLine)) {
                ++currentLineNumber;
                regex multiDrivenNetsPattern(R"(\b(\w+)\s*=\s*)");
                regex integerPattern(R"(^\s*(\d+'[bB]\d+):\s*)");
                regex patternq(R"(\d+'b(\d*\?|\?\d+|\d+\?\d+))"); 
                size_t defaultSpacePos = savedLine.find("default :");
                size_t defaultPos = savedLine.find("default:");
               
                smatch match;
                string multiDrivenNetVariable;
                
                if ((regex_search(savedLine,match, integerPattern))|| defaultSpacePos != string::npos || defaultPos != string::npos) {
                    continue;
                }
                if ( regex_search(savedLine, match, patternq)) {
                    continue;
                }

                size_t endmodule = savedLine.find("endmodule");
                // Output the result
                if (regex_search(savedLine, match, multiDrivenNetsPattern)) {
                    if (match.size() >= 1) {
                        multiDrivenNetVariable = match[1].str();

                        if (multiDrivenNetVariable == Vname) {
                            return true;
                        }
                    }

                }

                if (endmodule != string::npos) {
                    fileStream.close();
                    return false;
                }

            }

            fileStream.close();
       

        return false;
    }

    bool isNonParallelCase(const string& line) {
        size_t casePos = line.find("case");
        size_t endCasePos = line.find("endcase");

        if (casePos != string::npos && endCasePos == string::npos) {
            // Check if there is a keyword like "if" or "else" inside the case block
            if (parallel(line)) {
                return true;
            }
        }

        return false;
    }
    bool parallel(const string& caseLine) {
        size_t casePos = caseLine.find("case");
        if (casePos != string::npos) {
            // Open a file stream for reading
            ifstream fileStream("C:/Users/dell/Desktop/test/test.v");
            if (!fileStream.is_open()) {
                cerr << "Error: Unable to open file." << endl;
                return false;
            }

            // Seek to the line where "case" is found
            int currentLineNumber = 0;
            string savedLine;
            while (getline(fileStream, savedLine)) {
                currentLineNumber++;
                if (l == currentLineNumber) {
                    break;
                }
            }

            // Iterate until "endcase" is found

                while (getline(fileStream, savedLine)) {
                    ++currentLineNumber;

                    
                    bool isValid = checkPatternP(savedLine);
                   
                    size_t endCasePos = savedLine.find("endcase");
                    // Output the result
                    if (isValid) {
                        return true;
                    }
                   
                    if (endCasePos != string::npos) {
                        fileStream.close();
                        return false;
                    }
                    
                }


            fileStream.close();
        }

        return false;
    }
    bool checkPatternP(const string& input) {
        
        // Define the regular expression pattern
        regex pattern(R"(\d+'b(\d*\?|\?\d+|\d+\?\d+))");

        // Check if the input string contains the pattern
        return regex_search(input, pattern);
    }
    bool hasDefaultInsideCaseBlock(const string& caseLine) {
        size_t casePos = caseLine.find("case");
        size_t defaultPos;
        if (casePos != string::npos) {
            // Open a file stream for reading
            ifstream fileStream("C:/Users/dell/Desktop/test/test.v");
            if (!fileStream.is_open()) {
                cerr << "Error: Unable to open file." << endl;
                return false;
            }

            // Seek to the line where "case" is found
            int currentLineNumber = 0;
            string savedLine;
            while (getline(fileStream, savedLine)) {
                currentLineNumber++;
                if (l == currentLineNumber) {
                    break;
                }
            }

            // Iterate until "endcase" is found
            while (getline(fileStream, savedLine)) {
              
                size_t modulePos = savedLine.find("module");
                if (modulePos != string::npos) {
                    continue;
                }
                currentLineNumber++; 

                // Check if "endcase" or "default:" or "default :" is found
                size_t endCasePos = savedLine.find("endcase");
                size_t defaultPos = savedLine.find("default:");
                size_t defaultSpacePos = savedLine.find("default :");

                if ( defaultPos != string::npos || defaultSpacePos != string::npos) {
                    // Found "endcase" or "default:" or "default :" inside the "case" block, return true
                    fileStream.close();
                    return true;
                }
                if (endCasePos != string::npos) {
                    fileStream.close();
                    return false;
                }
               
                
                
            }

            fileStream.close();
        }

        return false;
    }

    void extractModuleName(const string& line) {
        regex modulePattern(R"(module\s+(\w+)\s*\()");
        smatch match;

        if (regex_search(line, match, modulePattern)) {
            currentModuleName = match[1].str();
            moduleVariables.clear();
        }
    }

    bool containsArithmeticOverflow(const string& line) {
        regex operandPattern(R"(\d+['bB]\d+|\b[a-zA-Z_]\w*\b)");

        for (const auto& keyword : controlKeywords) {
            if (line.find(keyword) != string::npos &&
                (line.find("<") != string::npos || line.find(">") != string::npos || line.find("==") != string::npos)) {
                return false;
            }
        }

        size_t forPos = line.find("for");
        if (forPos != string::npos) {
            size_t openParenPos = line.find("(", forPos);
            size_t semiColon1Pos = line.find(";", openParenPos);
            size_t semiColon2Pos = line.find(";", semiColon1Pos);

            if (semiColon1Pos != string::npos && semiColon2Pos != string::npos) {
                string init = line.substr(openParenPos + 1, semiColon1Pos - openParenPos - 1);
                string condition = line.substr(semiColon1Pos + 1, semiColon2Pos - semiColon1Pos - 1);
                string update = line.substr(semiColon2Pos + 1);

                if (checkArithmeticOverflow(init, "") ||
                    checkArithmeticOverflow(condition, "") ||
                    checkArithmeticOverflow(update, "")) {
                    return true;
                }
            }
        }

        sregex_iterator it(line.begin(), line.end(), operandPattern);
        sregex_iterator end;

        if (it != end) {
            string leftOperand = it->str();
            ++it;
            if (it != end) {
                string rightOperand = it->str();

                if (isArithmeticOperation(line) && checkArithmeticOverflow(leftOperand, rightOperand)) {
                    
                    return true;
                }
            }
        }

        return false;
  
    }

    bool isArithmeticOperation(const string& line) {
        size_t plusPos = line.find('+');
        size_t minusPos = line.find('-');
        size_t multiplyPos = line.find('*');
        size_t dividePos = line.find('/');
        size_t equalsPos = line.find('=');

        // Check for the basic arithmetic operators and ensure no -> or // is present
        return (plusPos != string::npos || minusPos != string::npos || multiplyPos != string::npos || dividePos != string::npos) &&
            (line.find("->") == string::npos && line.find("//") == string::npos) &&
            (equalsPos != string::npos && line.find('=', equalsPos + 1) == string::npos);
    }
    double powerOfTwo(int exponent) {
        return pow(2, exponent);
    }

    bool isnonfullcase(const string& line) {
        size_t endcasePos = line.find("endcase");
        size_t casePos = line.find("case");

        if (casePos != string::npos && endcasePos == string::npos) {
            // Extract values inside parentheses in the case line
            size_t startPos = line.find("(");
            size_t endPos = line.find(")");
            if (hasDefaultInsideCaseBlock(line)) {
                    return false;
            }

            if (startPos != string::npos && endPos != string::npos && endPos > startPos) {
                string values = line.substr(startPos + 1, endPos - startPos - 1);

                // Search for the size of extracted values in moduleVariables
                int size = -1; // Default value if not found
                for (const VariableInfo& var : moduleVariables) {
                    if (values == var.name) {
                        size = var.size;
                        break;
                    }
                }
                if (size != -1) {
                    if (ishascoloms(line) == powerOfTwo(size)) {

                        return false;
                    }
                }

                // Now 'size' contains the size of the extracted values
                // You can use 'size' as needed in your code


             
            } 
            return true;// Check for default inside the case block
                
        }

        return false;
    }

    int ishascoloms(const string& caseLine) {
        size_t casePos = caseLine.find("case");
        if (casePos != string::npos) {
            // Open a file stream for reading
            ifstream fileStream("C:/Users/dell/Desktop/test/test.v");
            if (!fileStream.is_open()) {
                cerr << "Error: Unable to open file." << endl;
                return 0;
            }

            // Seek to the line where "case" is found
            int currentLineNumber = 0;
            string savedLine;
            while (getline(fileStream, savedLine)) {
                currentLineNumber++;
                if (l == currentLineNumber) {
                    break;
                }
            }

            // Count of colons found
            int colonCount = 0;

            // Iterate until "endcase" is found
            while (getline(fileStream, savedLine)) {
                savedLine = removeComments(savedLine);
                size_t modulePos = savedLine.find("module");
                if (modulePos != string::npos) {
                   
                    continue;
                }
                currentLineNumber++;

                // Check if ":" is found
                size_t colonPos = savedLine.find(":");
                if (colonPos != string::npos) {
                    
                    // Increment the count for each colon found
                    colonCount++;
                }

                // Check if "endcase" is found
                size_t endCasePos = savedLine.find("endcase");
                if (endCasePos != string::npos) {
                    // Found "endcase," return the count of colons and close the file
                    fileStream.close();
                    return  colonCount;
                }
            }

            fileStream.close();
        }

        // Return 0 if "case" is not found or there is an issue with file opening
        return 0;
    }

    bool isUnreachableBlock(const string& line) {
        size_t ifPos = line.find("if");
        size_t casePos = line.find("case");
        size_t endCasePos = line.find("endcase");

        if (ifPos != string::npos) {
            size_t openParenPos = line.find("(", ifPos);
            size_t closeParenPos = line.find(")", openParenPos);

            if (ifPos != string::npos && openParenPos != string::npos && closeParenPos != string::npos) {
                string condition = line.substr(openParenPos + 1, closeParenPos - openParenPos - 1);
                if (iszero(condition) || (checkons(condition, line) && !containsComparisonOperator(condition))) {
                    return true;
                }
            }
        }

        if (casePos != string::npos && endCasePos == string::npos) {
            size_t openParenPos = line.find("(", casePos);
            size_t closeParenPos = line.find(")", openParenPos);
            string contentBetweenParentheses;
            

            if (openParenPos != string::npos && closeParenPos != string::npos) {
                contentBetweenParentheses = line.substr(openParenPos + 1, closeParenPos - openParenPos - 1);

               // Search for contentBetweenParentheses in the variableName vector     
            }
            int woh = numberofeq(contentBetweenParentheses);
            int size = getVariableSize(contentBetweenParentheses);
           
            
            if (!isnonfullcase(line) && (powerOfTwo(size) > woh)&&(woh!=ishascoloms(line))) {
               
                return true;
            }
            // Use 'woh' as needed in your code
        }

        return false;
    }
    int numberofeq(const string& name) {
        ifstream fileStream("C:/Users/dell/Desktop/test/test.v");
        if (!fileStream.is_open()) {
            cerr << "Error: Unable to open file." << endl;
            return -1;
        }

        // Seek to the desired line in the file
        int currentLineNumber = 0;
        string savedLine;
        while (getline(fileStream, savedLine)) {
            currentLineNumber++;
            savedLine = removeComments(savedLine);
            if (savedLine.find(currentModuleName) != string::npos) {
                break; // Found the starting line, exit the loop
            }
        }

        int count = 0;
        while (getline(fileStream, savedLine)) {
            currentLineNumber++;
            savedLine = removeComments(savedLine);
            // Check if the line contains "name =";
            size_t found = savedLine.find(name + " =");
            if (savedLine.find(name + " =") != string::npos|| savedLine.find(name + "=") != string::npos) {
                // Increment the counter for each occurrence
                count++;
            }

            // Check if the line contains the end of the module or any other condition to stop counting
            // Replace this condition based on your specific requirements
            if (currentLineNumber==l) {
                return count;
            }
        }

        // Close the file stream
        fileStream.close();

        // Return the count of occurrences
        return count;

    }
    bool containsUninitializedRegister(const string& line) {
       
            regex uninitializedRegisterPattern(R"(reg\s+(\[[^\]]*\])?\s*(\w+)\s*(,\s*(\w+)\s*)*;)");

            smatch match;
            string remainingLine = line;

            while (regex_search(remainingLine, match, uninitializedRegisterPattern)) {
                string registerWidth = match[1].str();  // New line to capture width
                string registerName = match[2].str();
                size_t equ = line.find("=");
                size_t notequ = line.find("==");// Modified line to capture name
                // Check if the register is uninitialized (e.g., assigned a value later in the code)
                for (const auto& variableInfo : moduleVariables) {
                    if (variableInfo.name == registerName) {
                        if (variableInfo.Type.find("input") != string::npos) {
                            return false;
                        }
                    }
                }
                if (equ != string::npos && notequ == string::npos) {
                   
                    return false;
                }
                else if (inreg(registerName)) {
                    return false;
                }
                else {
                    return true;
                }

                // Move to the remaining part of the line after the last match
                remainingLine = match.suffix();
            }
        

        return false;
    }
    bool inreg(const string& line) {
       
            // Open a file stream for reading
            ifstream fileStream("C:/Users/dell/Desktop/test/test.v");
            if (!fileStream.is_open()) {
                cerr << "Error: Unable to open file." << endl;
                return false;
            }

            // Seek to the desired line in the file
            int currentLineNumber = 0;
            string savedLine;
            while (getline(fileStream, savedLine)) {
                currentLineNumber++;

                if (currentLineNumber == l) {
                    break; // Found the starting line, exit the loop
                }
            }

            // Iterate until "end" is found
            while (getline(fileStream, savedLine)) {
                savedLine = removeComments(savedLine);
                // Check if "end" is found
                size_t variable = savedLine.find(line+" ");
                size_t notequ = savedLine.find("==");
                size_t equ = savedLine.find(line+" =");
                size_t equ2 = savedLine.find(line + " <=");
                size_t endmodule = savedLine.find("endmodule");
                if (variable != string::npos) {
                    if ((equ != string::npos && notequ == string::npos)|| equ2 != string::npos) {
                        return true;
                    }
                    
                }
                if (endmodule != string::npos) {
                        return false;
                    }
            }

            fileStream.close(); // Close the file stream
        

        return false;
    }
    bool checkons(const string& condition, const string& line) {
        
       
       if (iso(condition)) {
           if (hasElse1(line)) {
               return true;
           }
       }
       return false;
    }
    bool iso(const string& condition) {
        string conditionWithoutSpaces = regex_replace(condition, regex("\\s+"), "");

        return conditionWithoutSpaces.find("1'b1") != string::npos ||
            conditionWithoutSpaces.find("1'h1") != string::npos ||
            conditionWithoutSpaces.find("1") != string::npos ||
            conditionWithoutSpaces.find("1'b1") != string::npos ||
            conditionWithoutSpaces.find("if(1)") != string::npos ||
            conditionWithoutSpaces.find("if(1'b1)") != string::npos ||
            // Add more conditions as needed
            false;  // Default case
    }
   
    bool iszero(const string& condition) {
        return condition.find("1'b0") != string::npos || 
            condition.find("1'h0") != string::npos || 
            condition.find("0") != string::npos || 
            condition.find("0'b0") != string::npos || 
            condition.find("1'b0") != string::npos || 
            condition.find("if (0)") != string::npos ||
            condition.find("if (1'b0)") != string::npos || 
            condition.find("if(1'b0)") != string::npos ||
            // Add more conditions as needed
            false;  // Default case
    }

    bool containsComparisonOperator(const string& condition) {
        const vector<string> comparisonOperators = { "<", ">", "==", "<=", ">=", "!=" };

        for (const auto& op : comparisonOperators) {
            if (condition.find(op) != string::npos) {
                return true;
            }
        }

        return false;
    }
    
    bool checkArithmeticOverflow(const string& lhs, const string& rhs) {
        int lhsSize = getVariableSize(lhs);
        int rhsSize = getVariableSize(rhs);

        return lhsSize <= rhsSize;
    }

    int getVariableSize(const string& variable) {
        for (const auto& variableInfo : moduleVariables) {
            if (variableInfo.name == variable) {
                return variableInfo.size;
            }
        }
        return 1; // Default size if not found
    }

    bool hasElse(const string& line) {
        size_t ifPos = line.find("if");
        size_t beginPos = line.find("begin");

        if (beginPos != string::npos ) {

            // Open a file stream for reading
            ifstream fileStream("C:/Users/dell/Desktop/test/test.v");
            if (!fileStream.is_open()) {
                cerr << "Error: Unable to open file." << endl;
                return false;
            }

            // Seek to the desired line in the file
            int currentLineNumber = 0;
            string savedLine;
            while (getline(fileStream, savedLine)) {
                currentLineNumber++;

                if (savedLine == line) {
                    break; // Found the starting line, exit the loop
                }
            }

            // Iterate until "end" is found
            while (getline(fileStream, savedLine)) {
                savedLine = removeComments(savedLine);
                // Check if "end" is found
                size_t endPos = savedLine.find("end");
                size_t endPosp = savedLine.find("endmodule");
                if (endPos != string::npos) {
                    size_t elsePos = savedLine.find("else", endPos);
                   
                    if (elsePos != string::npos) { 
                        // "else" is found after the first occurrence of "end"
                        fileStream.close(); // Close the file stream
                        return true;
                    }
                    else {
                        // Continue searching for "else" in the next line
                        size_t posAfterEnd = endPos + 3; // Assuming "end" is 3 characters
                    if (savedLine.substr(posAfterEnd).find_first_not_of(" \t\r\n") == string::npos) {
                        // Everything after "end" is space or newline

                        // Check for "else" in the next line
                        getline(fileStream, savedLine);
                        size_t elsePos = savedLine.find("else");
                        if (elsePos != string::npos) {
                            // "else" is found after the first occurrence of "end"
                            fileStream.close(); // Close the file stream
                            return true;
                        }
                    }
                    else {
                        // Continue searching for "else" in the next line
                        continue;
                    }
                    }
                    // Check if everything after "end" is space or newline
                    
                }
                else if (endPosp != string::npos) {
                    return false;
                }
            }

            fileStream.close(); // Close the file stream
        }

        return false;
    }

    bool hasElse1(const string& line) {
        size_t ifPos = line.find("if");
        size_t beginPos = line.find("begin");

        if (beginPos != string::npos) {

            // Open a file stream for reading
            ifstream fileStream("C:/Users/dell/Desktop/test/test.v");
            if (!fileStream.is_open()) {
                cerr << "Error: Unable to open file." << endl;
                return false;
            }

            // Seek to the desired line in the file
            int currentLineNumber = 0;
            string savedLine;
            while (getline(fileStream, savedLine)) {
                currentLineNumber++;

                if (currentLineNumber == l) {
                    break; // Found the starting line, exit the loop
                }
            }

            // Iterate until "end" is found
            while (getline(fileStream, savedLine)) {
                savedLine = removeComments(savedLine);
                // Check if "end" is found
                size_t endPos = savedLine.find("end");
                size_t endPosp = savedLine.find("endmodule");
                if (endPos != string::npos) {
                    size_t elsePos = savedLine.find("else", endPos);

                    if (elsePos != string::npos) {
                        // "else" is found after the first occurrence of "end"
                        fileStream.close(); // Close the file stream
                        return true;
                    }
                    else {
                        // Continue searching for "else" in the next line
                        size_t posAfterEnd = endPos + 3; // Assuming "end" is 3 characters
                        if (savedLine.substr(posAfterEnd).find_first_not_of(" \t\r\n") == string::npos) {
                            // Everything after "end" is space or newline

                            // Check for "else" in the next line
                            getline(fileStream, savedLine);
                            size_t elsePos = savedLine.find("else");
                            if (elsePos != string::npos) {
                                // "else" is found after the first occurrence of "end"
                                fileStream.close(); // Close the file stream
                                return true;
                            }
                        }
                        else {
                            // Continue searching for "else" in the next line
                            continue;
                        }
                    }
                    // Check if everything after "end" is space or newline

                }
                else if (endPosp != string::npos) {
                    return false;
                }
            }

            fileStream.close(); // Close the file stream
        }

        return false;
    }
   


   

    string removeComments(const string& line) {
        regex commentPattern(R"((\/\/[^\n]*)|(/\*.*?\*/))");
        return regex_replace(line, commentPattern, "");
    }

    const vector<string> controlKeywords = { "if", "else", "for", "while", "do", "case", "begin", "end" };
};


class ReportGenerator {
public:
    void generateReport(const vector<VerilogViolation>& violations) {
        ofstream outFile("verilog_checker_report.txt");
        if (outFile.is_open()) {
            outFile << "Verilog Checker Report:" << endl;

            outFile << "\nArithmetic Overflow Warnings:" << endl;
            for (const auto& violation : violations) {
                if (violation.violationType == "Arithmetic Overflow") {
                    outFile << "Module: " << violation.moduleName << ", Line: " << violation.lineNumber
                            << ", Expression: " << violation.expression << endl;
                }
            }

            outFile << "\nUnreachable Block Warnings:" << endl;
            for (const auto& violation : violations) {
                if (violation.violationType == "Unreachable Block"&& violation.moduleName=="UnreachableBlocks") {
                    outFile << "Module: " << violation.moduleName << ", Line: " << violation.lineNumber
                        << ", Expression: " << violation.expression << endl;
                }
            }

            outFile << "\nUninitialized Register Warnings:" << endl;
            for (const auto& violation : violations) {
                if (violation.violationType == "Uninitialized Register") {
                    outFile << "Module: " << violation.moduleName << ", Line: " << violation.lineNumber
                        << ", Expression: " << violation.expression << endl;
                }
            }
            outFile << "\nNon Full Case Warnings:" << endl;
            for (const auto& violation : violations) {
                if (violation.violationType == "Non Full Case") {
                    outFile << "Module: " << violation.moduleName << ", Line: " << violation.lineNumber
                        << ", Expression: " << violation.expression << endl;
                }
            }

            outFile << "\nNon Parallel Case Warnings:" << endl;
            for (const auto& violation : violations) {
                if (violation.violationType == "Non Parallel Case") {
                    outFile << "Module: " << violation.moduleName << ", Line: " << violation.lineNumber
                        << ", Expression: " << violation.expression << endl;
                }
            }

            outFile << "\nMulti-Driven Register Warnings:" << endl;
            for (const auto& violation : violations) {
                if (violation.violationType == "Multi-Driven") {
                    outFile << "Module: " << violation.moduleName << ", Line: " << violation.lineNumber
                        << ", Expression: " << violation.expression << endl;
                }
            }

            outFile << "\nLatch Inference Warnings:" << endl;
            for (const auto& violation : violations) {
                if (violation.violationType == "Latch Inference") {
                    outFile << "Module: " << violation.moduleName << ", Line: " << violation.lineNumber
                        << ", Expression: " << violation.expression << endl;
                }
            }

            outFile.close();
            cout << "Report generated successfully. Check the file: verilog_checker_report.txt" << endl;
        }
        else {
            cerr << "Unable to open report file for writing." << endl;
        }
    }


};

int main() {
    string verilogFilePath = "C:/Users/dell/Desktop/test/test.v";

    VerilogParser parser(verilogFilePath);
    vector<VerilogViolation> violations = parser.parse();

    ReportGenerator reportGenerator;
    reportGenerator.generateReport(violations);

    return 0;
}
