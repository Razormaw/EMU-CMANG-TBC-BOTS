/* -------------------------------------------------------------------------
 * RaidEnhancement.cpp — estado compartido de la raid (parche IA-raid v2)
 * ------------------------------------------------------------------------- */
#include "RaidEnhancement.h"
#include "PlayerbotAI.h"
#include "PlayerbotClassAI.h"
#include "../../Entities/Player.h"
#include "../../Entities/Group.h"
#include "../../Globals/SharedDefines.h"
#include <sstream>

namespace raidai
{
    /* ----------------------------- estado ------------------------------ */

    static Modules       sModules;
    static AttemptStats  sStats;
    static FormationType sFormation = FORMACION_ABANICO;

    Modules&       ModulesRef()       { return sModules; }
    const Modules& GetModules()       { return sModules; }
    AttemptStats&  StatsRef()         { return sStats; }
    const AttemptStats& GetStats()    { return sStats; }
    void           SetFormation(FormationType f) { sFormation = f; }
    FormationType  GetFormation()     { return sFormation; }

    bool ToggleModule(const std::string& nombre)
    {
        if (nombre == "amenaza")      { sModules.amenaza      = !sModules.amenaza;      return sModules.amenaza; }
        if (nombre == "sanacion")     { sModules.sanacion     = !sModules.sanacion;     return sModules.sanacion; }
        if (nombre == "cortes")       { sModules.cortes       = !sModules.cortes;       return sModules.cortes; }
        if (nombre == "formacion")    { sModules.formacion    = !sModules.formacion;    return sModules.formacion; }
        if (nombre == "consumibles")  { sModules.consumibles  = !sModules.consumibles;  return sModules.consumibles; }
        if (nombre == "telemetria")   { sModules.telemetria   = !sModules.telemetria;   return sModules.telemetria; }
        if (nombre == "resurreccion") { sModules.resurreccion = !sModules.resurreccion; return sModules.resurreccion; }
        return false;   // nombre desconocido: el estado no cambia
    }

    /* -------------------------- zonas hostiles -------------------------- */

    struct HazardZone
    {
        float  x, y, z, radius;
        uint32 ttl;
    };

    static const size_t MAX_HAZARDS = 16;
    static HazardZone sHazards[MAX_HAZARDS];
    static size_t     sHazardCount = 0;

    void MarkHazard(float x, float y, float z, float radius, uint32 ttlTicks)
    {
        // Si ya hay una zona casi igual, refresca su duración.
        for (size_t i = 0; i < sHazardCount; ++i)
        {
            HazardZone& hz = sHazards[i];
            float dx = hz.x - x, dy = hz.y - y;
            if (dx * dx + dy * dy < 4.0f)
            {
                hz.ttl = ttlTicks;
                return;
            }
        }
        if (sHazardCount >= MAX_HAZARDS)
            sHazardCount = 0;   // reutiliza el anillo
        HazardZone& hz = sHazards[sHazardCount++];
        hz.x = x; hz.y = y; hz.z = z; hz.radius = radius; hz.ttl = ttlTicks;
    }

    bool IsInHazard(float x, float y, float z)
    {
        for (size_t i = 0; i < sHazardCount; ++i)
        {
            const HazardZone& hz = sHazards[i];
            float dx = hz.x - x, dy = hz.y - y, dz = hz.z - z;
            if (dx * dx + dy * dy + dz * dz < hz.radius * hz.radius)
                return true;
        }
        return false;
    }

    void TickHazards()
    {
        for (size_t i = 0; i < sHazardCount;)
        {
            if (sHazards[i].ttl > 0)
                --sHazards[i].ttl;
            if (sHazards[i].ttl == 0)
                sHazards[i] = sHazards[--sHazardCount];
            else
                ++i;
        }
    }

    /* --------------------------- telemetría ----------------------------- */

    std::string BuildReport()
    {
        const AttemptStats& s = sStats;
        std::ostringstream out;
        out << "Telemetria del intento (tick " << s.tick << "):"
            << " muertes=" << s.deaths
            << " overheal=" << s.overheal
            << " sanacion=" << s.healing
            << " cortes=" << s.interrupts
            << " wipes=" << s.wipes;
        if (s.healing > 0)
            out << " ratio-overheal=" << (s.overheal * 100 / s.healing) << "%";
        return out.str();
    }

    /* ------------------------ ganchos de combate ------------------------ */

    int CombatTick(Player& bot, Player& master, Unit& target, uint32 jobFlags)
    {
        (void)master;
        sStats.tick++;
        TickHazards();

        // 1. Zona hostil: avisamos al llamador (DoNextCombatManeuverPVE)
        //    para que use su propio FleeFromPointIfCan, que es protected.
        if (sModules.formacion &&
            IsInHazard(bot.GetPositionX(), bot.GetPositionY(), bot.GetPositionZ()))
            return 2;

        // 2. Regla del 80 %: el DPS frena si pisa la amenaza del tanque.
        if (sModules.amenaza && (jobFlags & JOB_DPS) && target.IsAlive())
        {
            ThreatManager& tm = target.GetThreatManager();
            Unit* victim = tm.getCurrentVictim();
            if (victim && victim != (Unit*)&bot)
            {
                float mine = tm.getThreat((Unit*)&bot);
                float tank = tm.getThreat(victim);
                if (tank > 0.0f && mine > tank * 0.80f)
                    return 1;   // no actuar este tick = frenar DPS
            }
        }

        return 0;
    }

    bool GetNearestHazard(float botX, float botY, float& hx, float& hy, float& hz, float& radius)
    {
        bool found = false;
        float best = 1.0e30f;
        for (size_t i = 0; i < sHazardCount; ++i)
        {
            const HazardZone& h = sHazards[i];
            float dx = h.x - botX, dy = h.y - botY;
            float d2 = dx * dx + dy * dy;
            if (d2 < h.radius * h.radius && d2 < best)
            {
                best = d2;
                hx = h.x; hy = h.y; hz = h.z; radius = h.radius;
                found = true;
            }
        }
        return found;
    }

    /* ---------------------- ganchos de no-combate ----------------------- */

    void NonCombatTick(Player& bot, Player& master, PlayerbotClassAI& classAI)
    {
        (void)master;

        // 1. Kit de consumibles: usa el EatDrinkBandage PUBLICO de la clase.
        if (sModules.consumibles)
        {
            if (bot.GetHealthPercent() < 85 || bot.GetPowerPercent(POWER_MANA) < 85)
                classAI.EatDrinkBandage(true, 80, 80, 70);
        }

        // 2. Telemetria: cuenta caidas del grupo para el informe.
        if (sModules.telemetria)
        {
            Group* grp = bot.GetGroup();
            if (grp)
            {
                for (GroupReference* ref = grp->GetFirstMember(); ref; ref = ref->next())
                {
                    Player* m = ref->getSource();
                    if (m && !m->IsAlive())
                        sStats.deaths++;
                }
            }
        }
    }
}
