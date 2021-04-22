cutoff = 1; % percent

SINR = -5:1/100:55; %Es/No
xlimit = [min(SINR),max(SINR)];

% 64-QUAM34
close all
h = [
13.75,1
15,8.5e-1
15.9,3.90e-1
17,7e-2
18,7.75e-3
19,1.4e-3];

x64 = h(:,1)' + 1;
y64 = h(:,2)';
c = linspace(.3,1,5);
% 34 endoding, 64-qam
semilogy([-5 x64]+2.12,[1 y64],'Marker','.','Color',[0,c(1),0]); xlim(xlimit); grid on; hold on
% 23 encoding, 64-qam
semilogy([-5 (2*x64-6)/2]+4.09,[1 y64],'Marker','.','Color',[0,c(2),0]); xlim(xlimit); grid on
% 34 encoding, 16-qam
semilogy([-5 (x64-6)]+2.1,[1 y64],'Marker','.','Color',[0,c(3),0]); xlim(xlimit); grid on
% 34 encoding, qpsk
semilogy([-5 (x64-12)]+2.65,[1 y64],'Marker','.','Color',[0,c(4),0]); xlim(xlimit); grid on
% 34 encoding, bpsk
semilogy([-5 (x64-15)]+2.643,[1 y64],'Marker','.','Color',[0,c(5),0]); xlim(xlimit); grid on;

f = [
-2,1
-.938,3.70e-1
0,5e-2
1.094,3.45e-3
2.05,2e-4];

x12 = f(:,1)' + 1;
y12 = f(:,2)';
c2 = linspace(.3,1,3);

% 12 encoding, bpsk
semilogy([-5 x12]+2.925,[1 y12],'Marker','.','Color',[0,0,c2(1)]); xlim(xlimit); grid on;
% 12 encoding, qpsk
semilogy([-5 (x12+3)]+2.925,[1 y12],'Marker','.','Color',[0,0,c2(2)]); xlim(xlimit); grid on
% 12 encoding, 16-qam
semilogy([-5 (x12+6)]+5.425,[1 y12],'Marker','.','Color',[0,0,c2(3)]); xlim(xlimit); grid on
% legend('64QAM34','64QAM23','16QAM34','QPSK34','BPSK34','BPSK12','QPSK12','16QAM12','Location','southwest')


%% %%%% UNCODED BASELINE %%%%%
%
% k = log2([4 16 64]);
% packet_length = 1537;
%
% Eb_N0_dB = SINR - 10*log10(1);
% theoryBer1 = 0.5*erfc(sqrt(10.^(Eb_N0_dB/10)));
% per1 = 1 - (1 - theoryBer1).^(packet_length*8);
% Eb_N0_dB = SINR - 10*log10(2);
% theoryBer2 = erfc(sqrt(0.5*(10.^(Eb_N0_dB/10)))) - (1/4)*(erfc(sqrt(0.5*(10.^(Eb_N0_dB/10))))).^2;
% per2 = 1 - (1 - theoryBer2).^(packet_length*8/k(1));
% Eb_N0_dB = SINR - 10*log10(4);
% theoryBer3 = 3/2*erfc(sqrt(0.1*(10.^(Eb_N0_dB/10))));
% per3 = 1 - (1 - theoryBer3).^(packet_length*8/k(2));
% kk = sqrt(1/((2/3)*(64-1)));
% Eb_N0_dB = SINR - 10*log10(6);
% theoryBer4 = 2*(1-1/sqrt(64))*erfc(kk*sqrt((10.^(Eb_N0_dB/10)))) ...
% 	              - (1-2/sqrt(64) + 1/64)*(erfc(kk*sqrt((10.^(Eb_N0_dB/10))))).^2;
% per4 = 1 - (1 - theoryBer4).^(packet_length*8/k(3));
%
% semilogy(SINR,per1);
% semilogy(SINR,per2);
% semilogy(SINR,per3);
% semilogy(SINR,per4);

%% %%%%%%%%%% CURVE FITTING %%%%%%%%%%%

xpoints = x64;
xaxis_hd = xpoints(1):1/200:xpoints(2);
f = polyfit(xpoints(1:2),y64(1:2),8);
qam64_34_ = polyval(f,xaxis_hd);
semilogy(xaxis_hd+2.12,qam64_34_);

