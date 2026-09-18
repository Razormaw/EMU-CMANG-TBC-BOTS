#include "TabernaConversationMgr.h"
#include "PlayerbotLLMInterface.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "Chat/ChannelMgr.h"
#include "Chat/Channel.h"
#include "Entities/Player.h"
#include "Entities/ObjectGuid.h"
#include "Log/Log.h"

// ============================================================================
// MODELO DE LA TABERNA. Cambialo AQUI si cambias de proveedor/modelo remoto.
// Debe coincidir con AiPlayerbot.LLMModel de tu ai_playerbot.conf.
// ============================================================================
static const char* TABERNA_LLM_MODEL = "qwen/qwen3.8-27b";

// Forward del namespace de frases que ya tienes en PlayerbotAI.cpp
namespace TabernaChat {
    bool GetRandomPhrase(std::string& out);
}

TabernaConversationMgr& TabernaConversationMgr::instance()
{
    static TabernaConversationMgr inst;
    return inst;
}

TabernaConversationMgr::TabernaConversationMgr() {}
TabernaConversationMgr::~TabernaConversationMgr() { Stop(); }

void TabernaConversationMgr::Start()
{
    if (m_running.exchange(true))
        return;

    m_healthUrl = sPlayerbotAIConfig.llmEndPointUrl.hostname;

    m_workerThread = std::thread(&TabernaConversationMgr::WorkerLoop, this);
    m_healthThread = std::thread(&TabernaConversationMgr::HealthCheckLoop, this);

    sLog.outString(">> TabernaConversationMgr: iniciado (endpoint: %s)",
        sPlayerbotAIConfig.llmEndPointUrl.hostname.c_str());
}

void TabernaConversationMgr::Stop()
{
    m_running = false;
    if (m_workerThread.joinable())  m_workerThread.join();
    if (m_healthThread.joinable())  m_healthThread.join();
}

