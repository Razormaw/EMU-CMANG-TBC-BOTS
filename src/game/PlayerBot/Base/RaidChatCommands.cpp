/* -------------------------------------------------------------------------
 * RaidChatCommands.cpp — comandos de susurro (parche IA-raid v2)
 * Destino: src/game/PlayerBot/Base/RaidChatCommands.cpp
 *
 * Punto de integracion (README C): alli donde tu fork detecta los
 * mensajes del maestro que empiezan por '!', llama a:
 *
 *     if (raidai::HandleChatCommand(botPlayer, text)) return true;
 *
 * botPlayer = el bot que recibe el mensaje.
 * ------------------------------------------------------------------------- */
#include "RaidEnhancement.h"
#include "PlayerbotAI.h"
#include "PlayerbotClassAI.h"
#include "PlayerbotMgr.h"
#include "TalentTemplatesTbc.h"
#include "EncounterScriptsTbc.h"
#include "../../Entities/Player.h"
#include "../../Entities/Group.h"
#include "../../Server/WorldPacket.h"
#include "../../Server/WorldSession.h"
#include <sstream>

namespace raidai
{
    /* Piden pull desde ClassAIHooks.cpp */
    void RequestPull(Player* master);

    /* ------------------------------ utilidades ------------------------- */

    static void SendWhisper(Player* from, Player* to, const std::string& msg)
    {
        if (!from || !to || !to->GetSession())
            return;

        WorldPacket data(SMSG_MESSAGECHAT, 200);
        data << uint8(CHAT_MSG_WHISPER);
        data << uint32(LANG_UNIVERSAL);
        data << from->GetObjectGuid();
        data << uint32(0);
        data << to->GetObjectGuid();
        data << uint32(msg.size() + 1);
        data << msg;
        data << uint8(0);
        to->GetSession()->SendPacket(data);
    }

    static std::string NextToken(const std::string& text, size_t& pos)
    {
        while (pos < text.size() && text[pos] == ' ')
            ++pos;
        size_t start = pos;
        while (pos < text.size() && text[pos] != ' ')
            ++pos;
        return text.substr(start, pos - start);
    }

    /* El maestro humano del grupo (el unico miembro sin IA). */
    static Player* FindMasterOf(Player* botPlayer)
    {
        Group* grp = botPlayer->GetGroup();
        if (grp)
        {
            for (GroupReference* ref = grp->GetFirstMember(); ref; ref = ref->next())
            {
                Player* m = ref->getSource();
                if (m && !m->GetPlayerbotAI())
                    return m;
            }
        }
        return botPlayer;
    }

    /* Busca un bot del grupo por nombre (sin depender de ObjectAccessor). */
    static Player* FindBotByName(Player* master, const std::string& name)
    {
        if (name.empty())
            return NULL;
        Group* grp = master ? master->GetGroup() : NULL;
        if (grp)
        {
            for (GroupReference* ref = grp->GetFirstMember(); ref; ref = ref->next())
            {
                Player* m = ref->getSource();
                if (m && stricmp(m->GetName(), name.c_str()) == 0)
                    return m;
            }
        }
        return NULL;
    }

    /* -------------------------- comandos !raid -------------------------- */

    static void HandleRaid(Player* botPlayer, const std::string& text)
    {
        Player* master = FindMasterOf(botPlayer);
        size_t pos = 5;                       // salta "!raid"
        std::string sub = NextToken(text, pos);

        if (sub == "comp")
        {
            SendWhisper(botPlayer, master, ApplyCompPlan(botPlayer, NextToken(text, pos)));
        }
        else if (sub == "formacion")
        {
            std::string f = NextToken(text, pos);
            SetFormation(f == "doble_tanque" ? FORMACION_DOBLE_TANQUE
                       : f == "stack"        ? FORMACION_STACK
                       :                       FORMACION_ABANICO);
            SendWhisper(botPlayer, master,
                        "Formación de pull cambiada a: " + (f.empty() ? std::string("abanico") : f));
        }
        else if (sub == "guion")
        {
            std::string key = NextToken(text, pos);
            if (!key.empty() && LoadEncounterScript(key))
                SendWhisper(botPlayer, master,
                            "Guion '" + key + "' cargado: cortes y zonas hostiles activos.");
            else
                SendWhisper(botPlayer, master,
                            "Guiones disponibles: gruul, magtheridon, hydross, vashj, "
                            "illidan, archimonde, kazrogal, teron");
        }
        else if (sub == "informe")
        {
            SendWhisper(botPlayer, master, BuildReport());
        }
        else
        {
            SendWhisper(botPlayer, master,
                        "Uso: !raid comp <estandar|gemelos|duro> | "
                        "!raid formacion <abanico|stack|doble_tanque> | "
                        "!raid guion <vashj|gruul|illidan|...> | !raid informe");
        }
    }

