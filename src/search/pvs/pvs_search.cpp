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

#include "pvs_search.h"

#include <iostream>
#include <algorithm>
#include <cmath>

#include "../../uci.h"
#include "../../movegen.h"
#include "../../eval/eval.h"
#include "../../limit/trivial.h"
#include "../../opts.h"

namespace polaris::search::pvs
{
	namespace
	{
		constexpr f64 MinReportDelay = 1.0;

		constexpr i32 MinAspDepth = 6;

		constexpr Score InitialWindow = eval::cpToInternalUnits(10);
		constexpr Score MaxWindow = eval::cpToInternalUnits(500);

		constexpr i32 MinNullmoveDepth = 3;
		constexpr i32 MinLmrDepth = 3;

		// values from viridithas
		//TODO tune for polaris
		constexpr f64 LmrBase = 0.77;
		constexpr f64 LmrDivisor = 2.36;

		[[nodiscard]] std::array<std::array<i32, 256>, 256> generateLmrTable()
		{
			std::array<std::array<i32, 256>, 256> dst{};

			// neither can be 0
			for (i32 depth = 1; depth < 256; ++depth)
			{
				for (i32 moves = 1; moves < 256; ++moves)
				{
					dst[depth][moves] = static_cast<i32>(LmrBase
						+ std::log(static_cast<f64>(depth)) * std::log(static_cast<f64>(moves)) / LmrDivisor);
				}
			}

			return dst;
		}

		const auto LmrTable = generateLmrTable();

		constexpr i32 MaxSeePruningDepth = 9;

		constexpr Score QuietSeeThreshold = -50;
		constexpr Score NoisySeeThreshold = -90;

		constexpr i32 MinSingularityDepth = 8;

		constexpr i32 MaxFpDepth = 8;

		constexpr Score FpMargin = eval::cpToInternalUnits(250);
		constexpr Score FpScale = eval::cpToInternalUnits(60);

		constexpr i32 MinIirDepth = 4;

		inline Score drawScore(usize nodes)
		{
			return 2 - static_cast<Score>(nodes % 4);
		}
	}

	PvsSearcher::PvsSearcher(std::optional<usize> hashSize)
		: m_table{hashSize ? *hashSize : DefaultHashSize}
	{
		auto &threadData = m_threads.emplace_back();

		threadData.id = m_nextThreadId++;
		threadData.thread = std::thread{[this, &threadData]
		{
			run(threadData);
		}};
	}

	PvsSearcher::~PvsSearcher()
	{
		stop();
		stopThreads();
	}

	void PvsSearcher::newGame()
	{
		m_table.clear();

		for (auto &thread : m_threads)
		{
			thread.pawnCache.clear();
			std::fill(thread.stack.begin(), thread.stack.end(), SearchStackEntry{});
			thread.history.clear();
		}
	}

	void PvsSearcher::startSearch(const Position &pos, i32 maxDepth, std::unique_ptr<limit::ISearchLimiter> limiter)
	{
		if (!limiter)
		{
			std::cerr << "missing limiter" << std::endl;
			return;
		}

		for (auto &thread : m_threads)
		{
			thread.maxDepth = maxDepth;
			thread.search = SearchData{};
			thread.pos = pos;
		}

		m_limiter = std::move(limiter);

		m_stop.store(false, std::memory_order::seq_cst);
		m_runningThreads.store(static_cast<i32>(m_threads.size()));

		m_flag.store(SearchFlag, std::memory_order::seq_cst);
		m_startSignal.notify_all();
	}

	void PvsSearcher::stop()
	{
		m_stop.store(true, std::memory_order::relaxed);
		m_flag.store(IdleFlag, std::memory_order::seq_cst);

		// safe, always runs from uci thread
		if (m_runningThreads.load() > 0)
		{
			std::unique_lock lock{m_stopMutex};
			m_stopSignal.wait(lock, [this]
			{
				return m_runningThreads.load(std::memory_order::seq_cst) == 0;
			});
		}
	}

	void PvsSearcher::runBench(BenchData &data, const Position &pos, i32 depth)
	{
		m_limiter = std::make_unique<limit::InfiniteLimiter>();

		// this struct is a small boulder the size of a large boulder
		// and overflows the stack if not on the heap
		auto threadData = std::make_unique<ThreadData>();

		threadData->pos = pos;
		threadData->maxDepth = depth;

		m_stop.store(false, std::memory_order::seq_cst);

		const auto start = util::g_timer.time();

		searchRoot(*threadData, true);

		const auto time = util::g_timer.time() - start;

		data.search = threadData->search;
		data.time = time;
	}

