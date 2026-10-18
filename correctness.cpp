#include <cmath>
#include <cstdio>

#include "src/jackel.h"
#include "src/jackel.cpp"

int main()
{
	constexpr U64 N = 161;
	constexpr F64 Forward = 100.0;
	constexpr F64 Expiry = 1.0;
	constexpr F64 Tolerance = 1e-10;
	const option_type Types[] = {Option_Call, Option_Put};
	F64 MaxError[2] = {};
	U64 Failures = 0;

	std::printf("forward,expiry,strike,input_iv,call_iv,put_iv\n");
	for (U64 I = 0; I < N; ++I)
	{
		const F64 Strike = 60.0 + 80.0 * I / (N - 1);
		const F64 M = std::log(Strike / Forward);
		const F64 Sigma = 0.20 - 0.10 * M + 0.40 * M * M;
		const F64 S = Sigma * std::sqrt(Expiry);
		const F64 D1 = -M / S + 0.5 * S;
		const F64 D2 = D1 - S;
		F64 Vols[2];
		for (U64 J = 0; J < 2; ++J)
		{
			const F64 Q = Types[J];
			const F64 Price = Q * (
				Forward * 0.5 * std::erfc(-Q * D1 / std::sqrt(2.0)) -
				Strike * 0.5 * std::erfc(-Q * D2 / std::sqrt(2.0)));
			Vols[J] = JackelImpliedVolatility(Price, Forward, Strike, Expiry, Types[J]);
			const F64 Error = std::isfinite(Vols[J]) ? std::fabs(Vols[J] - Sigma) : INFINITY;
			if (Error > MaxError[J]) { MaxError[J] = Error; }
			if (Error > Tolerance) { ++Failures; }
		}
		std::printf("%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n",
			Forward, Expiry, Strike, Sigma, Vols[0], Vols[1]);
	}
	std::fprintf(stderr, "%s: %llu / %llu options within %.1e absolute IV error\n",
		Failures ? "FAIL" : "PASS", static_cast<unsigned long long>(2 * N - Failures),
		static_cast<unsigned long long>(2 * N), Tolerance);
	std::fprintf(stderr, "Max absolute IV error: calls %.3e | puts %.3e\n",
		MaxError[0], MaxError[1]);
	return Failures ? 1 : 0;
}
