#ifndef _TABERNA_CONVERSATION_MGR_H
#define _TABERNA_CONVERSATION_MGR_H

#include "Common.h"
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <ctime>

class Player;
class Channel;

struct TabernaTurn
{
    Player* bot;
    Channel* chan;
    std::string seed;
    uint8 turnsLeft;
    bool useQwen;
};

class TabernaConversationMgr
{
public:
    TabernaConversationMgr();
    ~TabernaConversationMgr();

    static TabernaConversationMgr& instance();

    void Start();
    void Stop();

    void OnBotWantsToTalk(Player* bot, Channel* chan);
    void OnPlayerSpeaks(Player* speaker, Channel* chan, const std::string& text);

    bool IsOllamaUp() const { return m_ollamaUp.load(); }

private:
    void WorkerLoop();
    void HealthCheckLoop();
    bool HttpHealthPing();

    void ProcessTurn(const TabernaTurn& turn);
    std::string RequestQwenReply(const std::string& seed, const std::string& botName);
    void SayFromDB(Player* bot, Channel* chan);
    void RegisterBot(Channel* chan, Player* bot);
    Player* PickOtherBotInChannel(Channel* chan, Player* exclude);
    std::string Truncate(const std::string& s, size_t maxLen);
    std::string BuildTavernPrompt(const std::string& seed, const std::string& botName);

    std::thread m_workerThread;
    std::thread m_healthThread;
    std::atomic<bool> m_running{false};

    std::queue<TabernaTurn> m_queue;         // turnos de bots (prioridad baja)
    std::queue<TabernaTurn> m_playerQueue;   // turnos de JUGADORES (prioridad ALTA)
    std::mutex m_queueMutex;

    std::map<Channel*, std::vector<Player*>> m_channelBots;

    std::atomic<bool> m_ollamaUp{false};
    int m_failCount = 0;                     // histéresis del health-check
    time_t m_lastConversationEnd = 0;
    uint32 m_cooldownSec = 150;
    uint8  m_minTurns = 2;
    uint8  m_maxTurns = 3;
    uint32 m_minDelay = 4;
    uint32 m_maxDelay = 8;
    std::string m_healthUrl;
};

#define sTabernaConvMgr TabernaConversationMgr::instance()

#endif

