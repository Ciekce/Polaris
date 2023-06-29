/*
 * Polaris, a UCI chess engine
 * Copyright (C) 2023 Ciekce
 *
 * Polaris is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Polaris is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Polaris. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "../types.h"

#include <array>
#include <vector>
#include <utility>
#include <span>
#include <cstring>
#include <algorithm>

#include "../core.h"
#include "../position/boards.h"

namespace polaris::eval::nnue
{
	constexpr usize InputSize = 768;
	constexpr usize Layer1Size = 384;

	constexpr Score CReluMin = 0;
	constexpr Score CReluMax = 255;

	constexpr Score Scale = 400;

	constexpr Score Qa = 255;
	constexpr Score Qb = 64;

	constexpr Score Qab = Qa * Qb;

	struct alignas(64) NnueParams
	{
		std::array<i16, InputSize * Layer1Size> featureWeights;
		std::array<i16, Layer1Size> featureBias;
		std::array<i16, Layer1Size * 2> outputWeights;
		i16 outputBias;
	};

	extern const NnueParams &g_nnue;

	template <usize HiddenSize>
	struct alignas(64) Accumulator
	{
		std::array<i16, HiddenSize> black;
		std::array<i16, HiddenSize> white;

		inline auto init(std::span<const i16, HiddenSize> bias)
		{
			std::memcpy(black.data(), bias.data(), bias.size_bytes());
			std::memcpy(white.data(), bias.data(), bias.size_bytes());
		}
	};

	constexpr auto crelu(i16 x)
	{
		return std::clamp(static_cast<i32>(x), CReluMin, CReluMax);
	}

	class NnueState
	{
	public:
		explicit NnueState(const NnueParams &net = g_nnue)
			: m_net{net}
		{
			m_accumulatorStack.reserve(256);
		}

		~NnueState() = default;

		inline auto push()
		{
			m_accumulatorStack.push_back(*m_curr);
			m_curr = &m_accumulatorStack.back();
		}

		inline auto pop()
		{
			m_accumulatorStack.pop_back();
			m_curr = &m_accumulatorStack.back();
		}

		inline auto reset(const PositionBoards &boards)
		{
			m_accumulatorStack.clear();
			m_curr = &m_accumulatorStack.emplace_back();

			m_curr->init(m_net.featureBias);

			for (usize pieceIdx = 0; pieceIdx < 12; ++pieceIdx)
			{
				const auto piece = static_cast<Piece>(pieceIdx);
				auto board = boards.forPiece(piece);
				while (!board.empty())
				{
					const auto sq = board.popLowestSquare();
					updateFeature<true>(piece, sq);
				}
			}
		}

		inline auto moveFeature(Piece piece, Square src, Square dst)
		{
			const auto [blackSrc, whiteSrc] = featureIndices(piece, src);
			const auto [blackDst, whiteDst] = featureIndices(piece, dst);

			subtractAndAddToAll(m_curr->black, m_net.featureWeights, blackSrc * Layer1Size, blackDst * Layer1Size);
			subtractAndAddToAll(m_curr->white, m_net.featureWeights, whiteSrc * Layer1Size, whiteDst * Layer1Size);
		}

		template <bool Activate>
		inline auto updateFeature(Piece piece, Square sq) -> void
		{
			const auto [blackIdx, whiteIdx] = featureIndices(piece, sq);

			if constexpr (Activate)
			{
				addToAll(m_curr->black, m_net.featureWeights, blackIdx * Layer1Size);
				addToAll(m_curr->white, m_net.featureWeights, whiteIdx * Layer1Size);
			}
			else
			{
				subtractFromAll(m_curr->black, m_net.featureWeights, blackIdx * Layer1Size);
				subtractFromAll(m_curr->white, m_net.featureWeights, whiteIdx * Layer1Size);
			}
		}

		[[nodiscard]] inline auto evaluate(Color stm) const
		{
			const auto output = stm == Color::Black
				? screluFlatten(m_curr->black, m_curr->white, m_net.outputWeights)
				: screluFlatten(m_curr->white, m_curr->black, m_net.outputWeights);
			return (output + m_net.outputBias) * Scale / Qab;
		}

	private:
		const NnueParams &m_net;

		std::vector<Accumulator<Layer1Size>> m_accumulatorStack{};
		Accumulator<Layer1Size> *m_curr{};

		template <usize Size, usize Weights>
		static inline auto subtractAndAddToAll(std::array<i16, Size> &input,
			const std::array<i16, Weights> &delta, usize subOffset, usize addOffset) -> void
		{
			for (usize i = 0; i < Size; ++i)
			{
				input[i] += delta[addOffset + i] - delta[subOffset + i];
			}
		}

		template <usize Size, usize Weights>
		static inline auto addToAll(std::array<i16, Size> &input,
			const std::array<i16, Weights> &delta, usize offset) -> void
		{
			for (usize i = 0; i < Size; ++i)
			{
				input[i] += delta[offset + i];
			}
		}

		template <usize Size, usize Weights>
		static inline auto subtractFromAll(std::array<i16, Size> &input,
			const std::array<i16, Weights> &delta, usize offset) -> void
		{
			for (usize i = 0; i < Size; ++i)
			{
				input[i] -= delta[offset + i];
			}
		}

		static inline auto featureIndices(Piece piece, Square sq) -> std::pair<usize, usize>
		{
			constexpr usize ColorStride = 64 * 6;
			constexpr usize PieceStride = 64;

			const auto base = static_cast<usize>(basePiece(piece));
			const usize color = pieceColor(piece) == Color::White ? 0 : 1;

			const auto blackIdx = !color * ColorStride + base * PieceStride + (static_cast<usize>(sq) ^ 0x38);
			const auto whiteIdx =  color * ColorStride + base * PieceStride +  static_cast<usize>(sq)        ;

			return {blackIdx, whiteIdx};
		}

		static inline auto screluFlatten(const std::array<i16, Layer1Size> &us,
			const std::array<i16, Layer1Size> &them, const std::array<i16, Layer1Size * 2> &weights) -> Score
		{
			i32 sum = 0;

			for (usize i = 0; i < Layer1Size; ++i)
			{
				sum += crelu(  us[i]) * weights[             i];
				sum += crelu(them[i]) * weights[Layer1Size + i];
			}

			return sum;
		}
	};
}