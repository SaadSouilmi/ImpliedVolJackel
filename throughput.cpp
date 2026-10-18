#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

#include "base.h"
#include "jackel.h"
#include "profile.h"

#include "jackel.cpp"
#include "profile.cpp"

int main()
{
	constexpr U64 N = 65536;
	constexpr U64 Repeats = 100;
	constexpr F64 Forward = 100.0;
	constexpr F64 Expiry = 1.0;
	std::vector<F64> Prices(N), Strikes(N), Vols(N);
	std::vector<option_type> Types(N);
	std::mt19937 Random(42);
	std::uniform_real_distribution<F64> Strike(80.0, 120.0);
	std::uniform_real_distribution<F64> Sigma(0.1, 0.8);

	for (U64 I = 0; I < N; ++I)
	{
		Strikes[I] = Strike(Random);
		Types[I] = I % 2 ? Option_Put : Option_Call;
		const F64 S = Sigma(Random) * std::sqrt(Expiry);
		const F64 D1 = std::log(Forward / Strikes[I]) / S + 0.5 * S;
		const F64 D2 = D1 - S;
		const F64 Q = Types[I];
		Prices[I] = Q * (Forward * 0.5 * std::erfc(-Q * D1 / std::sqrt(2.0)) -
			Strikes[I] * 0.5 * std::erfc(-Q * D2 / std::sqrt(2.0)));
	}

	const auto Batch = [&]()
	{
		JackelImpliedVolVec(Prices.data(), Strikes.data(), Types.data(),
			Forward, Expiry, Vols.data(), N);
		// Keep every batch observable to the optimizer (GCC/Clang).
		asm volatile("" : : "g"(Vols.data()) : "memory");
	};
	GetTSCCalibration();
	Batch();

	profile_stats Stats = {};
	TimeFunctionN(Stats, Repeats, Batch(), false, TimeUnit_Ns);
	const F64 SecondsPerTick = TSCToWall(1, TimeUnit_Sec);
	const F64 Seconds = Stats.AvgTicks * Stats.Count * SecondsPerTick;
	const F64 StdNanosPerOption = Stats.StdTicks * SecondsPerTick * 1e9 / N;

	for (F64 Vol : Vols)
	{
		if (!std::isfinite(Vol) || Vol <= 0 || Vol >= 1)
		{
			std::fprintf(stderr, "Invalid implied volatility: %.17g\n", Vol);
			return 1;
		}
	}
	const F64 Count = static_cast<F64>(N * Stats.Count);
	std::printf("%.2f M options/s | %.1f +/- %.1f ns/option (batch mean +/- SD)\n",
		Count / Seconds / 1e6, Seconds * 1e9 / Count, StdNanosPerOption);
	std::printf("%llu options in %.3f s\n",
		static_cast<unsigned long long>(N * Stats.Count), Seconds);
}
