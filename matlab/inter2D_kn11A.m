% This class creates an inter2D with an 11 parameter kinematic model
% Frames:
% 0 - Global reference, X directed along proximal segment, Z upward
% 1 - 
% 2 - 
% 3 -   
% 4 - 
% 5 - Translation along the X4
classdef inter2D_kn11A < inter2D
%     properties (Access = public)
%         pms; 
%     end %public props
    
    methods (Access = public)
        
        function obj = inter2D_kn11A()
            obj@inter2D();
            obj.name = 'kn11A';
            
            obj.pms.tx01 = 806;
            obj.pms.ty01 = -66;
            obj.pms.tz01 = -28;
            obj.pms.ry01 = 0;
            obj.pms.rz01 = -.24;
            obj.pms.ry34 = 0; %rotation at the catheter base
            obj.pms.rz34 = 0; %redundant with alpha near 0
            obj.pms.kAlpha = 1;%alpha gain
            obj.pms.eAlpha = 1;%alpha raised to
            obj.pms.lCath = obj.drw.lCath; %as fabricated
            obj.pms.ry45 = 0; %rotation at the catheter tip
            
            obj.nums.pms = 11;
            obj.nums.qps = 5;
        end
        
        function varargout = forwardK(obj, qp, pm)
            if nargin < 3
                pm = obj.pms;
            end
            %if pms array, translate into struct
            if isstruct(pm)
                pms = pm;
            else
                pms = obj.paramArray2Struct(pm);
            end
            
            %don't divide by 0
            al = (qp(4)*obj.pms.kAlpha)^obj.pms.eAlpha; %allow slope,exponent in the articulation angle
            if al < 1e-3; al = 1e-3; end
            r = pms.lCath/al; %radius of catheter arc
            
            %compute transforms
            H01 = obj.Tx(pms.tx01)*obj.Ty(pms.ty01)*obj.Tz(pms.tz01)*obj.Ry(pms.ry01)*obj.Rz(pms.rz01) * obj.Rx(qp(1)); %prox roll
            H12 = obj.Tx(obj.drw.lProx) * obj.Rz(qp(2)); %pitch
            H23 = obj.Tx(obj.drw.lPtch) * obj.Rx(qp(3)); %roll
            H34 = obj.Tx(obj.drw.lRoll)*obj.Ry(pms.ry34)*obj.Rz(pms.rz34) * obj.Ty(r*(1-cos(al))) * obj.Tx(r*sin(al)) * obj.Rz(al); %ry34 for out-of-articulation plane
            H45 = obj.Ry(pms.ry45) * obj.Tx( qp(5) ); %translation along x4 to the target

            varargout = {H01*H12*H23*H34*H45}; %H05
            if nargout == 5;
                varargout = [{H01}, {H01*H12}, {H01*H12*H23}, {H01*H12*H23*H34}, {H01*H12*H23*H34*H45}]; %return H01,H02,H03,H04,H05
            end
        end %FK
    end %public methods
    
    methods (Access = private)
        
    end %private methods
    
    methods (Static)
        function pms = paramArray2Struct(pma)
            if length(pma) == 11;
                pms.tx01 = pma(1);
                pms.ty01 = pma(2);
                pms.tz01 = pma(3);
                pms.ry01 = pma(4);
                pms.rz01 = pma(5);
                pms.tx23 = pma(6);
                pms.ry34 = pma(7);
                pms.rz34 = pma(8);
                pms.lCath = pma(9);
                pms.ry45 = pma(10);
                pms.rz45 = pma(11);
            else
                error('Matlab:inter2D_kn11A','pma does not have 11 elements');
            end
        end %paramArray2Struct
        function pma = paramStruct2Array(pms)
            if isstruct(pms)
                pma(1,1) = pms.tx01;
                pma(2,1) = pms.ty01;
                pma(3,1) = pms.tz01;
                pma(4,1) = pms.ry01;
                pma(5,1) = pms.rz01;
                pma(6,1) = pms.tx23;
                pma(7,1) = pms.ry34;
                pma(8,1) = pms.rz34;
                pma(9,1) = pms.lCath;
                pma(10,1) = pms.ry45;
                pma(11,1) = pms.rz45;
            else
                error('Matlab:inter2D_kn11A','pms is not a struct');
            end
        end %paramStruct2Array
        
    end %statics
    
    
end