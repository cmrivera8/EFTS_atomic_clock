% clear all
% close all
hold off
cla reset
% delete(yp)
% delete(dyp)
clc

%Define input curve
%Adding transmission peak
x=-pi:0.01:pi;
y=cos(x);
%Adding flat part
y=[-1.*ones(1,50),y,-1*ones(1,50)];

%Interpolation
x=[1:size(y,2)];
[fy,gof] =fit(x',y','linearinterp');

yyaxis left
plot(fy(x))
hold on 

%Calculating derivate as the program
dy=[];

step=10;
for step=1:50:400
    dy=[];
for i=1:step:size(y,2)-1
	dy=[dy,y(i)-y(i+1)];
end
dy_x=[1:size(dy,2)];
% [fdy,gof] =fit(dy_x',dy','linearinterp');

nc = numel(dy);
nf = numel(y);
fdy=interp1(linspace(1,nf-3,nc),dy,1:nf);

yyaxis right
plot(fdy,'r');
% pause
end
% yyaxis left
% yp=plot(y);
% return
% yyaxis right
% dyp=plot(dy);

legend
grid on
%%
Fs = 2000
t = 500
ar1 = dy;
ar2 = downsample(ar1,4);
nc = numel(ar2);
nf = numel(ar1);
ar2f = interp1(linspace(1,nf-3,nc),ar2,1:nf);
numel(ar2f) % now ar2f has the same length as ar1

hold off
plot(ar1)
hold on
plot(ar2f)