#pragma once

namespace HRZ2
{
	class WeatherSetup;

	enum EWeatherOverrideType : int
	{
		NODEGRAPH_WEATHER_OVERRIDE = 0,
		SEQUENCE_WEATHER_OVERRIDE = 1,
		WEATHER_OVERRIDE_TYPE_COUNT = 2,
	};

	class WeatherSystem
	{
	public:
		void SetWeatherOverride(WeatherSetup *Setup, float TransitionTime, EWeatherOverrideType Type)
		{
			const auto func = Offsets::Signature("48 83 EC 38 C5 F8 28 DA C5 E8 57 D2 C7 44 24 20 00 00 00 00 E8")
								  .ToPointer<void(WeatherSystem *, WeatherSetup *, float, EWeatherOverrideType)>();

			func(this, Setup, TransitionTime, Type);
		}
	};
}
