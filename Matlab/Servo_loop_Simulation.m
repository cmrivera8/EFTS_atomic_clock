close all
clear all
clc
%% Define CPT signal
clc
%Adding absorption valley
step=0.005;
x=-pi:step:pi;
absorption_amplitude=0.22/2;
absorption_offset=2.58+absorption_amplitude;
y=-absorption_amplitude*cos(x)+absorption_offset;

%Adding transmission peak
cpt_amplitude=0.026/2;
cpt_offset=2.58+cpt_amplitude;
y2=cpt_amplitude*cos(50*x)+cpt_offset;
y2=y2(1,617:642);

%Mixing signals
y3=[];
y3=[y3,y(1,1:((end/2))-size(y2,2)/2)];
y3=[y3,y2];
y3=[y3,y(1,(end/2+size(y2,2)/2):end)];

%Plot individual signals
hold off
plot(y)
hold on
plot(y2)
plot(y3)
grid on

% Fit mixed signal
real_x=linspace(1.16,1.17,size(y3,2));
[xData, yData] = prepareCurveData(real_x', y3 );

% Set up fittype and options.
ft = fittype( 'smoothingspline' );
opts = fitoptions( 'Method', 'SmoothingSpline' );
opts.Normalize = 'on';
opts.SmoothingParam = 0.9999999;

% Fit model to data.
[fy, gof] = fit( xData, yData, ft, opts );

%Plot final signal
hold off
plot(xData,fy(xData),'LineWidth',1.5);
axis([1.16 1.17 2.57 2.81])
grid on
%% Laser servo loop
clc

%Prepare plot
hold off
tiledlayout(2,1)

% Top plot
subplot(2,1,1)
fig1=scatter(0,0);
plot(xData,fy(xData),'LineWidth',1.5);
axis([1.16 1.17 2.57 2.81])

initial_guess=1.162;
laser_lock_step=0.0001;
kp=0.01;

previous_value=initial_guess;

for i=1:50
value1=fy(previous_value+laser_lock_step);
value2=fy(previous_value-laser_lock_step);
error=value1-value2;

%Plot modulation range
hold on
subplot(2,1,1) 
s1=scatter(previous_value+laser_lock_step,value1,'x','LineWidth',1.5);
s1.SizeData = 300;
s2=scatter(previous_value-laser_lock_step,value2,'x','LineWidth',1.5);
s2.SizeData = 300;

%Plot lock value
hold on
subplot(2,1,1) 
delete(fig1)
fig1=scatter(previous_value,fy(previous_value),'SeriesIndex',i,'LineWidth',1.5);
axis([1.16 1.17 2.57 2.81])

subplot(2,1,2) 
fig2=scatter(previous_value,error,'SeriesIndex',i,'LineWidth',1.5);
axis([1.16 1.17 -15e-3 1e-3])

%Set new value based on error signal
previous_value=previous_value-kp*error;

pause(0.5);

%Adding Title and Labels
%Top graph
subplot(2,1,1) 
title('Simulated CPT signal (not to scale)')
xlabel('Laser current (mA)')
ylabel('Photodiode voltage (V)')
grid on
legend('CPT signal','Right modulation','Left modulation','Ongoing laser current','AutoUpdate','off')
%Bottom graph
subplot(2,1,2) 
title('Error signal for laser lock')
xlabel('Laser current (mA)')
ylabel('Error signal (a.u.)')
grid on
legend('Ongoing error signal','AutoUpdate','off')


%Save image to create animation
filename=sprintf('animation_laser_servo_loop/laser1_%d',i);
% save_pdf(filename)

% return
delete(s1)
delete(s2)
end
filename=sprintf('animation_laser_servo_loop/laser1_%d',0);
% save_pdf(filename)

%% Quartz servo loop (Show Error signal)
clc

%Prepare plot
hold off
tiledlayout(2,1)

% Top plot
subplot(2,1,1)
plot(xData,fy(xData),'LineWidth',1.5);
axis([1.163 1.167 2.57 2.65])
grid on

start=1.1645;
finish=1.1655;
quartz_mod_step=0.000099;

previous_value=initial_guess;
error=1;

% Bottom plot
index=0;
for i=start:0.0000165:finish
value1=fy(i);
value2=fy(i+quartz_mod_step);
error=value1-value2;

%Plot modulation range
hold on
subplot(2,1,1) 
s1=scatter(i,value1,'x','LineWidth',1.5);
s1.SizeData = 300;
s2=scatter(i+quartz_mod_step,value2,'x','LineWidth',1.5);
s2.SizeData = 300;
% axis([1.163 1.167 2.57 2.65])
axis([start finish 2.57 2.62])
%Plot current frequency
hold on
subplot(2,1,1) 
fig1=scatter(i,fy(i),'SeriesIndex',index+1,'LineWidth',1.5);
fig_tmp=scatter(i,fy(i),'SeriesIndex',index+1,'LineWidth',1.5);
fig1.Annotation.LegendInformation.IconDisplayStyle = 'off';
legend('CPT signal','FSK modulation: Off','FSK modulation: On','Ongoing PLL frequency','AutoUpdate','off')
pause(0.1)
delete(fig_tmp);

%Plot Error signal
subplot(2,1,2) 
fig2=scatter(i,error,'SeriesIndex',index+1,'LineWidth',1.5);
fig2.Annotation.LegendInformation.IconDisplayStyle = 'off';
fig_tmp=scatter(i,error,'SeriesIndex',index+1,'LineWidth',1.5);
legend('Ongoing error signal','AutoUpdate','off');
pause(0.1)
delete(fig_tmp);
% axis([1.16 1.17 -0.03 0.03])
axis([start finish -0.03 0.03])
grid on

%Adding Title and Labels
%Top graph
subplot(2,1,1) 
title('Simulated CPT signal (not to scale)')
xlabel('Laser current (mA)')
ylabel('Photodiode voltage (V)')
grid on

%Bottom graph
subplot(2,1,2) 
title('Error signal for quartz lock')
xlabel('Laser current (mA)')
ylabel('Error signal (a.u.)')
grid on

%Save image to create animation
filename=sprintf('animation_quartz_servo_loop/quartz_%d',index+1);
% save_pdf(filename)

delete(s1)
delete(s2)

index=index+1;
end
%Save image to create animation
filename=sprintf('animation_quartz_servo_loop/quartz_%d',0);
% save_pdf(filename)

%% Functions 
function save_pdf(filename)
clear figure_property;
figure_property.units = 'inches';
figure_property.format = 'pdf';
figure_property.Preview= 'none';
figure_property.Width= '8'; % Figure width on canvas
figure_property.Height= '11'; % Figure height on canvas
figure_property.Units= 'inches';
figure_property.Color= 'rgb';
figure_property.Background= 'w';
% figure_property.FixedfontSize= '12';
% figure_property.ScaledfontSize= 'auto';
% figure_property.FontMode= 'scaled';
% figure_property.FontSizeMin= '12';
% figure_property.FixedLineWidth= '1';
% figure_property.ScaledLineWidth= 'auto';
% figure_property.LineMode= 'none';
% figure_property.LineWidthMin= '0.1';
% figure_property.FontName= 'Times New Roman';% Might want to change this to something that is available
% figure_property.FontWeight= 'auto';
% figure_property.FontAngle= 'auto';
% figure_property.FontEncoding= 'latin1';
figure_property.PSLevel= '3';
figure_property.Renderer= 'painters';
figure_property.Resolution= '600';
figure_property.LineStyleMap= 'none';
figure_property.ApplyStyle= '0';
figure_property.Bounds= 'tight';
figure_property.LockAxes= 'off';
figure_property.LockAxesTicks= 'off';
figure_property.ShowUI= 'on';
figure_property.SeparateText= 'off';
chosen_figure=gcf;
set(chosen_figure,'PaperUnits','inches');
set(chosen_figure,'PaperPositionMode','auto');
set(chosen_figure,'PaperSize',[str2num(figure_property.Width) str2num(figure_property.Height)]); % Canvas Size
set(chosen_figure,'Units','inches');
hgexport(gcf,filename,figure_property); %Set desired file name
end