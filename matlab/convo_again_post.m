%% qam64-34
[~,idx_u] = unique(round(qam64_34.sinr,2));

sinr_unique = round(qam64_34.sinr(sort(idx_u)),2);
qam_unique = qam64_34.per(sort(idx_u));
semilogy(sinr_unique,qam_unique)

qam64_34.unique.sinr = sinr_unique;
qam64_34.unique.per = qam_unique;

%% qam64-23
[~,idx_u] = unique(round(qam64_23.sinr,2));

sinr_unique = round(qam64_23.sinr(sort(idx_u)),2);
qam_unique = qam64_23.per(sort(idx_u));
semilogy(sinr_unique,qam_unique)
qam64_23.unique.sinr = sinr_unique;
qam64_23.unique.per = qam_unique;


%% qam16_34
[~,idx_u] = unique(round(qam16_34.sinr,2));

sinr_unique = round(qam16_34.sinr(sort(idx_u)),2);
qam_unique = qam16_34.per(sort(idx_u));
semilogy(sinr_unique,qam_unique)
qam16_34.unique.sinr = sinr_unique;
qam16_34.unique.per = qam_unique;




%% qpsk_34
[~,idx_u] = unique(round(qpsk34.sinr,2));

sinr_unique = round(qpsk34.sinr(sort(idx_u)),2);
qam_unique = qpsk34.per(sort(idx_u));
semilogy(sinr_unique,qam_unique)
qpsk34.unique.sinr = sinr_unique;
qpsk34.unique.per = qam_unique;



%% bpsk_34
[~,idx_u] = unique(round(bpsk34.sinr,2));

sinr_unique = round(bpsk34.sinr(sort(idx_u)),2);
qam_unique = bpsk34.per(sort(idx_u));
semilogy(sinr_unique,qam_unique)

bpsk34.unique.sinr = sinr_unique;
bpsk34.unique.per = qam_unique;


%% bpsk12

xpoints = x12;
xaxis_hd = xpoints(2):1/1000:xpoints(end);
xaxis_hd_ = [ xpoints(1):1/200:xpoints(2) xaxis_hd(1:75)];


header = linspace(0,1.925,50);
trailer = linspace(5.975,25,600);
body = [xaxis_hd_+2.925, xaxis_hd(76:end)+2.925];
bpsk12.sinr = [header body trailer];
header1 = ones(1,numel(linspace(0,1.925,50)));
body1 = [bpsk12_,savey1];
trailer1 = zeros(1,numel(linspace(5.975,25,600)));
bpsk12.per = [header1 body1 trailer1];

[~,idx_u] = unique(round(bpsk12.sinr,2));

sinr_unique = round(bpsk12.sinr(sort(idx_u)),2);
qam_unique = bpsk12.per(sort(idx_u));
semilogy(bpsk12.unique.sinr,bpsk12.unique.per)
bpsk12.unique.sinr = sinr_unique;
bpsk12.unique.per = qam_unique;



%% qpsk_12

xpoints = x12 + 3;
xaxis_hd = xpoints(2):1/1000:xpoints(end);
xaxis_hd_ = [ xpoints(1):1/200:xpoints(2) xaxis_hd(1:75)];


header = linspace(0,4.925,100);
trailer = linspace(8.975,25,600);
body = [xaxis_hd_+2.925, xaxis_hd(76:end)+2.925];
qpsk12.sinr = [header body trailer];
header1 = ones(1,numel(linspace(0,4.925,100)));
body1 = [qpsk12_,savey2];
trailer1 = zeros(1,numel(linspace(8.975,25,600)));
qpsk12.per = [header1 body1 trailer1];

[~,idx_u] = unique(round(qpsk12.sinr,2));

sinr_unique = round(qpsk12.sinr(sort(idx_u)),2);
qam_unique = qpsk12.per(sort(idx_u));
semilogy(qpsk12.unique.sinr ,qpsk12.unique.per)
qpsk12.unique.sinr = sinr_unique;
% qpsk12.unique.per = qam_unique;


%% qam16_12

xpoints = x12 + 6;
xaxis_hd = xpoints(2):1/1000:xpoints(end);
xaxis_hd_ = [ xpoints(1):1/200:xpoints(2) xaxis_hd(1:75)];

header = linspace(0,10.43,200);
trailer = linspace(14.48,25,600);
body = [xaxis_hd_+5.425, xaxis_hd(76:end)+5.425];
qam16_12.sinr = [header body trailer];
header1 = ones(1,numel(linspace(0,10.43,200)));
body1 = [bpsk12_,savey3];
trailer1 = zeros(1,numel(linspace(14.48,25,600)));
qam16_12.per = [header1 body1 trailer1];

[~,idx_u] = unique(round(qam16_12.sinr,2));

sinr_unique = round(qam16_12.sinr(sort(idx_u)),2);
qam_unique = qam16_12.per(sort(idx_u));
semilogy(qam16_12.unique.sinr,qam16_12.unique.per)
qam16_12.unique.sinr = sinr_unique;
% qam16_12.unique.per = qam_unique;





