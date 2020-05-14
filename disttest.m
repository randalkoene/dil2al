% Testing various distribution functions for the selection
% of days in Active_List::expand().

i = [0:9]; % out of 10
x = [0:0.01:1];
xi = i./10;

%y = sqrt(1-x);
%yi = sqrt(1-xi); % pi(1)/pi(10)=3.16

%y = 1 - x.^2;
%yi = 1 - xi.^2; % pi(1)/pi(10)=5.26
%y = 1 - 0.5.*(x.^2);
%yi = 1 - 0.5.*(xi.^2); % pi(1)/pi(10)=1.68
y = 1 - 0.8.*(x.^2);
yi = 1 - 0.8.*(xi.^2); % pi(1)/pi(10)=2.84

%y = 1 - (x.*0.8).^2;
%yi = 1 - (xi.*0.8).^2; % pi(1)/pi(10)=2.08

%y = 1 - x.^4;
%yi = 1 - xi.^4; % pi(1)/pi(10)=2.9
%y = 1 - x.^3;
%yi = 1 - xi.^3; % pi(1)/pi(10)=3.69

%y = 1 - (0.9.*(x./1));
%yi = 1 - (0.9.*(i./10)); % pi(1)/pi(10)=5.26
%y = 1 - (0.5.*(x./1));
%yi = 1 - (0.5.*(i./10)); % pi(1)/pi(10)=1.82

pi = yi./sum(yi);

Z = [x' y'];
Zi = [xi' yi'];
P = [xi' pi'];
pi(1)/pi(10)

gplot Z, Zi with impulses, P