	bool PvsSearcher::searching()
	{
		std::unique_lock lock{m_searchMutex};
		return m_flag.load(std::memory_order::relaxed) == SearchFlag;
	}

	void PvsSearcher::setThreads(u32 threads)
	{
		if (threads != m_threads.size())
		{
			stopThreads();

			m_flag.store(IdleFlag, std::memory_order::seq_cst);

			m_threads.clear();
			m_threads.shrink_to_fit();
			m_threads.reserve(threads);

			m_nextThreadId = 0;

			for (i32 i = 0; i < threads; ++i)
			{
				auto &threadData = m_threads.emplace_back();

				threadData.id = m_nextThreadId++;
				threadData.thread = std::thread{[this, &threadData]
				{
					run(threadData);
				}};
			}
		}
	}

	void PvsSearcher::clearHash()
	{
		m_table.clear();
	}

	void PvsSearcher::setHashSize(usize size)
	{
		m_table.resize(size);
	}

	void PvsSearcher::stopThreads()
	{
		m_flag.store(QuitFlag, std::memory_order::seq_cst);
		m_startSignal.notify_all();

		for (auto &thread : m_threads)
		{
			thread.thread.join();
		}
	}

	void PvsSearcher::run(ThreadData &data)
	{
		while (true)
		{
			i32 flag{};

			{
				std::unique_lock lock{m_startMutex};
				m_startSignal.wait(lock, [this, &flag]
				{
					flag = m_flag.load(std::memory_order::seq_cst);
					return flag != IdleFlag;
				});
			}

			if (flag == QuitFlag)
				return;

			searchRoot(data, false);
		}
	}

	bool PvsSearcher::shouldStop(const SearchData &data, bool allowSoftTimeout)
	{
		if (m_stop.load(std::memory_order::relaxed))
			return true;

		bool shouldStop = m_limiter->stop(data, allowSoftTimeout);
		return m_stop.fetch_or(shouldStop, std::memory_order::relaxed) || shouldStop;
	}

	void PvsSearcher::searchRoot(ThreadData &data, bool bench)
	{
		auto &searchData = data.search;

		bool shouldReport = !bench && data.id == 0;

		Score score{};
		Move best{};

		const auto startTime = shouldReport ? util::g_timer.time() : 0.0;
		const auto startDepth = 1 + static_cast<i32>(data.id) % 16;

		i32 depthCompleted{};

		for (i32 depth = startDepth; depth <= data.maxDepth && !shouldStop(searchData, true); ++depth)
		{
			searchData.depth = depth;
			searchData.seldepth = 0;

			const auto prevBest = best;

			bool reportThisIter = shouldReport;

			if (depth < MinAspDepth)
			{
				const auto newScore = search(data, depth, 1, -ScoreMax, ScoreMax, false);

				depthCompleted = depth;

				if (depth > 1 && m_stop.load(std::memory_order::relaxed) || !data.search.move)
					break;

				score = newScore;
				best = data.search.move;
			}
			else
			{
				auto aspDepth = depth;

				auto delta = InitialWindow;

				auto alpha = score - delta;
				auto beta = score + delta;

				while (!shouldStop(searchData, false))
				{
					if (aspDepth < depth - 3)
						aspDepth = depth - 3;

					const auto newScore = search(data, aspDepth, 1, alpha, beta, false);

					const bool stop = m_stop.load(std::memory_order::relaxed);
					if (stop || !searchData.move)
					{
						reportThisIter &= !stop;
						break;
					}

					score = newScore;

					if (shouldReport && (score <= alpha || score >= beta))
					{
						const auto time = util::g_timer.time() - startTime;
						if (time > MinReportDelay)
							report(data, data.search.depth, best ?: searchData.move, time, score, alpha, beta);
					}

					delta += delta / 2;

					if (delta > MaxWindow)
						delta = ScoreMate;

					if (score >= beta)
					{
						beta += delta;
						--aspDepth;
					}
					else if (score <= alpha)
					{
						beta = (alpha + beta) / 2;
						alpha = std::max(alpha - delta, -ScoreMate);
						aspDepth = depth;
					}
					else
					{
						best = searchData.move;
						depthCompleted = depth;
						break;
					}
				}
			}

			m_limiter->update(data.search, prevBest == best);

			if (reportThisIter && depth < data.maxDepth)
			{
				if (const auto move = best ?: searchData.move)
					report(data, searchData.depth, move,
						util::g_timer.time() - startTime, score, -ScoreMax, ScoreMax);
				else
				{
					std::cout << "info string no legal moves" << std::endl;
					break;
				}
			}
		}

		if (shouldReport)
		{
			if (!bench)
				m_searchMutex.lock();

			if (const auto move = best ?: searchData.move)
			{
				report(data, depthCompleted, move, util::g_timer.time() - startTime, score, -ScoreMax, ScoreMax);
				std::cout << "bestmove " << uci::moveToString(move) << std::endl;
			}
			else std::cout << "info string no legal moves" << std::endl;
		}

		if (!bench)
		{
			data.history.age();

			--m_runningThreads;
			m_stopSignal.notify_all();

			if (shouldReport)
			{
				m_flag.store(IdleFlag, std::memory_order::relaxed);
				m_searchMutex.unlock();
			}
		}
	}

