/* -------------------------------------------------------------------------
 * ClassAIHooks.cpp — ganchos del PlayerbotClassAI (parche IA-raid v2)
 * Destino: src/game/PlayerBot/Base/ClassAIHooks.cpp
 *
 * Este archivo solo aporta funciones que LLAMAN a los puntos de gancho
 * descritos en el README. No modifica ninguna clase del core.
 * ------------------------------------------------------------------------- */
#include "RaidEnhancement.h"
#include "PlayerbotAI.h"
#include "PlayerbotClassAI.h"
#include "EncounterScriptsTbc.h"
#include "TalentTemplatesTbc.h"
#include "../../Entities/Player.h"
#include "../../Entities/Unit.h"
#include "../../Entities/Group.h"
#include "../../Globals/SharedDefines.h"
#include <ctime>

/* Todo vive en el namespace raidai; compila como parte del proyecto
   PlayerBot sin tocar ninguna clase del core. */

namespace raidai
{
    /* ------------------- estado local de los ganchos ------------------- */

    static uint64 sPullRequester = 0;      // guid del maestro que pidio !pull
    static uint64 sRezTurn       = 0;      // guid del sanador con el turno de rez
    static uint32 sRezTurnUntil  = 0;

    /* Peticion de pull desde el chat (la escribe RaidChatCommands.cpp). */
    void RequestPull(Player* master)
    {
        if (master)
            sPullRequester = master->GetObjectGuid().GetRawValue();
    }

    /* ------------------------- ayuda interna --------------------------- */

    /* El jefe esta canalizando algo de la lista blanca del guion activo? */
    static bool BossIsCastingWhitelisted(Unit* boss, const uint32* whitelist, uint8 count)
    {
        for (uint8 i = 0; i < count; ++i)
        {
            if (whitelist[i] != 0 && boss->HasAura(whitelist[i], EFFECT_INDEX_0))
                return true;
        }
        return false;
    }

    /* Corte por clase (TBC): devuelve 0 si la clase no tiene corte. */
    static uint32 InterruptSpellFor(uint8 classId)
    {
        switch (classId)
        {
            case CLASS_ROGUE:   return 1766;    // Patada
            case CLASS_MAGE:    return 2139;    // Contrahechizo
            case CLASS_SHAMAN:  return 8042;    // Choque de tierra
            case CLASS_WARRIOR: return 6552;    // Zurrar
            case CLASS_HUNTER:  return 34490;   // Disparo silenciador
            case CLASS_PRIEST:  return 15487;   // Silencio
            default:            return 0;
        }
    }

    /* Hechizo de resurreccion fuera de combate por clase (TBC).
       Verifica el rango exacto contra tu DBC. El druida usa Renacer,
       que es el unico que tiene fuera de combate. */
    static uint32 RezSpellFor(uint8 classId)
    {
        switch (classId)
        {
            case CLASS_PRIEST: return 2006;    // Resurreccion
            case CLASS_PALADIN: return 7328;   // Redencion
            case CLASS_SHAMAN: return 2008;    // Espiritu ancestral
            case CLASS_DRUID:  return 20484;   // Renacer
            default:           return 0;
        }
    }
}

namespace raidai
{
    /* ----------------- gancho de combate (ver README A) ----------------- */

