1;

%
% Computes the egcd of (a,b)
% Carlos Torchia - 2010-11-27
%

function z = egcd(a,b)

	if(b == 0)
		z = [1,0];
	else
		w = egcd(b,mod(a,b));
		z = [ w(2), w(1) - w(2)*floor(a/b) ];
	endif

endfunction
