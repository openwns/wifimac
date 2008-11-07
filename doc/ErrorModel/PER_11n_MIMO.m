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

%%%%%%%%%%%%% function PER_11n_MIMO(Pre_SNR_dB, L)%%%%%%%%%%%%%%
% This function calculate the Packet Error Ratio (PER), Bit Error Ratio
% (BER) and Symbol Error Ratio (SER) of 802.11n PHY with MIMO channel H
% (MrxMt) 

function [PER_11n_MIMO, RAWBER_11n_MIMO, FER_11n_MIMO, SER_11n_MIMO] = PER_11n_MIMO(Pre_SNR_dB, L, Mt, Mr, MCS)

%%% Input parameters %%%
% Pre_SNR_dB: vector of pre-processing SNR (dB);
% L: vector of packet body size (Byte);
% Mt: vector of transmitting antenna number; 
% Mr: vector of receivering antenna number; (Mt and Mr should be of the
%     samesize, and Mt(i)<=Mr(i))
% MCS: vector of the IEEE 802.11n MCS index:
% 1:    BPSK_12
% 2:    QPSK_12
% 3:    QPSK_34
% 4:    16QAM_12
% 5:    16QAM_34
% 6:    64QAM_23
% 7:    64QAM_34
% 8:    64QAM_56

%%% Return value %%%
% PER_11n_MIMO(SNR_i, L_i, Mt_i, MCS_i): the packet error probability of
% given SNR(SNR_i), L(L_i), Mt(Mt_i), Mr(Mr_i) and MCS(MCS_i);
% RAWBER_11n_MIMO(SNR_i, L_i, Mt_i, MCS_i): the raw bit error probability of
% given SNR(SNR_i), L(L_i), Mt(Mt_i), Mr(Mr_i) and MCS(MCS_i);
% FER_11n_MIMO(SNR_i, L_i, Mt_i, MCS_i): the first error probability of
% given SNR(SNR_i), L(L_i), Mt(Mt_i), Mr(Mr_i) and MCS(MCS_i);
% SER_11n_MIMO(SNR_i, L_i, Mt_i, MCS_i): the symbol error probability of
% given SNR(SNR_i), L(L_i), Mt(Mt_i), Mr(Mr_i) and MCS(MCS_i);

%%% External functions used in this function %%%
% function [Post_SNR_dB] = post_SNR_Hw_ZF(Pre_SNR_dB, Mt, Mr) or
% function [Post_SNT_dB] = post_SNT_Hw_MMSE(Pre_SNR_dB, Mt, Mr)
% function [P_me, P_ber, P_ser] = PER_11n(Post_SNR_dB, L)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% By Yunpeng Zang
% ComNets, RWTH-Aachen, Germany
% 2008-04-29
% last modified 2008-06-12
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%% Contants %%%

Size_SNR = size(Pre_SNR_dB,2); % Get the size of SNR vactor

L_max = size(L,2); % The number of packet length to be calculated

M_max = size(Mt, 2); % The max. number of antennas at Rx and Tx supported in .11n PHY

MCS_max = size(MCS,2); % The total number of MCSs given in MCS:

%%% Init variables %%%

PER_11n_MIMO = ones(Size_SNR, L_max, M_max, MCS_max); % Init PER_MIMO to 1.0 
RAWBER_11n_MIMO = ones(Size_SNR, L_max, M_max, MCS_max); % Init BER_MIMO to 1.0 
FER_11n_MIMO = ones(Size_SNR, L_max, M_max, MCS_max); % Init FER_MIMO to 1.0 
SER_11n_MIMO = ones(Size_SNR, L_max, M_max, MCS_max); % Init SER_MIMO to 1.0

%%%%%%%%%%%%%%%%%
% First we convert the pre-processing SNR to post-processing SNR 
% see the comments in Post_SNR_Hw_ZF/MMSE for assumptions
% the use of real() is to remove the 0 imaminary part of the result
% ZF receiver is used:
%Post_SNR_dB = real(Post_SNR_Hw_ZF(Pre_SNR_dB, Mt, Mr)); % get a Mt-by-Size_SNR matrix
% ZF receiver is used and the calcualtion is based on the deterministic model of post-SNR :
Post_SNR_dB = real(Post_SNR_Hw_ZF_Determ(Pre_SNR_dB, Mt, Mr)); 
% MMSE receiver is used:
%Post_SNR_dB = real(Post_SNR_Hw_MMSE(Pre_SNR_dB, Mt, Mr)); % get a Mt-by-Size_SNR matrix

for L_i = 1 : L_max % we calcualte for eath packet length value
    for M_i = 1 : M_max % we calcualte PER,BER,SER for each Mt,Mr combination
        for MCS_i = 1: MCS_max % we calcualte for each MCS
            [PER_11n_MIMO(:,L_i,M_i,MCS_i), ...
                RAWBER_11n_MIMO(:,L_i,M_i,MCS_i), ...
                FER_11n_MIMO(:,L_i,M_i,MCS_i), ...
                SER_11n_MIMO(:,L_i,M_i,MCS_i)] = PER_11n(Post_SNR_dB(M_i,:), L(L_i), MCS(MCS_i));
            for SNR_i = 1:Size_SNR % here we filt out the -inf values and replace it with 1
                if PER_11n_MIMO(SNR_i,L_i,M_i,MCS_i) < 0
                    PER_11n_MIMO(SNR_i,L_i,M_i,MCS_i) = 1;
                end
            end
        end
    end
end

