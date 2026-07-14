#include "Aimbot.hpp"

#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <random>

#include "Settings.hpp"

namespace {

static std::chrono::steady_clock::time_point s_lastStep{};

class HumanizedAimbot
{
public:
	HumanizedAimbot()
		: m_rng(static_cast<std::mt19937::result_type>(
			std::chrono::steady_clock::now().time_since_epoch().count()))
		, m_unit(-1.0f, 1.0f)
	{
		RenewPersonality();
	}

	void Reset() {
		m_hasFlick = false;
		m_previousDelta = { 0.0f, 0.0f };
		m_reactionRemaining = 0.0f;
		RenewPersonality();
	}

	std::pair<float, float> Step(float dx, float dy) {
		float dt = MeasureDeltaSeconds();
		float distance = std::sqrt(dx * dx + dy * dy);

		if (distance < 0.5f) {
			m_previousDelta = { 0.0f, 0.0f };
			return { 0.0f, 0.0f };
		}

		if (!m_hasFlick) {
			m_hasFlick = true;
			m_reactionRemaining = m_reactionTimeSec;
			RenewPersonality();
		}

		m_personalityTimer += dt;
		if (m_personalityTimer > 0.75f) {
			RenewPersonality();
			m_personalityTimer = 0.0f;
		}

		if (m_reactionRemaining > 0.0f) {
			m_reactionRemaining -= dt;
			return { 0.0f, 0.0f };
		}

	float smooth = (g_AimSmoothness < 0.1f) ? 0.1f : g_AimSmoothness;

	if (g_AimLegitMode == 0) {
		return ClampToMax(dx / smooth, dy / smooth);
	}

	float effSmooth = DynamicSmoothness(distance, smooth) * m_smoothVariance;

	float t = std::min(1.0f, distance / 200.0f);
	float eased = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
	float frac = eased / effSmooth;

	float moveX = dx * frac;
	float moveY = dy * frac;

	if (g_AimLegitMode >= 2 && g_AimHumanize > 0.0f) {
		float invDist = 1.0f / distance;
		float px = -dy * invDist;
		float py =  dx * invDist;
		float jitterAmp = g_AimHumanize * 0.35f * std::sqrt(distance);
		float lateral = m_unit(m_rng) * jitterAmp;
		moveX += px * lateral;
		moveY += py * lateral;

		moveX += m_biasX * g_AimHumanize;
		moveY += m_biasY * g_AimHumanize;

		moveX += m_previousDelta.first  * 0.15f;
		moveY += m_previousDelta.second * 0.15f;

		if (distance < 12.0f) {
			float taper = distance / 12.0f;
			moveX *= taper;
			moveY *= taper;
		}
	}

	auto out = ClampToMax(moveX, moveY);
	m_previousDelta = out;
	return out;
	}

private:

	std::mt19937 m_rng;
	std::uniform_real_distribution<float> m_unit;

	float m_biasX = 0.0f;
	float m_biasY = 0.0f;
	float m_smoothVariance = 1.0f;
	float m_reactionTimeSec = 0.06f;
	float m_personalityTimer = 0.0f;

	bool m_hasFlick = false;
	float m_reactionRemaining = 0.0f;
	std::pair<float, float> m_previousDelta{ 0.0f, 0.0f };

	void RenewPersonality() {
		m_biasX          = m_unit(m_rng) * 0.4f;

		m_biasY          = m_unit(m_rng) * 0.4f;
		m_smoothVariance = 0.9f + (m_unit(m_rng) + 1.0f) * 0.1f;

		m_reactionTimeSec = 0.03f + ((m_unit(m_rng) + 1.0f) * 0.5f) * 0.08f;

	}

	static float DynamicSmoothness(float distance, float baseSmoothness) {
		float closeness = 1.0f - std::min(1.0f, distance / 400.0f);

		return baseSmoothness * (1.0f + 0.4f * closeness);
	}

	static std::pair<float, float> ClampToMax(float x, float y) {
		constexpr float kMax = 250.0f;
		if (x >  kMax) x =  kMax;
		if (x < -kMax) x = -kMax;
		if (y >  kMax) y =  kMax;
		if (y < -kMax) y = -kMax;
		return { x, y };
	}

	static float MeasureDeltaSeconds() {
		auto now = std::chrono::steady_clock::now();
		if (s_lastStep.time_since_epoch().count() == 0) {
			s_lastStep = now;
			return 0.0f;
		}
		float dt = std::chrono::duration<float>(now - s_lastStep).count();
		s_lastStep = now;

		if (dt > 0.25f) dt = 0.25f;
		return dt;
	}
};

static HumanizedAimbot s_aimbot;

}

namespace Aimbot
{
	void Step(float targetX, float targetY, float centerX, float centerY)
	{
		float dx = targetX - centerX;
		float dy = targetY - centerY;

		auto [moveX, moveY] = s_aimbot.Step(dx, dy);
		if (moveX == 0.0f && moveY == 0.0f) return;

		INPUT input = {};
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = MOUSEEVENTF_MOVE;
		input.mi.dx = static_cast<LONG>(moveX);
		input.mi.dy = static_cast<LONG>(moveY);
		SendInput(1, &input, sizeof(INPUT));
	}

	void NoTarget() {
		s_aimbot.Reset();
	}
}