// ============================================================================
// Health check con HISTÉRESIS y BACKOFF:
//  - Solo marca CAIDO tras 2 fallos seguidos (evita flap cuando está ocupado).
//  - Revisa cada 60s (antes 20s, reducido para no saturar el log ni gastar requests).
// ============================================================================
void TabernaConversationMgr::HealthCheckLoop()
{
    while (m_running)
    {
        bool up = HttpHealthPing();
        
        if (up)
        {
            m_failCount = 0;
            if (!m_ollamaUp.exchange(true))
                sLog.outString(">> Taberna: LLM EN LINEA. Modo: conversacion activa");
        }
        else
        {
            if (++m_failCount >= 2 && m_ollamaUp.exchange(false))
                sLog.outString(">> Taberna: LLM CAIDO. Modo: solo frases de la BD");
        }

        // Ping cada 60 segundos (reducido de 20s para optimizar)
        int waitSec = 60;
        for (int i = 0; i < waitSec && m_running; ++i)
            std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

bool TabernaConversationMgr::HttpHealthPing()
{
    std::vector<std::string> dbg;
    std::string resp = PlayerbotLLMInterface::Generate(
        "{\"model\":\"" + std::string(TABERNA_LLM_MODEL) + "\","
        "\"messages\":[{\"role\":\"user\",\"content\":\"ping\"}],"
        "\"max_tokens\":1,\"stream\":false}", 4, 999, dbg);

    if (resp.empty() || resp == "error")
        return false;

    bool hasError   = resp.find("\"error\"")   != std::string::npos;
    bool hasContent = resp.find("\"content\"") != std::string::npos
                   || resp.find("\"choices\"") != std::string::npos;
    
    return (hasContent && !hasError);
}

// ============================================================================
// Registro de bots por canal
// ============================================================================
void TabernaConversationMgr::RegisterBot(Channel* chan, Player* bot)
{
    if (!chan || !bot) return;
    std::lock_guard<std::mutex> g(m_queueMutex);
    auto& vec = m_channelBots[chan];
    for (auto* p : vec)
        if (p == bot) return;
    vec.push_back(bot);
}

Player* TabernaConversationMgr::PickOtherBotInChannel(Channel* chan, Player* exclude)
{
    std::lock_guard<std::mutex> g(m_queueMutex);
    auto it = m_channelBots.find(chan);
    if (it == m_channelBots.end())
        return nullptr;

    std::vector<Player*> candidates;
    auto& vec = it->second;

    // Limpieza: quitar bots muertos o desconectados
    for (auto itr = vec.begin(); itr != vec.end(); )
    {
        Player* p = *itr;
        if (!p || !p->IsInWorld() || !p->IsAlive())
            itr = vec.erase(itr);
        else
            ++itr;
    }

    for (auto* p : vec)
    {
        if (p == exclude) continue;
        candidates.push_back(p);
    }

    if (candidates.empty())
        return nullptr;

    return candidates[urand(0, (uint32)candidates.size() - 1)];
}

// ============================================================================
// Bots charlando entre ellos (semilla BD + ping-pong LLM)
// ============================================================================
void TabernaConversationMgr::OnBotWantsToTalk(Player* bot, Channel* chan)
{
    if (!bot || !chan) return;

    RegisterBot(chan, bot);

    if (!IsOllamaUp())
    {
        SayFromDB(bot, chan);
        return;
    }

    const time_t now = time(nullptr);
    if (now - m_lastConversationEnd < (time_t)m_cooldownSec)
    {
        SayFromDB(bot, chan);
        return;
    }

    std::string seed;
    if (!TabernaChat::GetRandomPhrase(seed))
        return;

    chan->Say(bot, seed.c_str(), LANG_UNIVERSAL);

    Player* responder = PickOtherBotInChannel(chan, bot);
    if (!responder)
        return;

    uint8 turns = m_minTurns + (urand(0, 255) % (m_maxTurns - m_minTurns + 1));

    TabernaTurn turn;
    turn.bot = responder;
    turn.chan = chan;
    turn.seed = seed;
    turn.turnsLeft = turns;
    turn.useQwen = true;

    std::lock_guard<std::mutex> g(m_queueMutex);
    m_queue.push(turn);
}

// ============================================================================
// Un JUGADOR REAL habló: cola PRIORITARIA para que no espere detrás de los bots
// ============================================================================
void TabernaConversationMgr::OnPlayerSpeaks(Player* speaker, Channel* chan, const std::string& text)
{
    if (!speaker || !chan || text.empty())
        return;

    sLog.outString(">> Taberna: JUGADOR [%s] dijo: %s",
        speaker->GetName(), text.c_str());

    Player* responder = PickOtherBotInChannel(chan, nullptr);
    if (!responder)
    {
        sLog.outString(">> Taberna: sin bots registrados en el canal, no hay quien responda");
        return;
    }

    TabernaTurn turn;
    turn.bot = responder;
    turn.chan = chan;
    turn.seed = text;
    turn.turnsLeft = urand(1, 2);
    turn.useQwen = true;

    std::lock_guard<std::mutex> g(m_queueMutex);
    m_playerQueue.push(turn);
}

// ============================================================================
// Worker: atiende JUGADORES primero, bots después
// ============================================================================
void TabernaConversationMgr::WorkerLoop()
{
    while (m_running)
    {
        TabernaTurn turn;
        bool have = false;
        {
            std::lock_guard<std::mutex> g(m_queueMutex);
            if (!m_playerQueue.empty())
            { turn = m_playerQueue.front(); m_playerQueue.pop(); have = true; }
            else if (!m_queue.empty())
            { turn = m_queue.front(); m_queue.pop(); have = true; }
        }
        if (!have)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            continue;
        }
        ProcessTurn(turn);
    }
}

void TabernaConversationMgr::ProcessTurn(const TabernaTurn& turn)
{
    if (!turn.bot || !turn.chan) 
        return;

    // Delay natural (corre en worker, no congela el world update)
    std::this_thread::sleep_for(std::chrono::seconds(urand(2, 4)));

    RegisterBot(turn.chan, turn.bot);

    if (!turn.bot->IsInWorld() || !turn.bot->IsAlive())
    {
        m_lastConversationEnd = time(nullptr);
        return;
    }

    if (!turn.useQwen)
    {
        SayFromDB(turn.bot, turn.chan);
        m_lastConversationEnd = time(nullptr);
        return;
    }
    
    // Verificar estado real en el momento de procesar
    if (!IsOllamaUp())
    {
        SayFromDB(turn.bot, turn.chan);
        m_lastConversationEnd = time(nullptr);
        return;
    }

    sLog.outString(">> Taberna: pidiendo al LLM: %s", turn.seed.c_str());

    std::string reply = RequestQwenReply(turn.seed, turn.bot->GetName());

    if (reply.empty())
        sLog.outString(">> Taberna: LLM NO respondio -> failover a BD");
    else
        sLog.outString(">> Taberna: LLM respondio: %s", reply.c_str());

    if (reply.empty())
    {
        SayFromDB(turn.bot, turn.chan);
        m_lastConversationEnd = time(nullptr);
        return;
    }

    turn.chan->Say(turn.bot, reply.c_str(), LANG_UNIVERSAL);

    if (turn.turnsLeft > 1)
    {
        uint32 delay = m_minDelay + (urand(0, 255) % (m_maxDelay - m_minDelay + 1));
        std::this_thread::sleep_for(std::chrono::seconds(delay));

        Player* next = PickOtherBotInChannel(turn.chan, turn.bot);
        if (next)
        {
            TabernaTurn next_turn;
            next_turn.bot = next;
            next_turn.chan = turn.chan;
            next_turn.seed = reply;
            next_turn.turnsLeft = turn.turnsLeft - 1;
            next_turn.useQwen = true;

            std::lock_guard<std::mutex> g(m_queueMutex);
            m_queue.push(next_turn);
        }
        else
            m_lastConversationEnd = time(nullptr);
    }
    else
        m_lastConversationEnd = time(nullptr);
}

// ============================================================================
// Llamada al LLM remoto (formato OpenAI-compat) + parser manual robusto
// ============================================================================
std::string TabernaConversationMgr::RequestQwenReply(const std::string& seed, const std::string& botName)
{
    std::string systemPrompt = BuildTavernPrompt(seed, botName);
    std::string sanitized = PlayerbotLLMInterface::SanitizeForJson(systemPrompt);

    std::string body =
        "{\"model\":\"" + std::string(TABERNA_LLM_MODEL) + "\","
        "\"messages\":["
          "{\"role\":\"system\",\"content\":\"" + sanitized + "\"},"
          "{\"role\":\"user\",\"content\":\"Responde ahora, en personaje y unicamente en espanol.\"}"
        "],\"max_tokens\":120,\"temperature\":0.7,\"stream\":false}";

    std::vector<std::string> dbg;
    std::string resp = PlayerbotLLMInterface::Generate(body, 30, 25, dbg);

    if (resp.empty() || resp == "error")
        return "";

    // Parser manual: extrae "content":"..." respetando escapes, sin regex
    size_t cpos = resp.find("\"content\":\"");
    if (cpos == std::string::npos)
        return "";

    size_t i = cpos + 11;
    std::string out;
    for (; i < resp.size(); ++i)
    {
        char ch = resp[i];
        if (ch == '\\' && i + 1 < resp.size())
        {
            char nx = resp[i + 1];
            if (nx == 'n' || nx == 't') { out += ' '; i++; continue; }
            if (nx == '"' || nx == '\\') { out += nx; i++; continue; }
            out += nx; i++; continue;
        }
        if (ch == '"')
            break;
        out += ch;
    }
    
    // Limpieza final: fuera asteriscos y comillas que a veces se cuelan
    {
        std::string clean;
        for (char c : out)
            if (c != '*' && c != '"')
                clean += c;
        out = clean;
    }
    
    return Truncate(out, 180);
}

std::string TabernaConversationMgr::BuildTavernPrompt(const std::string& seed, const std::string& botName)
{
    return "Eres " + botName + ", un parroquiano de la cantina 'taberna' en Azeroth (WoW TBC). "
           "Otro parroquiano acaba de decir: \"" + seed + "\" "
           "Contéstale directamente a eso, UNICAMENTE en espanol latino, 1 o 2 frases cortas (maximo 150 caracteres). "
           "Tono: humor de taberna, sabiduria de veterano. Puedes referirte a mazmorras, raids, "
           "clases y memes de WoW TBC. Nunca salgas del rol. No menciones que eres una IA. "
           "Responde solo con el dialogo, sin comillas ni prefijos.";
}

// ============================================================================
// Failover a BD: la taberna NUNCA se queda muda
// ============================================================================
void TabernaConversationMgr::SayFromDB(Player* bot, Channel* chan)
{
    std::string texto;
    if (TabernaChat::GetRandomPhrase(texto))
        chan->Say(bot, texto.c_str(), LANG_UNIVERSAL);
}

std::string TabernaConversationMgr::Truncate(const std::string& s, size_t maxLen)
{
    if (s.size() <= maxLen) return s;
    size_t cut = s.find_last_of(" .,;:!", maxLen);
    if (cut == std::string::npos || cut < maxLen / 2)
        cut = maxLen;
    return s.substr(0, cut);
}