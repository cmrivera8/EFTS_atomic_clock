clc
close all
clear all

%% VCSEL_startup()
close all

y=0:100:(30000+100);
stairs(y)
%%
grid on
axis()
%Plot labels
title('Ramp of VCSEL current')
xlabel('Sample')
ylabel('Amplitude')

%% LaserLock() Example with arbitrary signal
close all

graph_step=0.1;
x=0:graph_step:2;
samples = 1:size(x,2);
y = -(2.9*x-2.9).^2+8;

subplot(2,1,1);
hold off
plot(samples,y,'LineWidth',1.5)
grid on
axis([1 samples(end) 0 8.5])
xticks([1:samples(end)])
xtickangle(45)

%Plot labels
title('Arbritary signal measured by ADC')
xlabel('Sample') 
ylabel('Amplitude') 

ADC=[30000];
step=1;
for i=2:size(y,2)
 if y(i)<y(i-1)
     ADC=[ADC,ADC(end)+step];
 else
     ADC=[ADC,ADC(end)-step];
 end
end

subplot(2,1,2); 
stairs(samples,ADC,'LineWidth',1.5)
grid on
axis([1 samples(end) 2.9989e4 3.0001e4])
xticks([1:samples(end)])
%Plot labels
title('Resulting DAC signal')
xlabel('Sample') 
ylabel('Amplitude') 
xtickangle(45)

%Saving plot
saveas(gcf, 'Linear_lock_simulation_result.png')