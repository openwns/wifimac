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


%%%%%%%%%%%%%  Plot out PER curves of 802.11n PHY MCSs %%%%%%%%%%%%%%
% This script plots out the PER curves of .11n MCSs 
clear all;
%%%%%%% Definition of constants %%%%%%%
% Initiate the Pre_SNR_dB vector (dB)
Pre_SNR_dB = -10:0.2:50; 
% Packet size in Byte
%L = 65535; 
L = 1000;
% Transmitting and receiving antenna number
% Mt = [1,1,1,1,2,2,2,3,3,4];
% Mr = [1,2,3,4,2,3,4,3,4,4];
Mt = [1];
Mr = [4];
% Modulation & Coding Scheme index 
MCS = [1,2,3,4,5,6,7,8];

M_max = size(Mt,2);
% Call PER_11n_MIMO to calculate PER, BER and SER
% Choose ZF or MMSE receiver in PER_11n_MIMO 
[PER, BER, FER, SER] = PER_11n_MIMO(Pre_SNR_dB, L, Mt, Mr, MCS);
% PER(SNR_i, L_i, M_i, MCS_i): the packet error probability
% BER(SNR_i, L_i, M_i, MCS_i): the (raw) bit error probability
% FER(SNR_i, L_i, M_i, MCS_i): the first error event probability
% SER(SNR_i, L_i, M_i, MCS_i): the symbol error probability


%%%%%%% Plot PER curves %%%%%%
clf;
for i = 1 : M_max
    figure(i); 
        semilogy(Pre_SNR_dB,PER(:,1,i,1),'-',Pre_SNR_dB,PER(:,1,i,2),'--',Pre_SNR_dB,PER(:,1,i,3),'-',Pre_SNR_dB,PER(:,1,i,4),'--', ...
             Pre_SNR_dB,PER(:,1,i,5),'-',Pre_SNR_dB,PER(:,1,i,6),'--',Pre_SNR_dB,PER(:,1,i,7),'-',Pre_SNR_dB,PER(:,1,i,8),'--')
    axis([min(Pre_SNR_dB) max(Pre_SNR_dB) 0.000001 1]);
    grid on;
    xlabel('Pre-processing SNR (dB)');
    ylabel('Packet Error Ratio (PER)');
    title(['802.11n PER with ZF Receiver, Packet size:', num2str(L(1)), ', Mt=', num2str(Mt(i)), ', Mr=', num2str(Mr(i))]);
    legend('BPSK1/2','QPSK1/2','QPSK3/4','16-QAM1/2',...
    '16-QAM3/4','64-QAM2/3','64-QAM3/4','64-QAM5/6',1)
end

for i = 1 : M_max
    figure(i + M_max); 
        semilogy(Pre_SNR_dB,BER(:,1,i,1),'-',Pre_SNR_dB,BER(:,1,i,2),'--',Pre_SNR_dB,BER(:,1,i,4),'-',Pre_SNR_dB,BER(:,1,i,6),'--')
    axis([min(Pre_SNR_dB) max(Pre_SNR_dB) 0.000001 1]);
    grid on;
    xlabel('Pre-processing SNR (dB)');
    ylabel('Raw Bit Error Ratio before Decoding (BER)');
    title(['802.11n BER with ZF Receiver', ', Mt=', num2str(Mt(i)), ', Mr=', num2str(Mr(i))]);
    legend('BPSK','QPSK','16-QAM','64-QAM',1)
end

for i = 1 : M_max
    figure(i + 2*M_max); 
        semilogy(Pre_SNR_dB,SER(:,1,i,1),'-',Pre_SNR_dB,SER(:,1,i,2),'--',Pre_SNR_dB,SER(:,1,i,3),'-',Pre_SNR_dB,SER(:,1,i,4),'--', ...
             Pre_SNR_dB,SER(:,1,i,5),'-',Pre_SNR_dB,SER(:,1,i,6),'--',Pre_SNR_dB,SER(:,1,i,7),'-',Pre_SNR_dB,SER(:,1,i,8),'--')
    axis([min(Pre_SNR_dB) max(Pre_SNR_dB) 0.000001 1]);
    grid on;
    xlabel('Pre-processing SNR (dB)');
    ylabel('Symbol Error Ratio (SER)');
    title(['802.11n SER with ZF Receiver, Packet size:', num2str(L(1)), ', Mt=', num2str(Mt(i)), ', Mr=', num2str(Mr(i))]);
    legend('BPSK','QPSK','16-QAM', '64-QAM')
end

