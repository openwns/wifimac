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

%%%%%%%%%%%%% function post_SNR_Hw_ZF_Determ(Pre_SNR, Mt, Mr)%%%%%%%%%%%%%%
% This function implements the SNR processing at the receiver for each
% spatial stream using ZF detection and returns the post-processing SNR,
% Post_SNR, corresponding to the pre-processing SNR values.

% The Caculation is based on the following assumptions
% 1. The MIMO channel model is block fading Guassian matrix representing
% the flat Rayleigh fading MIMO channel
% 2. The Zero-Forcing detect is used at the receiver and 
% the post SNR of kth stream is expressed as [A. Paulraj 2003]
% Deterministic model based on the Chi-quared distribution of Post-SNR is
% used in this function 
function [Post_SNR_dB]=Post_SNR_Hw_ZF_Determ(Pre_SNR_dB, Mt, Mr)

%%% parameters:
% Pre_SNR_dB; % Vector of pre-processing SNR=E/N0(dB), overall on all streams;
% Mt; % Vector of transmit antenna number;
% Mr; % Vector of receiver antenna number, (Mt(i)<=Mr(i), Mt and Mr are
% vectors of the same size)

%%% return values:
% Post_SNR_dB(k,:): the per stream post-processing SNR vector corresponding to
% the Pre_SNR_dB vector for the kth Mt-Mr antenna combination.

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% By Yunpeng Zang
% ComNets, RWTH-Aachen, Germany
% 2008-04-25
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%%%%%%%%%%%%% Constant value %%%%%%%%%%%%%%%%%%%%%%
n_large = 1; % a large enough value lets the the average post_processing 
                 %SNR converge to mean value acoording to the chi-square distribution 

%%%%%%%%%%%%% Variables %%%%%%%%%%%%%%%%%%%%%%%%%%%
Post_SNR = zeros(size(Mt,2),size(Pre_SNR_dB,2)); % initiate the Post_SNR Matrix

%%%%%%%%%%%%% Channel Matrix %%%%%%%%%%%%%%%%%%%%%%
Pre_SNR = 10.^(Pre_SNR_dB./10); % Convert the SNR from dB to ratio
%H_sc_sum = zeros(Mt,Mt); % for debugging the chi-square distribution
for k = 1 : size(Mt,2)
%     for i = 1:n_large % we need the average Post_SNR over n_large
%         % Rayleigh flat fading channel matrix with all elements being
%         % independent complex Gaussian variables with zero mean and unit
%         % variance:
%         H = randn(Mr(k), Mt(k))*(2^(-1/2))+j.*randn(Mr(k), Mt(k))*(2^(-1/2));
%         % H_array(:,:,i) = H; % for debugging, we collect all channel matrices 
%         H_ctrans = ctranspose(H); % Get the conjugate transpose of H      
%         H_Mt = H_ctrans*H; % and then left mutiplies H, get a MtxMt matrix
%         H_sc = inv(H_Mt); % inverse the MtxMt metrix
%         % here we calculate the post SNR for the first stream and
%         % accumulate it to Post_SNR, (Post_SNR of each stream is i.i.d)
%         Post_SNR(k,:) = Post_SNR(k,:)+(Pre_SNR./Mt(k))./(H_sc(1,1)); 
%         % H_sc_sum = H_sc_sum + H_sc; % for debugging
%     end
Post_SNR(k,:) = (Mr(k) - Mt(k) + 1).* (Pre_SNR./Mt(k)); % according to the Chi-squared distribution
end 
%H_sc_sum  % for debugging
%H_sc_avg = H_sc_sum ./n_large % for debugging the average processing gain
%Var_H = var(H_array(1,1,:)) % for debugging the variance of H elements and
                             % this should be always close to 1.00.

% Get the mean value of the post-process SNR and convert it to dB
% the processing gain follows the Chi-Square distribution with freedom of
% (Mr-Mt+1), therefore the mean valuse of processing gain should converge
% to Pre_SNR*(Mr-Mt+1) for each stream:
Post_SNR_dB = 10*log10(Post_SNR./n_large); 
