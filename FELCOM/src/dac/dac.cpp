#include "dac.h"

#include <math.h>

#include <debug.h>

namespace
{
constexpr size_t kSineSamples = 256;
uint8_t g_sineTable[kSineSamples];

uint8_t g_dacPin = 25;
size_t g_sampleIndex = 0;
uint32_t g_lastSampleMicros = 0;
uint32_t g_samplePeriodUs = 0;
bool g_dacReady = false;

void buildSineTable()
{
	for (size_t i = 0; i < kSineSamples; ++i)
	{
		const float angle = (2.0f * PI * static_cast<float>(i)) / static_cast<float>(kSineSamples);
		const float normalized = (sinf(angle) + 1.0f) * 127.5f;
		g_sineTable[i] = static_cast<uint8_t>(normalized);
	}
}
} // namespace

void setupDAC(uint8_t pin, float frequencyHz)
{
	if (pin != 25 && pin != 26)
	{
		LOG_ERROR("Invalid DAC pin %u. Falling back to GPIO 25.", pin);
		pin = 25;
	}

	if (frequencyHz <= 0.0f)
	{
		LOG_ERROR("Invalid DAC frequency %.2f Hz. Falling back to 440 Hz.", frequencyHz);
		frequencyHz = 440.0f;
	}

	buildSineTable();

	g_dacPin = pin;
	g_sampleIndex = 0;
	g_lastSampleMicros = micros();
	g_samplePeriodUs = static_cast<uint32_t>(1000000.0f / (frequencyHz * static_cast<float>(kSineSamples)));
	if (g_samplePeriodUs == 0)
	{
		g_samplePeriodUs = 1;
	}

	dacWrite(g_dacPin, 128);
	g_dacReady = true;

	LOG_INFO("DAC sine ready: pin=%u, frequency=%.2f Hz", g_dacPin, frequencyHz);
}

void updateDACSine()
{
	if (!g_dacReady)
	{
		return;
	}

	const uint32_t now = micros();
	while ((now - g_lastSampleMicros) >= g_samplePeriodUs)
	{
		g_lastSampleMicros += g_samplePeriodUs;
		dacWrite(g_dacPin, g_sineTable[g_sampleIndex]);
		g_sampleIndex = (g_sampleIndex + 1) % kSineSamples;
	}
}