	Score PvsSearcher::search(ThreadData &data, i32 depth, i32 ply, Score alpha, Score beta, bool cutnode)
	{
		if (depth > 1 && shouldStop(data.search, false))
			return beta;

		auto &pos = data.pos;
		const auto &boards = pos.boards();

		if (ply >= MaxDepth)
			return eval::staticEval(pos);

		const bool inCheck = pos.isCheck();

		if (depth <= 0 && !inCheck)
			return qsearch(data, alpha, beta, ply);

		// check extension
		if (inCheck)
			++depth;

		const auto us = pos.toMove();
		const auto them = oppColor(us);

		const bool root = ply == 1;
		const bool pv = root || beta - alpha > 1;

		auto &stack = data.stack[ply];

		const auto realPly = stack.excluded ? ply - 1 : ply;

		const auto &prevStack = data.stack[realPly - 1];

		if (realPly > data.search.seldepth)
			data.search.seldepth = realPly;

		// mate distance pruning
		if (!pv)
		{
			const auto mdAlpha = std::max(alpha, -ScoreMate + realPly);
			const auto mdBeta = std::min(beta, ScoreMate - realPly - 1);

			if (mdAlpha >= mdBeta)
				return mdAlpha;
		}

		ProbedTTableEntry entry{};
		auto hashMove = NullMove;

		if (!stack.excluded)
		{
			if (m_table.probe(entry, pos.key(), depth, alpha, beta) && !pv)
				return entry.score;
			else if (entry.move && pos.isPseudolegal(entry.move))
				hashMove = entry.move;

			// internal iterative reduction
			if (!hashMove
				&& !inCheck
				&& !stack.excluded
				&& (pv || cutnode)
				&& depth >= MinIirDepth)
				--depth;
		}

		const auto newBaseDepth = depth > 0 ? depth - 1 : 0;
		const auto newPly = ply + 1;

		const bool tableHit = !hashMove.isNull();

		if (!root && !pos.lastMove())
			stack.eval = eval::flipTempo(-prevStack.eval);
		else if (stack.excluded)
			stack.eval = data.stack[ply - 1].eval; // not prevStack
		else stack.eval = inCheck ? 0
			: (entry.score != 0 ? entry.score : eval::staticEval(pos, &data.pawnCache));

		stack.currMove = {};

		const bool improving = !inCheck && realPly > 1 && stack.eval > data.stack[realPly - 2].eval;

		if (!pv && !inCheck && !stack.excluded)
		{
			// reverse futility pruning
			if (depth <= tunable::maxRfpDepth(g_opts.tunable)
				&& stack.eval >= beta + tunable::rfpMargin(g_opts.tunable) * depth / (improving ? 2 : 1))
				return stack.eval;

			const bool nmpFailsLow = tableHit && (entry.type == EntryType::Alpha) && entry.score < beta;

			// nullmove pruning
			if (depth >= MinNullmoveDepth
				&& stack.eval >= beta
				&& !nmpFailsLow
				&& pos.lastMove()
				&& !boards.nonPk(us).empty())
			{
				const auto R = std::min(depth, 3 + depth / 3 + std::min((stack.eval - beta) / 200, 3));

				const auto guard = pos.applyMove(NullMove, &m_table);
				const auto score = -search(data, depth - R, newPly, -beta, -beta + 1, !cutnode);

				if (score >= beta)
				{
					if (score > ScoreWin)
						return beta;
					return score;
				}
			}
		}

		stack.quietsTried.clear();

		const auto prevMove = prevStack.currMove;
		const auto prevPrevMove = ply > 1 ? data.stack[ply - 2].currMove : HistoryMove{};

		auto best = NullMove;
		auto bestScore = -ScoreMax;

		auto entryType = EntryType::Alpha;

		MoveGenerator generator{pos, stack.movegen, hashMove, prevMove, prevPrevMove, &data.history};
		u32 legalMoves = 0;

		while (const auto move = generator.next())
		{
			if (move == stack.excluded)
				continue;

			const bool quietOrLosing = generator.stage() >= MovegenStage::Quiet;

			const auto baseLmr = LmrTable[depth][legalMoves + 1];

			if (!root && quietOrLosing && bestScore > -ScoreWin)
			{
				// futility pruning
				if (!inCheck
					&& depth <= MaxFpDepth
					&& stack.eval + FpMargin + std::max(0, depth - baseLmr) * FpScale <= alpha)
					break;

				// see pruning
				if (depth <= MaxSeePruningDepth
					&& !see::see(pos, move, depth * (pos.isNoisy(move) ? NoisySeeThreshold : QuietSeeThreshold)))
					continue;
			}

			const auto movingPiece = boards.pieceAt(move.src());

			auto guard = pos.applyMove(move, &m_table);

			if (pos.isAttacked(pos.king(us), them))
				continue;

			++data.search.nodes;
			++legalMoves;

			stack.currMove = {movingPiece, moveActualDst(move)};

			i32 extension{};

			// singular extension
			if (depth >= MinSingularityDepth
				&& move == hashMove
				&& !stack.excluded
				&& entry.depth >= depth - 3
				&& entry.type != EntryType::Alpha)
			{
				const auto singularityBeta = std::max(-ScoreMate, entry.score - 2 * depth);
				const auto singularityDepth = (depth - 1) / 2;

				data.stack[newPly].excluded = move;
				pos.popMove();

				const auto score = search(data, singularityDepth, newPly,
					-singularityBeta - 1, -singularityBeta, cutnode);

				data.stack[newPly].excluded = NullMove;
				pos.applyMoveUnchecked(move);

				if (score < singularityBeta)
					extension = 1;
			}

			Score score{};

			if (pos.isDrawn())
				score = drawScore(data.search.nodes);
			else
			{
				i32 reduction{};

				// lmr
				if (depth >= MinLmrDepth
					&& !inCheck // we are in check
					&& !pos.isCheck() // this move gives check
					&& generator.stage() >= MovegenStage::Quiet)
				{
					auto lmr = baseLmr;

					if (!pv)
						++lmr;

					reduction = std::clamp(lmr, 0, newBaseDepth - 1);
				}

				const auto newDepth = newBaseDepth + extension;

				if (pv && legalMoves == 1)
					score = -search(data, newDepth - reduction, newPly, -beta, -alpha, false);
				else
				{
					score = -search(data, newDepth - reduction, newPly, -alpha - 1, -alpha, true);

					if (score > alpha && reduction > 0)
						score = -search(data, newDepth, newPly, -alpha - 1, -alpha, !cutnode);

					if (score > alpha && score < beta)
						score = -search(data, newDepth, newPly, -beta, -alpha, false);
				}
			}

			if (score > bestScore)
			{
				best = move;
				bestScore = score;

				if (score > alpha)
				{
					if (score >= beta)
					{
						if (quietOrLosing)
						{
							if (move != stack.movegen.killer1)
							{
								stack.movegen.killer2 = stack.movegen.killer1;
								stack.movegen.killer1 = move;
							}

							const auto adjustment = depth * depth;

							auto *prevContEntry = prevMove ? &data.history.contEntry(prevMove) : nullptr;
							auto *prevPrevContEntry = prevPrevMove ? &data.history.contEntry(prevPrevMove) : nullptr;

							data.history.entry(stack.currMove).score += adjustment;

							if (prevContEntry)
								prevContEntry->score(stack.currMove) += adjustment;
							if (prevPrevContEntry)
								prevPrevContEntry->score(stack.currMove) += adjustment;

							for (const auto prevQuiet : stack.quietsTried)
							{
								data.history.entry(prevQuiet).score -= adjustment;

								if (prevContEntry)
									prevContEntry->score(prevQuiet) -= adjustment;
								if (prevPrevContEntry)
									prevPrevContEntry->score(prevQuiet) -= adjustment;
							}

							if (prevMove)
								data.history.entry(prevMove).countermove = move;
						}

						entryType = EntryType::Beta;
						break;
					}

					alpha = score;
					entryType = EntryType::Exact;
				}
			}

			if (quietOrLosing)
				stack.quietsTried.push(stack.currMove);
		}

		if (legalMoves == 0)
		{
			if (stack.excluded)
				return alpha;
			return inCheck ? (-ScoreMate + realPly) : 0;
		}

		// increase depth for tt if in check
		// honestly no idea why this gains
		if (!stack.excluded)
			m_table.put(pos.key(), bestScore, best, inCheck ? depth + 1 : depth, entryType);

		if (root && (!m_stop || !data.search.move))
			data.search.move = best;

		return bestScore;
	}