xaxis_hd = xpoints(2):1/1000:xpoints(end-1);
f = polyfit(xpoints,y64,6);
qam64_341 = polyval(f,xaxis_hd);
semilogy(xaxis_hd+2.12,qam64_341);

%
xpoints = (2*x64 - 6)/2;
xaxis_hd = xpoints(1):1/200:xpoints(2);
f = polyfit(xpoints(1:2),y64(1:2),8);
qam64_23_ = polyval(f,xaxis_hd);
semilogy(xaxis_hd+4.09,qam64_23_);

xaxis_hd = xpoints(2):1/1000:xpoints(end-1);
f = polyfit(xpoints,y64,6);
qam64_231 = polyval(f,xaxis_hd);
semilogy(xaxis_hd+4.09 ,qam64_231);

%
xpoints = x64 - 6;
xaxis_hd = xpoints(1):1/200:xpoints(2);
f = polyfit(xpoints(1:2),y64(1:2),6);
qam16_34_ = polyval(f,xaxis_hd);
semilogy(xaxis_hd+2.1,qam16_34_);

xaxis_hd = xpoints(2):1/1000:xpoints(end-1);
f = polyfit(xpoints,y64,6);
qam16_341 = polyval(f,xaxis_hd);
semilogy(xaxis_hd+2.1,qam16_341)


%
xpoints = x64 - 12;
xaxis_hd = xpoints(1):1/200:xpoints(2);
f = polyfit(xpoints(1:2),y64(1:2),3);
qpsk34_ = polyval(f,xaxis_hd);
semilogy(xaxis_hd+2.65,qpsk34_);

xaxis_hd = xpoints(2):1/1000:xpoints(end-1);
f = polyfit(xpoints,y64,5);
qpsk341 = polyval(f,xaxis_hd);
semilogy(xaxis_hd+2.65,qpsk341)

%
xpoints = x64 - 15;
xaxis_hd = xpoints(1):1/200:xpoints(2);
f = polyfit(xpoints(1:2),y64(1:2),10);
bpsk34_ = polyval(f,xaxis_hd);
plot(xaxis_hd+2.643,bpsk34_);

xaxis_hd = xpoints(2):1/1000:xpoints(end-1);
f = polyfit(xpoints,y64,5);
bpsk341 = polyval(f,xaxis_hd);
plot(xaxis_hd+2.643,bpsk341)

%%% coding bpsk12 %%%
xpoints = x12;
xaxis_hd = xpoints(2):1/1000:xpoints(end);
xaxis_hd_ = [ xpoints(1):1/200:xpoints(2) xaxis_hd(1:75)];
f = polyfit(xpoints(1:2),y12(1:2),1);
bpsk12_ = polyval(f,xaxis_hd_);
semilogy(xaxis_hd_+2.925,bpsk12_); hold on
semilogy(xaxis_hd(76:end)+2.925,savey1)

% coding qpsk12
xpoints = x12 + 3;
xaxis_hd = xpoints(2):1/1000:xpoints(end);
xaxis_hd_ = [ xpoints(1):1/200:xpoints(2) xaxis_hd(1:75)];
f = polyfit(xpoints(1:2),y12(1:2),1);
qpsk12_ = polyval(f,xaxis_hd_);
semilogy(xaxis_hd_+2.925,qpsk12_); hold on
semilogy(xaxis_hd(76:end)+2.925,savey2)

% coding 16-qam12
xpoints = x12 + 6;
xaxis_hd = xpoints(2):1/1000:xpoints(end);
xaxis_hd_ = [ xpoints(1):1/200:xpoints(2) xaxis_hd(1:75)];
f = polyfit(xpoints(1:2),y12(1:2),1);
qam1612_ = polyval(f,xaxis_hd_);
semilogy(xaxis_hd_+5.425,qam1612_); hold on
semilogy(xaxis_hd(76:end)+5.425,savey3)

plot(SINR, (cutoff/100)*ones(numel(SINR),1),'LineWidth',2,'LineStyle',':','Color',[0.5,0.5,0.5]);
title('PER vs. SINR');
ylabel('PER')
xlabel('SINR')

ylim([-10e-6,2])
legend('64QAM34','64QAM23','16QAM34','QPSK34','BPSK34','BPSK12','QPSK12','16QAM12','Location','southwest')
p =  [-1432 72 1412 814]; % [-1397 -58 1412 814] monitor to the right?
set(0, 'DefaultFigurePosition', p);