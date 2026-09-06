/* -------------------------------------------------------------------------
 * RaidEnhancement.h — núcleo del parche IA-raid v2
 * Destino: src/game/PlayerBot/Base/RaidEnhancement.h
 * Core: cmangos/mangos-tbc (PlayerBot integrado, ike3)
 *
 * Convenciones: C++03 (el estandar del core), sin dependencias nuevas.
 * ------------------------------------------------------------------------- */
#ifndef _RAID_ENHANCEMENT_H
#define _RAID_ENHANCEMENT_H

#include "Common.h"
#include <string>

class Player;
class Unit;
class PlayerbotClassAI;

namespace raidai
{
    /* ------------------------- módulos on/off -------------------------- */

    struct Modules
    {
        bool amenaza;        // regla del 80 % + taunts del tanque
        bool sanacion;       // triage: pesos por rol, sin curas pisadas
        bool cortes;         // interrupciones por turnos (lista blanca)
        bool formacion;      // ranuras por rol + zonas hostiles
        bool consumibles;    // kit por rol (comida/bebida/pociones)
        bool telemetria;     // contadores por intento
        bool resurreccion;   // cadena por turnos tras el wipe

        Modules()
            : amenaza(true), sanacion(true), cortes(true), formacion(true),
              consumibles(true), telemetria(true), resurreccion(true) {}
    };

    Modules&       ModulesRef();
    const Modules& GetModules();
    bool           ToggleModule(const std::string& nombre);   // devuelve el nuevo estado

    /* --------------------------- telemetría ---------------------------- */

    struct AttemptStats
    {
        uint32 tick;
        uint32 deaths;
        uint32 overheal;
        uint32 healing;
        uint32 interrupts;
        uint32 wipes;
        AttemptStats() { Reset(); }
        void Reset() { tick = deaths = overheal = healing = interrupts = wipes = 0; }
    };

    AttemptStats&       StatsRef();
    const AttemptStats& GetStats();
    std::string         BuildReport();   // resumen legible para !raid informe

    /* --------------------------- formaciones --------------------------- */

    enum FormationType { FORMACION_ABANICO = 0, FORMACION_STACK, FORMACION_DOBLE_TANQUE };

    void          SetFormation(FormationType f);
    FormationType GetFormation();

    /* -------------------------- zonas hostiles ------------------------- */

    /* Un aura/efecto del jefe pinta un área que los bots deben evitar. */
    void MarkHazard(float x, float y, float z, float radius, uint32 ttlTicks);
    bool IsInHazard(float x, float y, float z);   // para FleeFromPointIfCan
    void TickHazards();                            // se llama una vez por tick global

    /* ------------------------ puntos de gancho ------------------------- */

    /* Combate: llamarlo al inicio de PlayerbotClassAI::DoNextCombatManeuverPVE.
       Códigos de retorno:
         0 = seguir con la maniobra normal
         1 = tick consumido (p. ej. el bot frenó por amenaza)
         2 = el bot está en zona hostil: el llamador debe usar su propio
             FleeFromPointIfCan con GetNearestHazard() (es protected de la clase) */
    int CombatTick(Player& bot, Player& master, Unit& target, uint32 jobFlags);

    /* Centro y radio de la zona hostil más cercana al bot. */
    bool GetNearestHazard(float botX, float botY, float& hx, float& hy, float& hz, float& radius);

    /* Fuera de combate: llamarlo al inicio de DoNonCombatActions pasando
       el propio objeto de clase (usa su EatDrinkBandage, que es público). */
    void NonCombatTick(Player& bot, Player& master, PlayerbotClassAI& classAI);

    /* ------------------- ganchos de alto nivel --------------------------
       Son los que se llaman desde el core (README, puntos A y B). Usan
       CombatTick/NonCombatTick por debajo y añaden cortes por turnos,
       zonas hostiles, pull coordinado y cadena de resurrección.
       Implementados en ClassAIHooks.cpp. */
    int  CombatHook(Player& bot, Player& master, Unit& target, uint32 jobBits);
    void NonCombatHook(Player& bot, Player& master, PlayerbotClassAI& classAI);

    /* Chat: llamarlo donde tu fork procesa los mensajes '!' del maestro
       (WorldSession::HandleMessagechatOpcode o similar). */
    bool HandleChatCommand(Player* botPlayer, const std::string& text);
}

#endif
