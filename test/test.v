module UnreachableBlocks(data_out);    //for testing Unreachable Blocks
    output reg data_out;
    reg reach;
    reg [1:0] x;
    reg [1:0] T;
    wire state;
    wire a, b;
    wire c;
    initial 
    begin
        reach = 1'b1;               //initializing reach with '1'
    end

        always @(*)
        begin
            if (a)       x = 2b'00;     //these three cases will guarantee that 'x' will never be equals 2b'11
            else if (b)  x = 2b'01;
            else         x = 2b'10;

            if (a)       T = 2b'00;     //these four cases will guarantee that 'T' may be any value from 0-3
            else if (b)  T = 2b'01;
            else if(c)   T = 2b'11;
            else         T = 2b'10;
        end

    always @(*) 
        begin
            if (1'b1) begin             //expected to fire unreachable block, since  'else' will never be entered
                    data_out = 1'b1;
                end else 
                begin
                    data_out = 1'b0;    
                end

            if (1'b0) begin            //expected to fire unreachable block, since  'else' will always be entered
                    data_out = 1'b1;
                end else 
                begin
                    data_out = 1'b0;
                end

            if (1'b1) begin             //expected not to fire unreachableblock
                    data_out = 1'b1;
                end 

            if (1'b0) begin              //expected  to fire unreachableblock
                    data_out = 1'b1;
                end 
    
            case(x)
                2'b00:   data_out = 1'b0;   //expected not to fire unreachableblock, since all potenial values for "case(x)" exist
                2'b01:   data_out = 1'b1;
                2'b10:  data_out = 1'b1;
                endcase

            case(x)                         //expected  to fire unreachableblock, because case(2'b11) will never exist
                2'b00:   data_out = 1'b0;   
                2'b01:   data_out = 1'b1;
                2'b10:  data_out = 1'b1;
                2'b11:  data_out = 1'b1;
                endcase

            case(T)
                2'b00:   data_out = 1'b0;   //expected not to fire unreachableblock, since all potenial values for "case(T)" exist
                2'b01:   data_out = 1'b1;
                2'b10:  data_out = 1'b1;
                2'b11:  data_out = 1'b1;
                endcase
        end
endmodule

module UninitializedRegister(data_out);
    reg data; 
    reg data2;
    output  data_out;
    assign data_out = data;             //expected to fire UninitializedRegister, since data in not initialized 
   assign data2 = data_out;             //expected not to fire UninitializedRegister, since data_out is  initialized 
endmodule

module InferringLatches(enable, Data, out);
    input wire enable, Data
    reg [1:0] x, y;
    output reg out;

    always @(enable) 
    begin
        if (enable)         //expectedto fire InferringLatches, since else condition is not handled
        begin
            out = Data;
        end
 always @(enable) 
    begin
        case(x)                     //expected to fire InferringLatches, since not all potential values for case(x) are handled
            2'b00: y = 1'b00;
            2'b01: y = 1'b01;
            // Missing cases for '10' & '11'
        endcase
    end
    always @(enable) 
    begin
        case(x)                 //expected not to fire InferringLatches, since  all potential values for case(x) are handled
            2'b00: y = 1'b00;
            2'b01: y = 1'b01;
            default: y = 1'b0;
        endcase
    end
    always @(enable) 
    begin
        case(x)                  //expected not to fire InferringLatches, since  all potential values for case(x) are handled
            2'b00: y = 1'b00;
            2'b01: y = 1'b01;
            2'b10: y = 1'b01;
            2'b11: y = 1'b01;
        endcase
    end
    end
endmodule

module NonFullCase(y_out);
    output reg [1:0] y_out;
    reg [1:0] x, y;
    reg [1:0] x1, x2;

    always @(*) 
    begin
        case(x)                 //expected to fire NonFullCase, since not all potential values for case(x) are handled
            2'b00: y = 1'b00;
            2'b01: y = 1'b01;
            // Missing cases for '10' & '11'
        endcase

        case(x1)                //expected not to fire NonFullCase, since  their is a default 
            2'b00: y = 1'b00;
            2'b01: y = 1'b01;
            default: y = 1'b0;
        endcase

        case(x2)                 //expected not to fire NonFullCase, since  all potential values for case(x2) are handled
            2'b00: y = 1'b00;
            2'b01: y = 1'b01;
            2'b10: y = 1'b01;
            2'b11: y = 1'b01;
        endcase

        y_out = y;
    end
endmodule

module NonParallelCase(y_out);
    output reg [1:0] y_out;
    reg [1:0] x, y;

    always @(*) 
    begin
        case(x)                 //expected to fire NonParallelCase
            2'b00: y = 1'b00;
            2'b0?: y = 1'b01;
            2'b?0: y = 1'b10;
            default: y = 1'b11;
        endcase
        case(x)                  //expected not to fire NonParallelCase
            2'b00: y = 1'b00;
            2'b01: y = 1'b01;
            2'b10: y = 1'b10;
            default: y = 1'b11;
        endcase
        y_out = y;
    end
endmodule

module MultipleDrivers(myIn, outputVar);
    input [1:0] myIn;
    output reg [1:0] outputVar;
    reg myReg=5;

    always @(*) 
    begin
        myReg = myReg + 1;                  //expected to fire MultipleDrivers, since there exists another always block 
                                           //assigning another value for "myReg" with the same condition for the first assign (Race) 
    end
    always @(*)                     
    begin 
        myReg = 1'b0;                   //MultipleDrivers is fired because of this statement
        outputVar = myIn;
    end
endmodule


module ArithmeticOverflow(a,b,result,result2,k);
    input reg [3:0] a, b;
    output reg [3:0] result;
    output reg [4:0] result2;
    output reg k;
    always @ (*)
    begin
        k = a + b;              //expected to fire ArithmeticOverflow, since the size  of 'k' is less than or equal the size of 'a' and 'b'
        result = a + b;         //expected to fire ArithmeticOverflow, since the size  of 'result' is less than or equal the size of 'a' and 'b'
        result2 = a + b;        //expected not to fire ArithmeticOverflow, since the size  of 'result2' is greater than the size of 'a' and 'b'
     end
endmodule