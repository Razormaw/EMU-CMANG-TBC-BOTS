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

// ============================================================================
// Contextos de conversacion: cada uno con su tono y reglas
// ============================================================================
enum TalkContext : uint8
{
    CONTEXT_TAVERN = 0,   // canal #taberna: efusivo, humor
    CONTEXT_GUILD  = 1,   // chat de gremio: sobrio, funcional
    CONTEXT_PARTY  = 2,   // chat de grupo: reservado, tactico
    CONTEXT_RAID   = 3,   // chat de raid: reservado, tactico de banda
    CONTEXT_TRADE  = 4    // canal de comercio: promotor de productos
};

struct TabernaTurn
{
    Player* bot;
    Channel* chan;          // usado en TAVERN y TRADE; nullptr en GUILD/PARTY/RAID
    std::string seed;
    uint8 turnsLeft;
    bool useQwen;
    uint8 context;
};

class TabernaConversationMgr
{
public:
    TabernaConversationMgr();
    ~TabernaConversationMgr();

    static TabernaConversationMgr& instance();

    void Start();
    void Stop();

    // --- Taberna y Comercio (canal) ---
    void OnBotWantsToTalk(Player* bot, Channel* chan);
    void OnPlayerSpeaks(Player* speaker, Channel* chan, const std::string& text);
    void OnTradePlayerSpeaks(Player* speaker, Channel* chan, const std::string& text);

    // --- Gremio / Grupo / Raid (hooks de WorldSession) ---
    void RegisterBot(Channel* chan, Player* bot);
    void RegisterGuildBot(Player* bot);
    void OnGuildPlayerSpeaks(Player* speaker, const std::string& text);
    void OnPartyPlayerSpeaks(Player* speaker, const std::string& text, bool isRaid);

    bool IsOllamaUp() const { return m_ollamaUp.load(); }

private:
    void WorkerLoop();
    void HealthCheckLoop();
    bool HttpHealthPing();

    void ProcessTurn(const TabernaTurn& turn);
    std::string RequestQwenReply(const std::string& seed, const std::string& botName, uint8 context);
    void SayFromDB(Player* bot, Channel* chan);
    void SendGuildSay(Player* bot, const std::string& text);
    void SendGroupSay(Player* bot, const std::string& text, bool isRaid);

    Player* PickOtherBotInChannel(Channel* chan, Player* exclude);
    Player* PickGuildBot(uint32 guildId, Player* exclude);
    Player* PickBotInGroup(Player* speaker, bool isRaid);

    std::string Truncate(const std::string& s, size_t maxLen);
    std::string BuildTavernPrompt(const std::string& seed, const std::string& botName);
    std::string BuildGuildPrompt(const std::string& seed, const std::string& botName);
    std::string BuildPartyPrompt(const std::string& seed, const std::string& botName, bool isRaid);
    std::string BuildTradePrompt(const std::string& seed, const std::string& botName);

    std::thread m_workerThread;
    std::thread m_healthThread;
    std::atomic<bool> m_running{false};

    std::queue<TabernaTurn> m_queue;         // bots (prioridad baja)
    std::queue<TabernaTurn> m_playerQueue;   // jugadores (prioridad ALTA)
    std::mutex m_queueMutex;

    std::map<Channel*, std::vector<Player*>> m_channelBots;  // taberna/comercio
    std::map<uint32, std::vector<Player*>>   m_guildBots;    // gremios

    std::atomic<bool> m_ollamaUp{false};
    int m_failCount = 0;
    time_t m_lastConversationEnd = 0;
    uint32 m_cooldownSec = 120;
    uint8  m_minTurns = 2;
    uint8  m_maxTurns = 3;
    uint32 m_minDelay = 4;
    uint32 m_maxDelay = 8;
    std::string m_healthUrl;

    // Anti-loop: registro de la ultima respuesta enviada por un bot
    std::string m_lastBotSaid;
    time_t      m_lastBotSaidTime = 0;
};

#define sTabernaConvMgr TabernaConversationMgr::instance()

#endif