	Score PvsSearcher::qsearch(ThreadData &data, Score alpha, Score beta, i32 ply)
	{
		if (shouldStop(data.search, false))
			return beta;

		auto &pos = data.pos;

		const auto staticEval = eval::staticEval(pos, &data.pawnCache);

		if (staticEval > alpha)
		{
			if (staticEval >= beta)
				return staticEval;

			alpha = staticEval;
		}

		if (ply >= MaxDepth)
			return staticEval;

		const auto us = pos.toMove();

		auto &stack = data.stack[ply];

		++ply;

		if (ply > data.search.seldepth)
			data.search.seldepth = ply;

		auto bestScore = staticEval;

		auto hashMove = m_table.probeMove(pos.key());
		if (hashMove && !pos.isPseudolegal(hashMove))
			hashMove = NullMove;

		QMoveGenerator generator{pos, stack.movegen, hashMove};

		while (const auto move = generator.next())
		{
			auto guard = pos.applyMove(move, &m_table);

			if (pos.isAttacked(pos.king(us), oppColor(us)))
				continue;

			++data.search.nodes;

			const auto score = pos.isDrawn()
				? drawScore(data.search.nodes)
				: -qsearch(data, -beta, -alpha, ply);

			if (score > bestScore)
			{
				bestScore = score;

				if (score > alpha)
					alpha = score;

				if (score >= beta)
					break;
			}
		}

		return bestScore;
	}

