#include "TabernaConversationMgr.h"
#include "PlayerbotLLMInterface.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "Chat/ChannelMgr.h"
#include "Chat/Channel.h"
#include "Chat/Chat.h"
#include "Guilds/Guild.h"
#include "Guilds/GuildMgr.h"
#include "Groups/Group.h"
#include "Globals/ObjectMgr.h"
#include "Entities/Player.h"
#include "Entities/ObjectGuid.h"
#include "Log/Log.h"

// ============================================================================
// MODELO Y OPCIONES
// ============================================================================
static const char* TABERNA_LLM_MODEL = "qwen/qwen3.8-27b";

// true = ver charlas y llamadas al LLM (para depuracion). false = log limpio.
static const bool TABERNA_VERBOSE_LOG = false;

namespace TabernaChat {
    bool GetRandomPhrase(std::string& out);
}

// ============================================================================
// Deteccion de bot (debe coincidir con la que usa tu hook de Channel::Say)
// ============================================================================
static bool IsBotPlayer(Player* p)
{
    if (!p) return false;
    return p->GetPlayerbotAI() != nullptr;
}


// ============================================================================
// Filtro de mensajes automáticos (addons, loot, quests, etc.)
// ============================================================================
static bool IsAutoMessage(const std::string& text)
{
    // Mensajes del addon pfQuest
    if (text.find("pfQuest") != std::string::npos) return true;
    if (text.find("VERSION:") != std::string::npos) return true;
    
    // Mensajes de loot/compra/venta del cliente
    if (text.find("Buying") != std::string::npos) return true;
    if (text.find("Selling") != std::string::npos) return true;
    if (text.find("Casting") != std::string::npos) return true;
    if (text.find("|Hitem:") != std::string::npos) return true;
    if (text.find("|Hquest:") != std::string::npos) return true;
    if (text.find("|Hspell:") != std::string::npos) return true;
    
    // Mensajes de misiones automáticos
    if (text.find("está disponible") != std::string::npos) return true;
    if (text.find("No puedo aceptar") != std::string::npos) return true;
    if (text.find("Misión") != std::string::npos && text.find("disponible") != std::string::npos) return true;
    
    // Mensajes muy cortos o solo símbolos (probablemente automáticos)
    if (text.length() < 5) return true;
    
    return false;
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
// Health check con histéresis y backoff (ping cada 60s)
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
// Registro de bots por canal (TABERNA / COMERCIO)
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

// ============================================================================
// Registro de bots por gremio (GREMIO)
// ============================================================================
void TabernaConversationMgr::RegisterGuildBot(Player* bot)
{
    if (!bot || bot->GetGuildId() == 0)
        return;
    std::lock_guard<std::mutex> g(m_queueMutex);
    auto& vec = m_guildBots[bot->GetGuildId()];
    for (auto itr = vec.begin(); itr != vec.end(); )
    {
        Player* p = *itr;
        if (!p || !p->IsInWorld() || !p->IsAlive())
            itr = vec.erase(itr);
        else
            ++itr;
    }
    for (auto* p : vec)
        if (p == bot) return;
    vec.push_back(bot);

    sLog.outString(">> [DIAG-G] RegisterGuildBot: %s en gremio %u (total bots en gremio: %u)",
        bot->GetName(), bot->GetGuildId(), (uint32)vec.size());
}

Player* TabernaConversationMgr::PickOtherBotInChannel(Channel* chan, Player* exclude)
{
    std::lock_guard<std::mutex> g(m_queueMutex);
    auto it = m_channelBots.find(chan);
    if (it == m_channelBots.end())
        return nullptr;

    std::vector<Player*> candidates;
    auto& vec = it->second;
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

Player* TabernaConversationMgr::PickGuildBot(uint32 guildId, Player* exclude)
{
    std::lock_guard<std::mutex> g(m_queueMutex);
    auto it = m_guildBots.find(guildId);
    if (it == m_guildBots.end())
        return nullptr;

    std::vector<Player*> candidates;
    auto& vec = it->second;
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

Player* TabernaConversationMgr::PickBotInGroup(Player* speaker, bool isRaid)
{
    if (!speaker) return nullptr;
    Group* group = speaker->GetGroup();
    if (!group) return nullptr;
    if (group->IsRaidGroup() != isRaid) return nullptr;

    std::vector<Player*> bots;
    const Group::MemberSlotList& slots = group->GetMemberSlots();
    for (Group::MemberSlotList::const_iterator itr = slots.begin(); itr != slots.end(); ++itr)
    {
        Player* member = sObjectMgr.GetPlayer(itr->guid);
        if (member && member != speaker && IsBotPlayer(member)
            && member->IsInWorld() && member->IsAlive())
            bots.push_back(member);
    }
    if (bots.empty())
        return nullptr;
    return bots[urand(0, (uint32)bots.size() - 1)];
}

// ============================================================================
// TABERNA: bots charlando entre ellos
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
    turn.context = CONTEXT_TAVERN;

    std::lock_guard<std::mutex> g(m_queueMutex);
    m_queue.push(turn);
}

// ============================================================================
// TABERNA: un JUGADOR REAL hablo en el canal
// ============================================================================
void TabernaConversationMgr::OnPlayerSpeaks(Player* speaker, Channel* chan, const std::string& text)
{
    if (!speaker || !chan || text.empty())
        return;

    // Anti-loop
    {
        const time_t now = time(nullptr);
        if (now - m_lastBotSaidTime < 10 && text == m_lastBotSaid)
            return;
    }
	
	
    // Filtrar mensajes automáticos
    if (IsAutoMessage(text))
    {
        if (TABERNA_VERBOSE_LOG)
            sLog.outString(">> Gremio: mensaje automático ignorado");
        return;
    }

    if (TABERNA_VERBOSE_LOG)
        sLog.outString(">> Taberna: JUGADOR [%s] dijo: %s", speaker->GetName(), text.c_str());

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
    turn.context = CONTEXT_TAVERN;

    std::lock_guard<std::mutex> g(m_queueMutex);
    m_playerQueue.push(turn);
}

// ============================================================================
// COMERCIO: un JUGADOR REAL hablo en el canal de comercio
// ============================================================================
void TabernaConversationMgr::OnTradePlayerSpeaks(Player* speaker, Channel* chan, const std::string& text)
{
    if (!speaker || !chan || text.empty())
        return;

    // Anti-loop
    {
        const time_t now = time(nullptr);
        if (now - m_lastBotSaidTime < 10 && text == m_lastBotSaid)
            return;
    }
	
	
        // Filtrar mensajes automáticos
    if (IsAutoMessage(text))
        return;

    if (TABERNA_VERBOSE_LOG)
        sLog.outString(">> Comercio: JUGADOR [%s] dijo: %s", speaker->GetName(), text.c_str());

    Player* responder = PickOtherBotInChannel(chan, nullptr);
    if (!responder)
    {
        sLog.outString(">> Comercio: sin bots registrados en el canal, no hay quien responda");
        return;
    }

    TabernaTurn turn;
    turn.bot = responder;
    turn.chan = chan;
    turn.seed = text;
    turn.turnsLeft = 1;
    turn.useQwen = true;
    turn.context = CONTEXT_TRADE;

    std::lock_guard<std::mutex> g(m_queueMutex);
    m_playerQueue.push(turn);
}

// ============================================================================
// GREMIO: un JUGADOR REAL hablo en el chat de hermandad
// ============================================================================
void TabernaConversationMgr::OnGuildPlayerSpeaks(Player* speaker, const std::string& text)
{
    if (!speaker || text.empty() || speaker->GetGuildId() == 0)
        return;

    // Anti-loop
    {
        const time_t now = time(nullptr);
        if (now - m_lastBotSaidTime < 10 && text == m_lastBotSaid)
            return;
    }

    sLog.outString(">> [DIAG-G] OnGuildPlayerSpeaks llamado: speaker=[%s] gremio=%u",
        speaker->GetName(), speaker->GetGuildId());

    if (TABERNA_VERBOSE_LOG)
        sLog.outString(">> Gremio: JUGADOR [%s] dijo: %s", speaker->GetName(), text.c_str());

    Player* responder = PickGuildBot(speaker->GetGuildId(), speaker);

    sLog.outString(">> [DIAG-G] responder = %s",
        responder ? responder->GetName() : "nullptr");

    if (!responder)
    {
        sLog.outString(">> Gremio: sin bots en la hermandad, no hay quien responda");
        return;
    }

    TabernaTurn turn;
    turn.bot = responder;
    turn.chan = nullptr;
    turn.seed = text;
    turn.turnsLeft = 1;
    turn.useQwen = true;
    turn.context = CONTEXT_GUILD;

    std::lock_guard<std::mutex> g(m_queueMutex);
    m_playerQueue.push(turn);
}

// ============================================================================
// GRUPO / RAID: un JUGADOR REAL hablo en el chat de grupo/raid
// ============================================================================
void TabernaConversationMgr::OnPartyPlayerSpeaks(Player* speaker, const std::string& text, bool isRaid)
{
    if (!speaker || text.empty())
        return;

    // Anti-loop
    {
        const time_t now = time(nullptr);
        if (now - m_lastBotSaidTime < 10 && text == m_lastBotSaid)
            return;
    }
	
	    // Filtrar mensajes automáticos
    if (IsAutoMessage(text))
    {
        if (TABERNA_VERBOSE_LOG)
            sLog.outString(">> %s: mensaje automático ignorado", isRaid ? "Raid" : "Grupo");
        return;
    }

    if (TABERNA_VERBOSE_LOG)
        sLog.outString(">> %s: JUGADOR [%s] dijo: %s",
            isRaid ? "Raid" : "Grupo", speaker->GetName(), text.c_str());

    Player* responder = PickBotInGroup(speaker, isRaid);
    if (!responder)
    {
        sLog.outString(">> %s: sin bots en el grupo/raid, no hay quien responda",
            isRaid ? "Raid" : "Grupo");
        return;
    }

    TabernaTurn turn;
    turn.bot = responder;
    turn.chan = nullptr;
    turn.seed = text;
    turn.turnsLeft = 1;
    turn.useQwen = true;
    turn.context = isRaid ? CONTEXT_RAID : CONTEXT_PARTY;

    std::lock_guard<std::mutex> g(m_queueMutex);
    m_playerQueue.push(turn);
}

// ============================================================================
// Worker: atiende JUGADORES primero, bots despues
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
    if (!turn.bot)
        return;
    if ((turn.context == CONTEXT_TAVERN || turn.context == CONTEXT_TRADE) && !turn.chan)
        return;

    uint8 lo = 2;
    uint8 hi = (turn.context == CONTEXT_PARTY || turn.context == CONTEXT_RAID) ? 3 : 4;
    std::this_thread::sleep_for(std::chrono::seconds(urand(lo, hi)));

    if (turn.chan)
        RegisterBot(turn.chan, turn.bot);
    else if (turn.context == CONTEXT_GUILD)
        RegisterGuildBot(turn.bot);

    if (!turn.bot->IsInWorld() || !turn.bot->IsAlive())
    {
        m_lastConversationEnd = time(nullptr);
        return;
    }

    if (!turn.useQwen || !IsOllamaUp())
    {
        if (turn.context == CONTEXT_TAVERN)
            SayFromDB(turn.bot, turn.chan);
        m_lastConversationEnd = time(nullptr);
        return;
    }

    const char* tag =
        (turn.context == CONTEXT_GUILD) ? "Gremio" :
        (turn.context == CONTEXT_PARTY) ? "Grupo" :
        (turn.context == CONTEXT_RAID)  ? "Raid" :
        (turn.context == CONTEXT_TRADE) ? "Comercio" : "Taberna";

    if (TABERNA_VERBOSE_LOG)
        sLog.outString(">> %s: pidiendo al LLM: %s", tag, turn.seed.c_str());

    std::string reply = RequestQwenReply(turn.seed, turn.bot->GetName(), turn.context);

    if (TABERNA_VERBOSE_LOG)
    {
        if (reply.empty())
            sLog.outString(">> %s: LLM NO respondio", tag);
        else
            sLog.outString(">> %s: LLM respondio: %s", tag, reply.c_str());
    }

    if (reply.empty())
    {
        if (turn.context == CONTEXT_TAVERN)
            SayFromDB(turn.bot, turn.chan);
        m_lastConversationEnd = time(nullptr);
        return;
    }

    m_lastBotSaid = reply;
    m_lastBotSaidTime = time(nullptr);

    switch (turn.context)
    {
        case CONTEXT_GUILD:
            SendGuildSay(turn.bot, reply);
            break;
        case CONTEXT_PARTY:
            SendGroupSay(turn.bot, reply, false);
            break;
        case CONTEXT_RAID:
            SendGroupSay(turn.bot, reply, true);
            break;
        case CONTEXT_TRADE:
        case CONTEXT_TAVERN:
        default:
            turn.chan->Say(turn.bot, reply.c_str(), LANG_UNIVERSAL);
            break;
    }

    if (turn.context == CONTEXT_TAVERN && turn.turnsLeft > 1)
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
            next_turn.context = CONTEXT_TAVERN;

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
// Llamada al LLM con prompt segun contexto
// ============================================================================
std::string TabernaConversationMgr::RequestQwenReply(const std::string& seed, const std::string& botName, uint8 context)
{
    std::string systemPrompt;
    switch (context)
    {
        case CONTEXT_GUILD:  systemPrompt = BuildGuildPrompt(seed, botName); break;
        case CONTEXT_PARTY:  systemPrompt = BuildPartyPrompt(seed, botName, false); break;
        case CONTEXT_RAID:   systemPrompt = BuildPartyPrompt(seed, botName, true); break;
        case CONTEXT_TRADE:  systemPrompt = BuildTradePrompt(seed, botName); break;
        case CONTEXT_TAVERN:
        default:             systemPrompt = BuildTavernPrompt(seed, botName); break;
    }
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

    {
        std::string clean;
        for (char c : out)
            if (c != '*' && c != '"')
                clean += c;
        out = clean;
    }

    size_t maxLen = (context == CONTEXT_TAVERN) ? 180 : 120;
    return Truncate(out, maxLen);
}

// ============================================================================
// Prompts: cada contexto con su tono
// ============================================================================
std::string TabernaConversationMgr::BuildTavernPrompt(const std::string& seed, const std::string& botName)
{
    return "Eres " + botName + ", un parroquiano de la cantina 'taberna' en Azeroth (WoW TBC). "
           "Otro parroquiano acaba de decir: \"" + seed + "\" "
           "Contéstale directamente a eso, UNICAMENTE en espanol latino, 1 o 2 frases cortas (maximo 150 caracteres). "
           "Tono: humor de taberna, sabiduria de veterano. Puedes referirte a mazmorras, raids, "
           "clases y memes de WoW TBC. Nunca salgas del rol. No menciones que eres una IA. "
           "Responde solo con el dialogo, sin comillas ni prefijos.";
}

std::string TabernaConversationMgr::BuildGuildPrompt(const std::string& seed, const std::string& botName)
{
    return "Eres " + botName + ", miembro de una hermandad de WoW TBC. "
           "Un companero de gremio escribio en el chat de hermandad: \"" + seed + "\" "
           "Responde UNICAMENTE en espanol latino, con UNA sola frase corta y directa (maximo 100 caracteres). "
           "TONO NORMAL de companero de gremio: tranquilo, util, sin exagerar, sin humor de taberna. "
           "REGLAS: si invita o pide ayuda para mazmorra/raid/invasion/mision, acepta o declina realista y breve "
           "(ej: 'Dale, mandame invite', 'Voy, dame un minuto', 'Ahora no puedo, estoy en una quest'). "
           "Si es duda, responde con sentido comun de veterano. Si es charla, responde tranquilo. "
           "Nunca salgas del rol, no menciones IA, sin comillas ni asteriscos.";
}

std::string TabernaConversationMgr::BuildPartyPrompt(const std::string& seed, const std::string& botName, bool isRaid)
{
    std::string grupo = isRaid ? "una banda (raid)" : "un grupo";
    return "Eres " + botName + ", miembro de " + grupo + " en WoW TBC, en plena actividad. "
           "Un companero escribio en el chat de " + (isRaid ? "raid" : "grupo") + ": \"" + seed + "\" "
           "Responde UNICAMENTE en espanol latino, con UNA frase MUY corta y directa (maximo 70 caracteres). "
           "TONO RESERVADO Y TACTICO: coordinacion de combate, sin humor de taberna, sin gritos, sin efusividad, "
           "sin charla larga. Si es un plan o instruccion de mision/mazmorra/ataque, confirma o aclara brevemente "
           "(ej: 'Entendido, voy delante', 'Listo, espero tu marca', 'Cuidado con el patrullero', 'Marco al objetivo'). "
           "Si es duda tactica, responde util y breve. Nunca salgas del rol, no menciones IA, sin comillas ni asteriscos.";
}

std::string TabernaConversationMgr::BuildTradePrompt(const std::string& seed, const std::string& botName)
{
    return "Eres " + botName + ", un artesano o comerciante de Azeroth (WoW TBC). "
           "Un jugador escribio en el canal de comercio: \"" + seed + "\" "
           "Responde UNICAMENTE en espanol latino, con UNA frase corta (maximo 100 caracteres) promoviendo "
           "tus productos o servicios de forma natural y amable (no spam). Menciona tu profesion "
           "(herreria, sastreria, alquimia, encantamiento, peleteria, joyeria, ingenieria) o un item que vendes, "
           "con precio o invitacion a comerciar (ej: 'Vendo pociones mayores de sanacion, 5g la unidad, susurrame', "
           "'Hago encantamientos de arma, materiales por tu cuenta', 'Compro hierbas, pago bien'). "
           "Nunca salgas del rol, no menciones IA, sin comillas ni asteriscos.";
}

// ============================================================================
// Envios por canal de gremio y por grupo/raid
// ============================================================================
void TabernaConversationMgr::SendGuildSay(Player* bot, const std::string& text)
{
    if (!bot || bot->GetGuildId() == 0)
        return;
    Guild* guild = sGuildMgr.GetGuildById(bot->GetGuildId());
    if (!guild)
    {
        sLog.outString(">> [DIAG-G] SendGuildSay: guild nullptr para bot %s (guildId %u)",
            bot->GetName(), bot->GetGuildId());
        return;
    }

    sLog.outString(">> [DIAG-G] SendGuildSay: bot=%s guild=ok texto='%s'",
        bot->GetName(), text.c_str());

    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_GUILD, text.c_str(),
        Language(LANG_UNIVERSAL), bot->GetChatTag(), bot->GetObjectGuid(),
        bot->GetName(), ObjectGuid(), "", "");
    guild->BroadcastPacket(data);
}

void TabernaConversationMgr::SendGroupSay(Player* bot, const std::string& text, bool isRaid)
{
    if (!bot) return;
    Group* group = bot->GetGroup();
    if (!group) return;

    sLog.outString(">> [DIAG-G] SendGroupSay: bot=%s raid=%s texto='%s'",
        bot->GetName(), isRaid ? "si" : "no", text.c_str());

    WorldPacket data;
    ChatHandler::BuildChatPacket(data, isRaid ? CHAT_MSG_RAID : CHAT_MSG_PARTY,
        text.c_str(), Language(LANG_UNIVERSAL), bot->GetChatTag(),
        bot->GetObjectGuid(), bot->GetName(), ObjectGuid(), "", "");

    const Group::MemberSlotList& slots = group->GetMemberSlots();
    for (Group::MemberSlotList::const_iterator itr = slots.begin(); itr != slots.end(); ++itr)
    {
        Player* member = sObjectMgr.GetPlayer(itr->guid);
        if (member && member->GetSession())
            member->GetSession()->SendPacket(data);
    }
}

// ============================================================================
// Failover taberna a BD
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