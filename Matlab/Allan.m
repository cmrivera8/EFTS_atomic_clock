clear all
clc
close all
%Lock off
averaging_time=[0.1,0.2,0.4,1,2,4,10,20,40,100,200,400,1000,2000,4000];
allan_deviation=[0.0000000000400100000,0.0000000000410500000,0.0000000000423200000,0.0000000000478300000,0.0000000000533000000,0.0000000000536000000,0.0000000000519000000,0.0000000000492000000,0.0000000000559000000,0.0000000000940000000,0.0000000001670000000,0.0000000002970000000,0.0000000006000000000,0.0000000010700000000,0.0000000019000000000];
phase_noise_floor=[0.0000000000004578180,0.0000000000003656200,0.0000000000002912680,0.0000000000002012580,0.0000000000001544500,0.0000000000001108650,0.0000000000000919294,0.0000000000000691150,0.0000000000000639392,0.0000000000000623318,0.0000000000000536829,0.0000000000000408009];

loglog(averaging_time,allan_deviation)
hold on
scatter(averaging_time,allan_deviation)
xlabel('Averaging time')
ylabel('Allan Deviation')
grid on
axis([10e-1 10e4 10e-14 10e-8])

%lock on

%% Extract data from file 
clear all
clc

hold off
[averaging_time, allan_deviation, phase_noise_floor] = extract_data('no_lock_measurement/adev_1s.txt');
plot_allan(averaging_time, allan_deviation, phase_noise_floor)
[averaging_time, allan_deviation, phase_noise_floor] = extract_data('lock_on_measurement/adev_1s.txt');
plot_allan(averaging_time, allan_deviation, phase_noise_floor)
% [averaging_time, allan_deviation, phase_noise_floor] = extract_data('clock1_working/adev_1s.txt');
% plot_allan(averaging_time, allan_deviation, phase_noise_floor)
[averaging_time, allan_deviation, phase_noise_floor] = extract_data('experiment_1_clock_1/adev_1s.txt');
plot_allan(averaging_time, allan_deviation, phase_noise_floor)
[averaging_time, allan_deviation, phase_noise_floor] = extract_data('experiment_2_clock_1/adev_1s.txt');
plot_allan(averaging_time, allan_deviation, phase_noise_floor)
% [averaging_time, allan_deviation, phase_noise_floor] = extract_data('Settings 1/adev_1s.txt');
% plot_allan(averaging_time, allan_deviation, phase_noise_floor)
% [averaging_time, allan_deviation, phase_noise_floor] = extract_data('Settings 2/adev_1s.txt');
% plot_allan(averaging_time, allan_deviation, phase_noise_floor)
% legend('Allan deviation', 'Measured points','Instrument phase noise floor')
grid on
axis([10e-1 10e2 10e-15 10e-8])
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
legend('Quartz free running','Clock 3 (not working properly)','Clock 1','Clock 1 (modified pid 0)','Clock 1 (modified pid 1)','Clock 1 (modified pid 2)','AutoUpdate','off','location','southwest')
h=area(averaging_time,phase_noise_floor);
h.Annotation.LegendInformation.IconDisplayStyle = 'off';
xlabel('Averaging time')
ylabel('Allan Deviation')
end
function [averaging_time, allan_deviation, phase_noise_floor]=extract_data(file)
fid = fopen(file, 'rt');
data_cell = textscan(fid, '%f%f%f','Delimiter','\t');
fclose(fid);
averaging_time=data_cell{1};
allan_deviation=data_cell{2};
phase_noise_floor=data_cell{3};
end