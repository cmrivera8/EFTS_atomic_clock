%% Calculating RF Synthesizer
%% Determine Register values out of desired RFout
clear all 
clc
format long eng

% RFout = 5800.2e6;       % Required frequency 5.8002 GHz (Datasheet)
RFout = 5.96e6;  % Found value from registers
REFin = 10e6;           % Reference frequency 10 MHz

% Frequency resolution:
Fres = REFin/2^25; % units: Hz

% PHASE FREQUENCY DETECTOR (PFD)
D = 0;% RF REFin doubler bit
R = 1;% RF reference division factor
T = 0;% reference divide-by-2 bit (0 or 1)

Fpfd = REFin*((1+D)/(R*(1+T)));

% Calculating N and FRAC values
INT = fix(RFout/Fpfd);    % Integer part
Fmsb = fix(((RFout/Fpfd)-INT)*2^12);
Flsb = fix(((((RFout/Fpfd)-INT)*2^12)-Fmsb)*2^13);
FRAC = Fmsb*2^13+Flsb;

fprintf("> N: %0.25f\n",RFout/Fpfd)
fprintf("> INT (integer of N):\n")
printBits(INT)
fprintf("> FRAC:\n")
printBits(FRAC)
fprintf("> MSb value of FRAC:\n")
printBits(Fmsb)
fprintf("> LSb value of FRAC:\n")
printBits(Flsb)

%% Reg to decimal
clc
printBits(0x7)
printBits(0x800006)
printBits(0x6)
printBits(0x800005)
printBits(0x200695)
printBits(0x180104)
printBits(0x143)
printBits(0x40800A)
printBits(0x80F0001)
printBits(0xE5D190) 

printBits(0x1280001)
printBits(0xF8E5D0A8)

%% Determine desired RFout based on hex values sent
%% Content of function "void ADF4158_Set_CPT()"
clear all
clc
format long eng

REFin = 10e6;	% Reference clock

% Input individual values:
% Original ADF4158_Set_CPT function content:
% Fmsb = 2581;	% From R0 = 0xF8E5D0A8
% INT = 459;      % From R0 = 0xF8E5D0A8
% Flsb = 592;     % From R1 = 0x1280001

% "Precedent" commented value for R1 in ADF4158_Set_CPT:
% Fmsb = 2581;      % From R0 = 0xFFF8001
% INT = 459;        % From R0 = 0xF8E5D0A8
% Flsb = 8191;      % From R1 = 0x1280001

% Original ADF4158_Set_CPT_lock function content:
% Fmsb = 2610;	% From R0 = 0xE5D190
% INT = 459;      % From R0 = 0xE5D190
% Flsb = 4126;     % From R1 = 0x80F0001

% Commented values in ADF4158_Set_CPT_lock function:
% Fmsb = 2597;	% From R0 = 0xE5D128
% INT = 459;      % From R0 = 0xE5D128
% Flsb = 7093;     % From R1 = 0xDDA8001

% Input registers:
r1=0x1280001;
r0=0xF8E5D0A8;

Flsb=double(bi2de(bitget(r1,28:-1:16),'left-msb'));
Fmsb=double(bi2de(bitget(r0,15:-1:4),'left-msb'));
INT=double(bi2de(bitget(r0,27:-1:16),'left-msb'));

FRAC = Fmsb*2^13+Flsb;
% PHASE FREQUENCY DETECTOR (PFD)
D = 0;% RF REFin doubler bit
R = 1;% RF reference division factor
T = 0;% reference divide-by-2 bit (0 or 1)

%  The INT and FRAC registers define an overall N-divider as N = INT + (FRAC/2^25).

Fpfd = REFin*((1+D)/(R*(1+T))); %PFD: Phase Frequency Detection
RFout = (INT+(FRAC/2^25))*Fpfd;

% fprintf("FRAC: %d\n", FRAC)
% fprintf("N: %0.21f\n", (INT+(FRAC/2^25)))
fprintf("RFout: %sHz\n", num2eng(RFout,true,false,true,11))

%% Functions
function printBits(value)
    format long
    fprintf("DEC: %d\n", value)
    return
    fprintf("HEX: %s\n", dec2hex(value))
    fprintf("BIN: ")
    bin_num = de2bi(value,'left-msb');
    fprintf("%d", bin_num)
    fprintf("\n")
    format short eng
end