    int CombatHook(Player& bot, Player& master, Unit& target, uint32 jobBits)
    {
        int r = CombatTick(bot, master, target, jobBits);
        if (r != 0)
            return r;

        /* Zona hostil: el aura del jefe pinta el area en el mapa compartido. */
        if (ModulesRef().formacion)
        {
            const EncounterDef* enc = ActiveEncounter();
            if (enc && target.GetEntry() == enc->entry &&
                enc->hazardAura != 0 && target.HasAura(enc->hazardAura, EFFECT_INDEX_0))
            {
                MarkHazard(target.GetPositionX(), target.GetPositionY(),
                           target.GetPositionZ(), enc->hazardRadius, 8);
            }
        }

        /* Cortes por turnos: solo un bot del grupo corta a la vez.
           La lista blanca la pone el guion activo (!raid guion <jefe>). */
        if (ModulesRef().cortes)
        {
            const EncounterDef* enc = ActiveEncounter();
            if (enc && target.GetEntry() == enc->entry &&
                BossIsCastingWhitelisted(&target, enc->interruptible, enc->interruptCount))
            {
                uint32 spell = InterruptSpellFor(bot.getClass());
                if (spell != 0 && InterruptTurnAllowed(bot))
                {
                    bot.CastSpell(&target, spell, false);
                    StatsRef().interrupts++;
                    return 1;   // tick consumido: esta cortando
                }
            }
        }
        return 0;
    }

    /* --------------- gancho de no-combate (ver README B) ---------------- */

    void NonCombatHook(Player& bot, Player& master, PlayerbotClassAI& classAI)
    {
        NonCombatTick(bot, master, classAI);

        /* Pull coordinado: solo el tanque principal carga; el resto espera. */
        if (sPullRequester != 0 &&
            master.GetObjectGuid().GetRawValue() == sPullRequester)
        {
            sPullRequester = 0;   // se consume una sola vez
            if (classAI.CanPull())
                classAI.Pull();
        }

        /* Cadena de resurreccion: un sanador por turno, orden
           tanque -> sanador -> DPS. El resto bebe. */
        if (ModulesRef().resurreccion)
        {
            JOB_TYPE job = classAI.GetBotJob(&bot);
            bool isHealer = (job == JOB_MAIN_HEAL || job == JOB_HEAL);

            if (!isHealer)
                return;

            uint32 now = (uint32)time(NULL);
            if (sRezTurn != bot.GetObjectGuid().GetRawValue())
            {
                // Nadie tiene el turno, o el turno expiro: lo tomo yo.
                if (sRezTurn == 0 || now > sRezTurnUntil)
                {
                    sRezTurn = bot.GetObjectGuid().GetRawValue();
                    sRezTurnUntil = now + 20;   // 20 s maximo por turno
                }
                else
                {
                    classAI.EatDrinkBandage(true, 30, 30, 70);  // bebo mientras tanto
                    return;
                }
            }

            if (bot.GetPowerPercent(POWER_MANA) < 25)
            {
                // Sin mana: libero el turno para el siguiente sanador.
                sRezTurn = 0;
                classAI.EatDrinkBandage(true, 20, 20, 70);
                return;
            }

            // Revivo en orden tactico (tanque -> sanador -> DPS) usando
            // SOLO API publica: escaneo del grupo + Player::CastSpell.
            // (GetResurrectionTarget/ResurrectPlayer son protected.)
            Player* rezTarget = NULL;
            int bestPrio = 99;
            Group* grp = bot.GetGroup();
            if (grp)
            {
                for (GroupReference* ref = grp->GetFirstMember(); ref; ref = ref->next())
                {
                    Player* m = ref->getSource();
                    if (!m || m->IsAlive() || !m->IsInWorld())
                        continue;
                    if (!bot.IsWithinDistInMap(m, 30.0f))
                        continue;
                    JOB_TYPE j = classAI.GetBotJob(m);
                    int prio = (j == JOB_MAIN_TANK || j == JOB_TANK)   ? 0
                             : (j == JOB_MAIN_HEAL || j == JOB_HEAL)   ? 1
                             :                                           2;
                    if (prio < bestPrio)
                    {
                        bestPrio = prio;
                        rezTarget = m;
                    }
                }
            }

            if (rezTarget)
            {
                uint32 rezSpell = RezSpellFor(bot.getClass());
                if (rezSpell != 0)
                    bot.CastSpell(rezTarget, rezSpell, false);
            }
            else
                sRezTurn = 0;   // no queda nadie: libero el turno
        }
    }
}
