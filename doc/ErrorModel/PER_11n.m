%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% This file is part of openWNS (open Wireless Network Simulator)
% _____________________________________________________________________________
%
% Copyright (C) 2004-2008
% Chair of Communication Networks (ComNets)
% Kopernikusstr. 16, D-52074 Aachen, Germany
% phone: ++49-241-80-27910,
% fax: ++49-241-80-22242
% email: info@openwns.org
% www: http://www.openwns.org
% _____________________________________________________________________________
%
% openWNS is free software; you can redistribute it and/or modify it under the
% terms of the GNU Lesser General Public License version 2 as published by the
% Free Software Foundation;
%
% openWNS is distributed in the hope that it will be useful, but WITHOUT ANY
% WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
% A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
% details.
%
% You should have received a copy of the GNU Lesser General Public License
% along with this program.  If not, see <http://www.gnu.org/licenses/>.
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%% function PER_11n(Post_SNR_dB, L)%%%%%%%%%%%%%%
% This function calculates the 
% PER (Packet Error Ratio), 
% BER (Bit Error Ratio)
% SER (Symbol Error Ratio)
% with given SNR array and packet length L
% The calculation process:
% Post_SNR -> SER (of each modulation) -> P_d (pairwise comparison error
% probability) -> P_u (first event error probability of MSC) -> P_me
% (upper bound of PER for packet length L)
% The channel coding scheme is convolutional coding with constrain length
% 7, rate 1/2 and punchtured rate 2/3, 3/4 and 5/6 using generator polynom
% g133 and g171. 

function [P_me, P_ber, P_u, P_ser] = PER_11n(Post_SNR_dB, L, MCS)

%%% parameters:
% Pre_SNR_dB; % Vector of post-processing SNR=E/N0(dB), (1-by-SNR) vector;
% L; % Packet size in B 
% MCS; MCS index bumber [1..8]

%%% return values:
% P_me(MSC, SNR): the Packet Error Ratio of all SNR
% P_ber(MSC, SNR): the Bit Error Ratio of all SNR
% P_ser(MSC, SNR): the Symbol Error Ratio of all SNR


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% By G. Orfanos and Y. Zang
% ComNets, RWTH-Aachen, Germany
% 2008-04-28
% Last modified on 2008-06-11
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%% Constant value %%%%%%%%%%%%%%%%%%%%%%
% MCS index:
% 1:    BPSK_12
% 2:    QPSK_12
% 3:    QPSK_34
% 4:    16QAM_12
% 5:    16QAM_34
% 6:    64QAM_23
% 7:    64QAM_34
% 8:    64QAM_56
% 
Post_SNR = 10.^(Post_SNR_dB./10); % convert SNR from dB to ratio

SNR = Post_SNR.*0.8; % Reduction due to cyclic prefix 
                     % for 800ns GI: 0.8
                     % for 400ns GI: 0.9
