/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2017 XMRig       <support@xmrig.com>
 *
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <thread>


#include "crypto/CryptoNight.h"
#include "workers/MultiWorker.h"
#include "workers/Workers.h"


template <size_t FACTOR>
class MultiWorker : public Worker
{
public:
    explicit MultiWorker(Handle *handle);
    ~MultiWorker();

    void start() override;

private:
    bool resume(const Job &job);
    void consumeJob();
    void save(const Job &job);

    class State;

    uint8_t m_hash[32*FACTOR];
    State *m_state;
    State *m_pausedState;
};

template <size_t FACTOR>
class MultiWorker<FACTOR>::State
{
public:
  State()
  {
      for(uint32_t& nonce : nonces) {
          nonce = 0;
      }
  }

  Job job;
  uint32_t nonces[FACTOR];
  uint8_t blob[84 * FACTOR];
};

template <size_t FACTOR>
MultiWorker<FACTOR>::MultiWorker(Handle *handle)
    : Worker(handle)
{
    m_state       = new MultiWorker<FACTOR>::State();
    m_pausedState = new MultiWorker<FACTOR>::State();
}

template <size_t FACTOR>
MultiWorker<FACTOR>::~MultiWorker()
{
    delete m_state;
    delete m_pausedState;
}

template <size_t FACTOR>
void MultiWorker<FACTOR>::start()
{
    while (Workers::sequence() > 0) {
        if (Workers::isPaused()) {
            do {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            while (Workers::isPaused());

            if (Workers::sequence() == 0) {
                break;
            }

            consumeJob();
        }

        while (!Workers::isOutdated(m_sequence)) {
            if ((m_count & 0xF) == 0) {
                storeStats();
            }

            m_count += FACTOR;

            for (size_t i=0; i < FACTOR; ++i) {
                *Job::nonce(m_state->blob + i * m_state->job.size()) = ++m_state->nonces[i];
            }

            CryptoNight::hash<FACTOR>(m_state->blob, m_state->job.size(), m_hash, m_ctx);

            for (size_t i=0; i < FACTOR; ++i) {
                if (*reinterpret_cast<uint64_t *>(m_hash + 24 + i * 32) < m_state->job.target()) {
                    Workers::submit(JobResult(m_state->job.poolId(), m_state->job.id(), m_state->nonces[i], m_hash + i * 32,
                                              m_state->job.diff()), m_id);
                }
            }

            std::this_thread::yield();
        }

        consumeJob();
    }
}

template <size_t FACTOR>
bool MultiWorker<FACTOR>::resume(const Job &job)
{
    if (m_state->job.poolId() == -1 && job.poolId() >= 0 && job.id() == m_pausedState->job.id()) {
        *m_state = *m_pausedState;
        return true;
    }

    return false;
}

template <size_t FACTOR>
void MultiWorker<FACTOR>::consumeJob()
{
    Job job = Workers::job();
    m_sequence = Workers::sequence();
    if (m_state->job == job) {
        return;
    }

    save(job);

    if (resume(job)) {
        return;
    }

    m_state->job = std::move(job);

    for (size_t i=0; i < FACTOR; ++i) {
        memcpy(m_state->blob + i * m_state->job.size(), m_state->job.blob(), m_state->job.size());
        if (m_state->job.isNicehash()) {
            m_state->nonces[i] = (*Job::nonce(m_state->blob + m_state->job.size()) & 0xff000000U) +
                                 (0xffffffU / (m_threads * FACTOR) * (m_id + i * m_threads));
        }
        else {
            m_state->nonces[i] = std::numeric_limits<uint32_t>::max() / (m_threads * FACTOR) *
                                 (m_id + i * m_threads);
        }
    }
}

template <size_t FACTOR>
void MultiWorker<FACTOR>::save(const Job &job)
{
    if (job.poolId() == -1 && m_state->job.poolId() >= 0) {
        *m_pausedState = *m_state;
    }
}

Worker* createMultiWorker(size_t numHashes, Handle *handle) {
    switch (numHashes) {
        case 2:
            return new MultiWorker<2>(handle);
        case 3:
            return new MultiWorker<3>(handle);
        case 4:
            return new MultiWorker<4>(handle);
        case 1:
        default:
            return new MultiWorker<1>(handle);
    }
}