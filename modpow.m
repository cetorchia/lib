1;

%
% Calculates x^y mod n
%
% Carlos Torchia 2010-11-27
% With help from wikipedia.org/wiki/Modular_exponentiation
%

function z = modpow(x,y,n)

	z = 1;

	while y > 0

		if (mod(y,2) == 1)
			z = mod(z * x, n); % Add a power of 2 to the exponent for this bit
		endif

		y = floor(y/2);
		x = mod(x * x, n);

	endwhile

endfunction