switch MCS
    case 1
        %%% For BPSK modulation:
        P_ser = Q(sqrt(2.*SNR)); % symbol error ratio of BPSK
        P_ber = P_ser;           % for BPSK P_ser = P_ber
        % PER BPSK_12
        P_u = Pu12(P_ber); % First event error probability of BPSK_12
        P_me = Pe(P_u, L); % PER for BPSK_12
        % % PER BPSK_34 ** this mode is no longer supported by 802.11n
        % P_u = Pu34(P_ber); % First event error probability of BPSK_34
        % P_me = Pe(P_u, L); % PER for BPSK_34

    case 2
        %%% For QPSK modulation
        %%P_ber = Q(sqrt(SNR));   % bit error ratio of QPSK
        %%P_ser = 2*P_ber;
        P_ser = 2.*Q(sqrt(SNR)).*(1-0.5.*Q(sqrt(SNR)));
        P_ber = 0.5.*P_ser;
        % PER QPSK_12
        P_u = Pu12(P_ber); % 
        P_me = Pe(P_u, L);
    case 3
        %%% For QPSK modulation
        %%P_ber = Q(sqrt(SNR));   % bit error ratio of QPSK
        %%P_ser = 2*P_ber;
        P_ser = 2.*Q(sqrt(SNR)).*(1-0.5.*Q(sqrt(SNR)));
        P_ber = 0.5.*P_ser;        
        % PER QPSK_34
        P_u = Pu34(P_ber);
        P_me = Pe(P_u, L);
    case 4
        %%% For 16QAM
        P_sqrt16 = 3/2 .* Q(sqrt((3/15).*SNR)); 
        P_ser = 1 - (1 - P_sqrt16).^2;  % Symbol error ratio
        P_ber = P_ser ./ 4; % BER of 16QAM
        % PER 16QAM_12
        P_u = Pu12(P_ber);
        P_me = Pe(P_u,L);
    case 5 
        %%% For 16QAM
        P_sqrt16 = 3/2 .* Q(sqrt((3/15).*SNR)); 
        P_ser = 1 - (1 - P_sqrt16).^2;  % Symbol error ratio
        P_ber = P_ser ./ 4; % BER of 16QAM
        % PER 16QAM_34
        P_u = Pu34(P_ber);
        P_me = Pe(P_u,L);
    case 6
        %%% For 64QAM
        P_sqrt64 = 7/4 .* Q(sqrt((3/63).*SNR)); 
        P_ser = 1 - (1 - P_sqrt64).^2;
        P_ber = P_ser ./ 6; % BER of 64QAM
        % PER 64QAM_23
        P_u = Pu23(P_ber);
        P_me = Pe(P_u,L);
    case 7 
        %%% For 64QAM
        P_sqrt64 = 7/4 .* Q(sqrt((3/63).*SNR)); 
        P_ser = 1 - (1 - P_sqrt64).^2;
        P_ber = P_ser ./ 6; % BER of 64QAM
        % PER 64QAM_34
        P_u = Pu34(P_ber);
        P_me = Pe(P_u,L);
    case 8
        %%% For 64QAM
        P_sqrt64 = 7/4 .* Q(sqrt((3/63).*SNR)); 
        P_ser = 1 - (1 - P_sqrt64).^2;
        P_ber = P_ser ./ 6; % BER of 64QAM
        % PER 64QAM_56
        P_u = Pu56(P_ber);
        P_me = Pe(P_u,L);
end

%%%%%%%%%%%%%% Function definitions  %%%%%%%%%%%%%%%%%%%%%%

% Q-Fuction definded in [Proakis95]
function a = Q(x)
    a = erfc(x./(sqrt(2))) ./ 2;

% The number of ways to select k out of d
function a = bnc(d,k)
    a = factorial(d) / factorial(k) / factorial(d-k);

% Calcualte Probability of error in pairwise comparison of two paths that
% differ in d bits, p is the vector of bit error probability:
function b = Pd(p,d)
    b = (4.*p.*(1-p)).^(d/2);
    return;
    if floor(d/2) == d/2
        n = d/2 + 1;
    else
        n = (d+1)/2;
    end
    b = 0;
    for k=n:d
        b = b + bnc(d,k) * p.^k .* (1-p).^(d-k);
    end
    if floor(d/2) == d/2
        b = b + bnc(d,d/2) * (p.^(d/2) .* (1-p).^(d/2)) / 2;
    end
    % Chernoff-Approximation
    % b = (4*p*(1-p))^(d/2);

% Calculate PER upper bound of packets with length l with first error
% probability of p:
function c = Pe(p,l)
    c = 1 - (1-p) .^ (8*l);

% Calculate first event error prob of coding rate 1/2
function d = Pu12(p)
    d = 11 * Pd(p,10) + 38 * Pd(p,12) + 193 * Pd(p,14) + 1331 * Pd(p,16) + ...
        7275 * Pd(p,18) + 40406 * Pd(p,20) + 234969 * Pd(p,22);

% Calculate first event error prob. of coding rate 2/3
function e = Pu23(p)
    e = Pd(p,6) + 16 * Pd(p,7) +  48 * Pd (p,8) + 158 * Pd(p,9) + 642 * Pd(p,10) + ...
        2435 * Pd(p,11) + 6174 * Pd(p,12) + 34705 * Pd(p,13) + 131585 * Pd(p,14) + 499608 * Pd(p,15);

% Calculate first event error prob. of coding rate 3/4
function f = Pu34(p)
    f = 8 * Pd(p,5) + 31 * Pd(p,6) + 160 * Pd(p,7) + 892 * Pd(p,8) + 4512 * Pd(p,9) + 23307 * Pd(p,10) + ...
        121077 * Pd(p,11) + 625059 * Pd(p,12) + 3234886 * Pd(p,13) + 16753077 * Pd(p,14);

% Calculate first event error prob. of coding rate 5/6
function g = Pu56(p)
    g = 14*Pd(p,4) + 69*Pd(p,5) + 654*Pd(p,6) + 4996*Pd(p,7) + 39677*Pd(p,8) + ... 
        314973*Pd(p,9) + 2503576*Pd(p,10) + 19875546*Pd(p,11);

      
