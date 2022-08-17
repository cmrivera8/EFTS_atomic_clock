clear all
close all
clc
%% Plot the value of ADC vs Voltage to determine conversion factor
% Measurement 1 (Didn't work)
% DAC_word=[28000,    28500,  29000,    29500,  30000,  30500,  31000,  31500,  32000,  32500,  33000,  33500,  34000];
% new_volt=[1.8710,   1.9424, 2.0390,   2.1180, 2.1973, 2.2782, 2.3575, 2.4399, 2.5193, 2.5999, 2.6799, 2.7600  2.8411];
% ADC_word=[45003,    44994,  47861,    52944,   50396,  52935,  55518,  59288,  60662,  64669,  292,    2885,   5528];

% Measurement with 3ms second settling time (CPT on)(Didn't work)
% DAC_word=[28000,	28500,	29000,      29500,  30000,  30500,  31000,  31500,  32000,  32500,  33000,  33500,  34000];
% new_volt=[1.8160,   1.1157, 1.9598,     1.8918, 2.0181, 2.0347, 1.7696, 2.2841, 2.1909, 2.4611, 2.4316, 2.4903, 2.7530];
% ADC_word=[38864,    32768,  45278,      46480,  49434,  51393,  42243,  58283,  55373,  63875,  64132,  64141,  8073];

% Measurement with 6ms second delay between measurements (CPT on) (Best)
DAC_word=[28000,    28250,  28500,  28750,  29000,  29250,  29500,  29750,  30000,  30250,  30500,  30750,  31000,  31250,  31500,  31750,  32000,  32250,  32500,  32750,  33000,  33250,  33500,  33750,  34000];
new_volt=[1.8110,   1.7293, 1.7673, 1.8570, 1.9593, 1.9830, 1.8902, 1.9346, 2.0212, 2.1058, 2.0282, 1.6720, 1.7722,	2.781,  2.2831, 2.2826  2.1932, 2.2721, 2.4619, 2.5172, 2.4241, 2.4095, 2.4922, 2.6903, 2.7463];
ADC_word=[41145,    40960,  41867,  44691,  48084,  48945,  46053,  47303,  50013,  52853,  50668,  39004,  42160,  51556,  58479,  58636,  55525,  58094,  64210,  595,    63335,  62636,  65044,  6083,   8042];

% Correct ADC overflow
for i=1:size(ADC_word,2)
    if ADC_word(i)< 20000
        ADC_word(i)=65535+ADC_word(i);
    end
end

%% With new overflow correction
%With ADC FSR = 1.024 V
% ADC_word=[-16210,-13494,-10875,3399 ,17367]
% new_volt=[1.9650,2.0300,2.1450,2.545,2.985]

%With ADC FSR = 4.096 V
ADC_word=[1900,    3500,   1100,   7200,   10500];
new_volt=[2.72425, 2.9197, 2.6222, 3.3872, 3.7910];
hold off

scatter(ADC_word,new_volt,'LineWidth',1.5)
grid on
ylabel('Photodiode voltage (V)') 
xlabel('ADC measurement (machine units)') 
title('Comparison between ADC and photodiode voltage measurements')

% Fit of curve Voltage vs ADC reading
hold on

[f2,gof] =fit(ADC_word',new_volt','poly1','Robust','Bisquare');
clc
vpa(coeffvalues(f2))
eq2=formula(f2)
plot(ADC_word,f2(ADC_word),'LineWidth',1.5)

legend('Measured points','Linear fit','Location','southeast')
ylabel('Photodiode voltage (V)') 
xlabel('ADC measurement (machine units)') 
grid on
% saveas(gcf,'output/word_to_voltage_fsr_1','epsc')
saveas(gcf,'output/word_to_voltage_fsr_4','epsc')
%% Fit of curve ADC reading vs DAC word
clc

hold off
plot(DAC_word,ADC_word)
f3 = fit(DAC_word',ADC_word','smoothingspline');%,'Robust','Bisquare');
hold on
DAC_word_matlab = 26900:34500; % 1.0262 mA to 1.3161 mA
word_to_current(29000)
current_to_word(1.3)
y=fix(f3(DAC_word_matlab));

%Add overflow
% plot(DAC_word_matlab,y)
for i=1:size(y,1)
    if y(i)>2^16
        y(i)=y(i)-2^16;
    end
end
plot(DAC_word_matlab,y)
grid on

% Save array to text file
fileID = fopen('result.txt','w');
fprintf(fileID,"int f[]={");
for i=1:size(y,1)-1
    if mod(i,100)==0
        fprintf(fileID,"%d,\n",y(i));
    else
        fprintf(fileID,"%d,",y(i));
    end
end
fprintf(fileID,"%d};\n",y(end));
fclose(fileID);

return
%% Word to Hertz conversion
Quartz_DAC_word=[0,5000,10000,15000,20000,25000,30000,35000,40000,45000,50000,55000,60000,65000];
Offset=[4.596301875,4.596310000,4.596318125,4.596326250,4.596334375,4.596342500,4.596350625,4.596359375,4.596366875,4.596375625,4.596383125,4.596391250,4.596399375,4.596407500];

%Apply correct units
Offset=Offset*1e9;
%Remove offset
Offset=Offset-min(Offset);

hold off
scatter(Quartz_DAC_word,Offset,'LineWidth',1.5)
grid on
ylabel('Frequency offset (Hz)') 
xlabel('Quartz´s DAC setpoints (machine units)') 
title('Comparison between quartz´s DAC and frequency offset measurements',' ')

[f2,gof] =fit(Quartz_DAC_word',Offset','poly1','Robust','Bisquare');
clc
vpa(coeffvalues(f2))
eq2=formula(f2)
hold on
plot(Quartz_DAC_word,f2(Quartz_DAC_word),'LineWidth',1.5)

legend('Measured points','Linear fit','Location','southeast')
grid on
saveas(gcf,'output/word_to_frequency','epsc')
%% Functions
function result=word_to_current(value)
    result=(value/2^16)*2.5;
end
function result=current_to_word(value)
    result=fix((value/2.5)*2^16);
end
function scatter_datatip(x,y)
sc=scatter(x,y);
datatip(sc,x,y);
hold on
end