	void PvsSearcher::report(const ThreadData &data, i32 depth,
		Move move, f64 time, Score score, Score alpha, Score beta)
	{
		usize nodes = 0;

		// technically a potential race but it doesn't matter
		for (const auto &thread : m_threads)
		{
			nodes += thread.search.nodes;
		}

		const auto ms = static_cast<usize>(time * 1000.0);
		const auto nps = static_cast<usize>(static_cast<f64>(nodes) / time);

		std::cout << "info depth " << depth << " seldepth " << data.search.seldepth
			<< " time " << ms << " nodes " << nodes << " nps " << nps << " score ";

		score = std::clamp(score, alpha, beta);

		if (std::abs(score) > ScoreWin)
		{
			if (score > ScoreWin)
				std::cout << "mate " << ((ScoreMate - score + 1) / 2);
			else std::cout << "mate " << (-(ScoreMate + score) / 2);
		}
		else std::cout << "cp " << eval::internalUnitsToCp(score);

		if (score == alpha)
			std::cout << " upperbound";
		else if (score == beta)
			std::cout << " lowerbound";

		std::cout << " hashfull " << m_table.full() << " pv " << uci::moveToString(move);

		Position pos{data.pos};
		pos.applyMoveUnchecked<false, false>(move);

		StaticVector<u64, MaxDepth> positionsHit{};
		positionsHit.push(pos.key());

		while (true)
		{
			const auto pvMove = m_table.probeMove(pos.key());

			if (pvMove && pos.isPseudolegal(pvMove))
			{
				pos.applyMoveUnchecked<false, false>(pvMove);

				if (std::find(positionsHit.begin(), positionsHit.end(), pos.key()) == positionsHit.end()
					&& !pos.isAttacked(pos.king(pos.opponent()), pos.toMove()))
				{
					std::cout << ' ' << uci::moveToString(pvMove);
					positionsHit.push(pos.key());
				}
				else break;
			}
			else break;
		}

		std::cout << std::endl;
	}
}