    /* ---------------------- comandos de un solo bot --------------------- */

    static void HandleBot(Player* botPlayer, Player* master, const std::string& text)
    {
        size_t pos = 5;                       // salta "!bot "
        std::string name = NextToken(text, pos);
        std::string sub  = NextToken(text, pos);

        Player* target = FindBotByName(master, name);
        if (!target)
        {
            SendWhisper(botPlayer, master, "No encuentro a '" + name + "' en el grupo.");
            return;
        }

        if (sub == "talents" || sub == "talentos" || sub == "spec")
        {
            std::string spec = NextToken(text, pos);
            if (spec.empty())
            {
                SendWhisper(botPlayer, master, "Uso: !bot " + name + " talents <clave-de-spec>");
                return;
            }
            if (target->IsInCombat())
            {
                SendWhisper(botPlayer, master,
                            std::string(target->GetName()) + " está en combate: espera al reset.");
                return;
            }
            if (ApplyTalentTemplate(target, spec))
                SendWhisper(botPlayer, master,
                            std::string(target->GetName()) + " re-especificado a '" + spec + "'.");
            else
                SendWhisper(botPlayer, master,
                            "Spec '" + spec + "' no existe para esa clase. Ej: furia, proteccion, "
                            "sagrado, sombra, ferocidad... (mira TalentTemplatesTbc.cpp).");
        }
        else if (sub == "stats")
        {
            std::ostringstream out;
            out << target->GetName() << ": nivel " << target->getLevel()
                << ", vida " << target->GetHealthPercent() << "%"
                << ", rol " << (uint32)target->getClass();
            SendWhisper(botPlayer, master, out.str());
        }
        else
        {
            SendWhisper(botPlayer, master, "Uso: !bot <nombre> talents <spec> | !bot <nombre> stats");
        }
    }

    /* --------------------------- punto de entrada ----------------------- */

    bool HandleChatCommand(Player* botPlayer, const std::string& text)
    {
        if (!botPlayer || text.empty() || text[0] != '!')
            return false;

        /* !raid ... — comandos de grupo (no necesitan PlayerbotMgr) */
        if (text.compare(0, 5, "!raid") == 0)
        {
            HandleRaid(botPlayer, text);
            return true;
        }

        /* !pull — el tanque del bot inicia el pull coordinado */
        if (text.compare(0, 5, "!pull") == 0)
        {
            RequestPull(FindMasterOf(botPlayer));
            SendWhisper(botPlayer, FindMasterOf(botPlayer), "Pull solicitado: el tanque inicia.");
            return true;
        }

        /* !bots — lista los bots del grupo */
        if (text.compare(0, 5, "!bots") == 0)
        {
            std::ostringstream out;
            uint32 count = 0;
            Group* grp = botPlayer->GetGroup();
            if (grp)
            {
                for (GroupReference* ref = grp->GetFirstMember(); ref; ref = ref->next())
                {
                    Player* m = ref->getSource();
                    if (m && m->GetPlayerbotAI())
                    {
                        if (count > 0) out << ", ";
                        out << m->GetName() << " (" << (uint32)m->getClass() << ")";
                        ++count;
                    }
                }
            }
            SendWhisper(botPlayer, FindMasterOf(botPlayer),
                        count ? out.str() : std::string("No hay bots en el grupo."));
            return true;
        }

        /* !bot <nombre> <subcomando> */
        if (text.compare(0, 5, "!bot ") == 0)
        {
            HandleBot(botPlayer, FindMasterOf(botPlayer), text);
            return true;
        }

        return false;
    }
}
