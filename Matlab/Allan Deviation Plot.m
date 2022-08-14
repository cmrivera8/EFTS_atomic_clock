%% Extract data from file 
clear all
clc

hold off
[averaging_time, allan_deviation, phase_noise_floor] = extract_data('no_lock_measurement/adev_1s.txt'); %Quartz free running
averaging_time_1=averaging_time;
phase_noise_floor_1=phase_noise_floor;
plot_allan(averaging_time, allan_deviation, phase_noise_floor)
% [averaging_time, allan_deviation, phase_noise_floor] = extract_data('lock_on_measurement/adev_1s.txt'); %Clock 3
% plot_allan(averaging_time, allan_deviation, phase_noise_floor)
% [averaging_time, allan_deviation, phase_noise_floor] = extract_data('clock1_working/adev_1s.txt');
% plot_allan(averaging_time, allan_deviation, phase_noise_floor)
% [averaging_time, allan_deviation, phase_noise_floor] = extract_data('experiment_1_clock_1/adev_1s.txt'); %Clock 1 - pid set 1
% plot_allan(averaging_time, allan_deviation, phase_noise_floor)
[averaging_time, allan_deviation, phase_noise_floor] = extract_data('experiment_2_clock_1/adev_1s.txt'); %Clock 1 - pid set 2
plot_allan(averaging_time, allan_deviation, phase_noise_floor)
% [averaging_time, allan_deviation, phase_noise_floor] = extract_data('Settings 1/adev_1s.txt');
% plot_allan(averaging_time, allan_deviation, phase_noise_floor)
% [averaging_time, allan_deviation, phase_noise_floor] = extract_data('Settings 2/adev_1s.txt');
% plot_allan(averaging_time, allan_deviation, phase_noise_floor)
% legend('Allan deviation', 'Measured points','Instrument phase noise floor')
grid on
% axis([10e-1 10e2 10e-15 10e-8])
axis([10e-1 10e1 10e-15 10e-10]) 

%Plotting the phase noise floor
area(averaging_time_1,phase_noise_floor_1);
legend('Quartz free running','Quartz with locked servo loops','Phase noise floor','AutoUpdate','off','location','southwest')
title('Frequency stability comparison', 'Free running quartz vs locked servo loops quartz')
saveas(gcf,'output/allan_deviation','epsc')
return
%% Extract information from fcounter file
clear all
clc
fid = fopen('no_lock_measurement/fcounter.txt', 'rt'); 
C = textscan(fid, '%f%f' ,'HeaderLines',3,'Delimiter',',');
fclose(fid);
av_time=C{1};
frequency=C{2}*1e6;
hold off
semilogx(av_time,frequency)
grid on
%% Extract spectrum
clear all
clc
fid = fopen('no_lock_measurement/spectrum.txt', 'rt'); 
C = textscan(fid, '%f%f','Delimiter','\t');
fclose(fid);
x=C{1};
y=C{2};
hold off
plot(x,y)
grid on
%%
function plot_allan(averaging_time, allan_deviation, phase_noise_floor)
loglog(averaging_time,allan_deviation,'LineWidth',1.5)
hold on
h=scatter(averaging_time,allan_deviation);
h.Annotation.LegendInformation.IconDisplayStyle = 'off';
% legend('Quartz free running','Clock 3 (not working properly)','Clock 1','Clock 1 (modified pid 0)','Clock 1 (modified pid 1)','Clock 1 (modified pid 2)','AutoUpdate','off','location','southwest')
% legend('Quartz free running','Quartz with locked servo loops','Phase noise floor','AutoUpdate','off','location','southwest')
% h=area(averaging_time,phase_noise_floor);
% h.Annotation.LegendInformation.IconDisplayStyle = 'off';
xlabel('Averaging time')
ylabel('Allan Deviation')
end
function [averaging_time, allan_deviation, phase_noise_floor]=extract_data(file)
file=strcat('SymmetricomData/',file);
fid = fopen(file, 'rt');
data_cell = textscan(fid, '%f%f%f','Delimiter','\t');
fclose(fid);
averaging_time=data_cell{1};
allan_deviation=data_cell{2};
phase_noise_floor=data_cell{3};
end