
[esc dat] = runAdcLog(esc);

v2 = hex2dec('8000')
v3 = hex2dec('4000')

f2 = dat >= v2;
dat(f2) = dat(f2) - v2;

f3 = dat >= v3;
dat(f3) = dat(f3) - v3;

scale = 3.3 / 1024 * 12.7 / 2.7;

pwm_rate = 80000;
t = (1:size(dat,2)) / pwm_rate * 1000;

subplot(311)
plot(t(f3(1,:)),dat(:,f3(1,:)) * scale, '-o');
ylim([0 25]);
ylabel('[V]')
legend('A', 'B', 'C')
title('Off period voltage');

subplot(312)
plot(t(~f3(1,:)),dat(:,~f3(1,:))* scale, '-o');
ylim([0 25]);
ylabel('[V]')
legend('A', 'B', 'C')
title('On period Voltage');

subplot(313)
interpT = 1:0.1:size(dat,2);
interpHigh=interp1(t(f3(1,:)), dat(:,f3(1,:))' * scale, interpT);
interpLow=interp1(t(~f3(1,:)), dat(:,~f3(1,:))'* scale, interpT);
plot(interpT, interpHigh-interpLow);
ylim([-1 1]);
xlabel('[us]')
ylabel('[V]')
legend('A', 'B', 'C')
title('Voltage difference between ON and OFF');
